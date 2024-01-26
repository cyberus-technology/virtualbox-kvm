/** @file
 * GCM - Guest Compatibility Manager.
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

#ifndef VBOX_INCLUDED_vmm_gcm_h
#define VBOX_INCLUDED_vmm_gcm_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/param.h>

#include <VBox/vmm/cpum.h>
#include <VBox/vmm/pdmifs.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_gcm   The Guest Compatibility Manager API
 * @ingroup grp_vmm
 * @{
 */

/**
 * GCM Fixer Identifiers.
 * @remarks Part of saved state!
 */
typedef enum GCMFIXERID
{
    /** None. */
    GCMFIXER_NONE       = 0,
    /** DOS division by zero, the worst. Includes Windows 3.x. */
    GCMFIXER_DBZ_DOS    = RT_BIT(0),
    /** OS/2 (any version) division by zero. */
    GCMFIXER_DBZ_OS2    = RT_BIT(1),
    /** Windows 9x division by zero. */
    GCMFIXER_DBZ_WIN9X  = RT_BIT(2),
    /** 32-bit hack. */
    GCMFIXER_32BIT_HACK = 0x7fffffff
} GCMFIXERID;
AssertCompileSize(GCMFIXERID, sizeof(uint32_t));


#ifdef IN_RING3
/** @defgroup grp_gcm_r3  The GCM Host Context Ring-3 API
 * @{
 */
VMMR3_INT_DECL(int)         GCMR3Init(PVM pVM);
VMMR3_INT_DECL(void)        GCMR3Relocate(PVM pVM, RTGCINTPTR offDelta);
VMMR3_INT_DECL(int)         GCMR3Term(PVM pVM);
VMMR3_INT_DECL(void)        GCMR3Reset(PVM pVM);
/** @} */
#endif /* IN_RING3 */

VMMDECL(bool)               GCMIsEnabled(PVM pVM);
VMM_INT_DECL(bool)          GCMShouldTrapXcptDE(PVMCPUCC pVCpu);
VMM_INT_DECL(VBOXSTRICTRC)  GCMXcptDE(PVMCPUCC pVCpu, PCPUMCTX pCtx, PDISCPUSTATE pDis, uint8_t *pcbInstr);
/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_gcm_h */

