/** @file
 * VirtualBox - VMCPUSET Operation.
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

#ifndef VBOX_INCLUDED_vmm_vmcpuset_h
#define VBOX_INCLUDED_vmm_vmcpuset_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <iprt/asm.h>
#include <iprt/string.h>

/** @defgroup grp_vmcpuset VMCPUSET Operations
 * @ingroup grp_types_both
 * @sa VMCPUSET
 * @{
 */

/** Tests if a valid CPU ID is present in the set. */
#define VMCPUSET_IS_PRESENT(pSet, idCpu)    ASMBitTest( &(pSet)->au32Bitmap[0], (idCpu))
/** Adds a CPU to the set. */
#define VMCPUSET_ADD(pSet, idCpu)           ASMBitSet(  &(pSet)->au32Bitmap[0], (idCpu))
/** Deletes a CPU from the set. */
#define VMCPUSET_DEL(pSet, idCpu)           ASMBitClear(&(pSet)->au32Bitmap[0], (idCpu))
/** Adds a CPU to the set, atomically. */
#define VMCPUSET_ATOMIC_ADD(pSet, idCpu)    ASMAtomicBitSet(  &(pSet)->au32Bitmap[0], (idCpu))
/** Deletes a CPU from the set, atomically. */
#define VMCPUSET_ATOMIC_DEL(pSet, idCpu)    ASMAtomicBitClear(&(pSet)->au32Bitmap[0], (idCpu))
/** Empties the set. */
#define VMCPUSET_EMPTY(pSet)                memset(&(pSet)->au32Bitmap[0], '\0', sizeof((pSet)->au32Bitmap))
/** Fills the set. */
#define VMCPUSET_FILL(pSet)                 memset(&(pSet)->au32Bitmap[0], 0xff, sizeof((pSet)->au32Bitmap))
/** Checks if two sets are equal to one another. */
#define VMCPUSET_IS_EQUAL(pSet1, pSet2)     (memcmp(&(pSet1)->au32Bitmap[0], &(pSet2)->au32Bitmap[0], sizeof((pSet1)->au32Bitmap)) == 0)
/** Checks if the set is empty. */
#define VMCPUSET_IS_EMPTY(a_pSet)           (   (a_pSet)->au32Bitmap[0] == 0 \
                                             && (a_pSet)->au32Bitmap[1] == 0 \
                                             && (a_pSet)->au32Bitmap[2] == 0 \
                                             && (a_pSet)->au32Bitmap[3] == 0 \
                                             && (a_pSet)->au32Bitmap[4] == 0 \
                                             && (a_pSet)->au32Bitmap[5] == 0 \
                                             && (a_pSet)->au32Bitmap[6] == 0 \
                                             && (a_pSet)->au32Bitmap[7] == 0 \
                                            )
/** Finds the first CPU present in the SET.
 * @returns CPU index if found, NIL_VMCPUID if not. */
#define VMCPUSET_FIND_FIRST_PRESENT(a_pSet) VMCpuSetFindFirstPresentInternal(a_pSet)

/** Implements VMCPUSET_FIND_FIRST_PRESENT.
 *
 * @returns CPU index of the first CPU present in the set, NIL_VMCPUID if none
 *          are present.
 * @param   pSet        The set to scan.
 */
DECLINLINE(int32_t) VMCpuSetFindFirstPresentInternal(PCVMCPUSET pSet)
{
    int i = ASMBitFirstSet(&pSet->au32Bitmap[0], RT_ELEMENTS(pSet->au32Bitmap) * 32);
    return i >= 0 ? (VMCPUID)i : NIL_VMCPUID;
}

/** Finds the first CPU present in the SET.
 * @returns CPU index if found, NIL_VMCPUID if not. */
#define VMCPUSET_FIND_LAST_PRESENT(a_pSet)  VMCpuSetFindLastPresentInternal(a_pSet)

/** Implements VMCPUSET_FIND_LAST_PRESENT.
 *
 * @returns CPU index of the last CPU present in the set, NIL_VMCPUID if none
 *          are present.
 * @param   pSet        The set to scan.
 */
DECLINLINE(int32_t) VMCpuSetFindLastPresentInternal(PCVMCPUSET pSet)
{
    uint32_t i = RT_ELEMENTS(pSet->au32Bitmap);
    while (i-- > 0)
    {
        uint32_t u = pSet->au32Bitmap[i];
        if (u)
        {
            u = ASMBitLastSetU32(u);
            u--;
            u |= i << 5;
            return u;
        }
    }
    return NIL_VMCPUID;
}

/** @} */

#endif /* !VBOX_INCLUDED_vmm_vmcpuset_h */

