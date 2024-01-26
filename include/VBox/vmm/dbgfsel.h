/** @file
 * DBGF - Debugger Facility, selector interface partly shared with SELM.
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

#ifndef VBOX_INCLUDED_vmm_dbgfsel_h
#define VBOX_INCLUDED_vmm_dbgfsel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <iprt/x86.h>


/** @addtogroup grp_dbgf
 * @{ */

/**
 * Selector information structure.
 */
typedef struct DBGFSELINFO
{
    /** The base address.
     * For gate descriptors, this is the target address.  */
    RTGCPTR         GCPtrBase;
    /** The limit (-1).
     * For gate descriptors, this is set to zero. */
    RTGCUINTPTR     cbLimit;
    /** The raw descriptor. */
    union
    {
        X86DESC     Raw;
        X86DESC64   Raw64;
    } u;
    /** The selector. */
    RTSEL           Sel;
    /** The target selector for a gate.
     * This is 0 if non-gate descriptor. */
    RTSEL           SelGate;
    /** Flags. */
    uint32_t        fFlags;
} DBGFSELINFO;
/** Pointer to a SELM selector information struct. */
typedef DBGFSELINFO *PDBGFSELINFO;
/** Pointer to a const SELM selector information struct. */
typedef const DBGFSELINFO *PCDBGFSELINFO;

/** @name DBGFSELINFO::fFlags
 * @{ */
/** The CPU is in real mode. */
#define DBGFSELINFO_FLAGS_REAL_MODE     RT_BIT_32(0)
/** The CPU is in protected mode. */
#define DBGFSELINFO_FLAGS_PROT_MODE     RT_BIT_32(1)
/** The CPU is in long mode. */
#define DBGFSELINFO_FLAGS_LONG_MODE     RT_BIT_32(2)
/** The selector is a hyper selector.
 * @todo remove me!  */
#define DBGFSELINFO_FLAGS_HYPER         RT_BIT_32(3)
/** The selector is a gate selector. */
#define DBGFSELINFO_FLAGS_GATE          RT_BIT_32(4)
/** The selector is invalid. */
#define DBGFSELINFO_FLAGS_INVALID       RT_BIT_32(5)
/** The selector not present. */
#define DBGFSELINFO_FLAGS_NOT_PRESENT   RT_BIT_32(6)
/** @}  */


/**
 * Tests whether the selector info describes an expand-down selector or now.
 *
 * @returns true / false.
 * @param   pSelInfo        The selector info.
 */
DECLINLINE(bool) DBGFSelInfoIsExpandDown(PCDBGFSELINFO pSelInfo)
{
    return (pSelInfo)->u.Raw.Gen.u1DescType
        && ((pSelInfo)->u.Raw.Gen.u4Type & (X86_SEL_TYPE_DOWN | X86_SEL_TYPE_CODE)) == X86_SEL_TYPE_DOWN;
}


VMMR3DECL(int) DBGFR3SelInfoValidateCS(PCDBGFSELINFO pSelInfo, RTSEL SelCPL);

/** @}  */

#endif /* !VBOX_INCLUDED_vmm_dbgfsel_h */

