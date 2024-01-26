/** @file
 * IPRT - X86 and AMD64 Helpers.
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

#ifndef IPRT_INCLUDED_x86_helpers_h
#define IPRT_INCLUDED_x86_helpers_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>


/** @defgroup grp_rt_x86_helpers    x86 Helper Functions
 * @ingroup grp_rt_x86
 * @{
 */


/**
 * Tests if it a genuine Intel CPU based on the ASMCpuId(0) output.
 *
 * @returns true/false.
 * @param   uEBX    EBX return from ASMCpuId(0)
 * @param   uECX    ECX return from ASMCpuId(0)
 * @param   uEDX    EDX return from ASMCpuId(0)
 */
DECLINLINE(bool) RTX86IsIntelCpu(uint32_t uEBX, uint32_t uECX, uint32_t uEDX)
{
    /* 'GenuineIntel' */
    return uEBX == UINT32_C(0x756e6547)     /* 'Genu' */
        && uEDX == UINT32_C(0x49656e69)     /* 'ineI' */
        && uECX == UINT32_C(0x6c65746e);    /* 'ntel' */
}


/**
 * Tests if it an authentic AMD CPU based on the ASMCpuId(0) output.
 *
 * @returns true/false.
 * @param   uEBX    EBX return from ASMCpuId(0)
 * @param   uECX    ECX return from ASMCpuId(0)
 * @param   uEDX    EDX return from ASMCpuId(0)
 */
DECLINLINE(bool) RTX86IsAmdCpu(uint32_t uEBX, uint32_t uECX, uint32_t uEDX)
{
    /* 'AuthenticAMD' */
    return uEBX == UINT32_C(0x68747541)     /* 'Auth' */
        && uEDX == UINT32_C(0x69746e65)     /* 'enti' */
        && uECX == UINT32_C(0x444d4163);    /* 'dAMD' */
}


/**
 * Tests if it a centaur hauling VIA CPU based on the ASMCpuId(0) output.
 *
 * @returns true/false.
 * @param   uEBX    EBX return from ASMCpuId(0).
 * @param   uECX    ECX return from ASMCpuId(0).
 * @param   uEDX    EDX return from ASMCpuId(0).
 */
DECLINLINE(bool) RTX86IsViaCentaurCpu(uint32_t uEBX, uint32_t uECX, uint32_t uEDX)
{
    /* 'CentaurHauls' */
    return uEBX == UINT32_C(0x746e6543)     /* 'Cent' */
        && uEDX == UINT32_C(0x48727561)     /* 'aurH' */
        && uECX == UINT32_C(0x736c7561);    /* 'auls' */
}


/**
 * Tests if it a Shanghai CPU based on the ASMCpuId(0) output.
 *
 * @returns true/false.
 * @param   uEBX    EBX return from ASMCpuId(0).
 * @param   uECX    ECX return from ASMCpuId(0).
 * @param   uEDX    EDX return from ASMCpuId(0).
 */
DECLINLINE(bool) RTX86IsShanghaiCpu(uint32_t uEBX, uint32_t uECX, uint32_t uEDX)
{
    /* '  Shanghai  ' */
    return uEBX == UINT32_C(0x68532020)     /* '  Sh' */
        && uEDX == UINT32_C(0x68676e61)     /* 'angh' */
        && uECX == UINT32_C(0x20206961);    /* 'ai  ' */
}


/**
 * Tests if it a genuine Hygon CPU based on the ASMCpuId(0) output.
 *
 * @returns true/false.
 * @param   uEBX    EBX return from ASMCpuId(0)
 * @param   uECX    ECX return from ASMCpuId(0)
 * @param   uEDX    EDX return from ASMCpuId(0)
 */
DECLINLINE(bool) RTX86IsHygonCpu(uint32_t uEBX, uint32_t uECX, uint32_t uEDX)
{
    /* 'HygonGenuine' */
    return uEBX == UINT32_C(0x6f677948)     /* Hygo */
        && uECX == UINT32_C(0x656e6975)     /* uine */
        && uEDX == UINT32_C(0x6e65476e);    /* nGen */
}


/**
 * Checks whether ASMCpuId_EAX(0x00000000) indicates a valid range.
 *
 *
 * @returns true/false.
 * @param   uEAX    The EAX value of CPUID leaf 0x00000000.
 *
 * @note    This only succeeds if there are at least two leaves in the range.
 * @remarks The upper range limit is just some half reasonable value we've
 *          picked out of thin air.
 */
DECLINLINE(bool) RTX86IsValidStdRange(uint32_t uEAX)
{
    return uEAX >= UINT32_C(0x00000001) && uEAX <= UINT32_C(0x000fffff);
}


/**
 * Checks whether ASMCpuId_EAX(0x80000000) indicates a valid range.
 *
 * This only succeeds if there are at least two leaves in the range.
 *
 * @returns true/false.
 * @param   uEAX    The EAX value of CPUID leaf 0x80000000.
 *
 * @note    This only succeeds if there are at least two leaves in the range.
 * @remarks The upper range limit is just some half reasonable value we've
 *          picked out of thin air.
 */
DECLINLINE(bool) RTX86IsValidExtRange(uint32_t uEAX)
{
    return uEAX >= UINT32_C(0x80000001) && uEAX <= UINT32_C(0x800fffff);
}


/**
 * Checks whether ASMCpuId_EAX(0x40000000) indicates a valid range.
 *
 * This only succeeds if there are at least two leaves in the range.
 *
 * @returns true/false.
 * @param   uEAX    The EAX value of CPUID leaf 0x40000000.
 *
 * @note    Unlike RTX86IsValidStdRange() and RTX86IsValidExtRange(), a single
 *          leaf is okay here.  So, you always need to check the range.
 * @remarks The upper range limit is take from the intel docs.
 */
DECLINLINE(bool) RTX86IsValidHypervisorRange(uint32_t uEAX)
{
    return uEAX >= UINT32_C(0x40000000) && uEAX <= UINT32_C(0x4fffffff);
}


/**
 * Extracts the CPU family from ASMCpuId(1) or ASMCpuId(0x80000001)
 *
 * @returns Family.
 * @param   uEAX    EAX return from ASMCpuId(1) or ASMCpuId(0x80000001).
 */
DECLINLINE(uint32_t) RTX86GetCpuFamily(uint32_t uEAX)
{
    return ((uEAX >> 8) & 0xf) == 0xf
         ? ((uEAX >> 20) & 0x7f) + 0xf
         : ((uEAX >> 8) & 0xf);
}


/**
 * Extracts the CPU model from ASMCpuId(1) or ASMCpuId(0x80000001), Intel variant.
 *
 * @returns Model.
 * @param   uEAX    EAX from ASMCpuId(1) or ASMCpuId(0x80000001).
 */
DECLINLINE(uint32_t) RTX86GetCpuModelIntel(uint32_t uEAX)
{
    return ((uEAX >> 8) & 0xf) == 0xf || (((uEAX >> 8) & 0xf) == 0x6) /* family! */
         ? ((uEAX >> 4) & 0xf) | ((uEAX >> 12) & 0xf0)
         : ((uEAX >> 4) & 0xf);
}


/**
 * Extracts the CPU model from ASMCpuId(1) or ASMCpuId(0x80000001), AMD variant.
 *
 * @returns Model.
 * @param   uEAX    EAX from ASMCpuId(1) or ASMCpuId(0x80000001).
 */
DECLINLINE(uint32_t) RTX86GetCpuModelAMD(uint32_t uEAX)
{
    return ((uEAX >> 8) & 0xf) == 0xf
         ? ((uEAX >> 4) & 0xf) | ((uEAX >> 12) & 0xf0)
         : ((uEAX >> 4) & 0xf);
}


/**
 * Extracts the CPU model from ASMCpuId(1) or ASMCpuId(0x80000001)
 *
 * @returns Model.
 * @param   uEAX    EAX from ASMCpuId(1) or ASMCpuId(0x80000001).
 * @param   fIntel  Whether it's an intel CPU. Use RTX86IsIntelCpu() or
 *                  RTX86IsIntelCpu().
 */
DECLINLINE(uint32_t) RTX86GetCpuModel(uint32_t uEAX, bool fIntel)
{
    return ((uEAX >> 8) & 0xf) == 0xf || (((uEAX >> 8) & 0xf) == 0x6 && fIntel) /* family! */
         ? ((uEAX >> 4) & 0xf) | ((uEAX >> 12) & 0xf0)
         : ((uEAX >> 4) & 0xf);
}


/**
 * Extracts the CPU stepping from ASMCpuId(1) or ASMCpuId(0x80000001)
 *
 * @returns Model.
 * @param   uEAX    EAX from ASMCpuId(1) or ASMCpuId(0x80000001).
 */
DECLINLINE(uint32_t) RTX86GetCpuStepping(uint32_t uEAX)
{
    return uEAX & 0xf;
}


/** @} */
#endif /* !IPRT_INCLUDED_x86_helpers_h */

