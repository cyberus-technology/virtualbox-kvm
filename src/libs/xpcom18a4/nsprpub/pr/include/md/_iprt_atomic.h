/* $Id: _iprt_atomic.h $ */
/** @file
 * IPRT Atomic Operation, for including into a system config header.
 */

/*
 * Copyright (C) 2009-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_SRC_md__iprt_atomic_h
#define VBOX_INCLUDED_SRC_md__iprt_atomic_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Note! Do not copy this around, put it in a header of its own if reused anywhere! */
#include <iprt/asm.h>
#define _PR_HAVE_ATOMIC_OPS
#define _MD_INIT_ATOMIC()               do {} while (0)

DECLINLINE(PRInt32) _PR_IPRT_AtomicIncrement(PRInt32 *pVal)
{
    return ASMAtomicIncS32(pVal);
}
#define _MD_ATOMIC_INCREMENT(pVal)      _PR_IPRT_AtomicIncrement(pVal)

DECLINLINE(PRInt32) _PR_IPRT_AtomicDecrement(PRInt32 *pVal)
{
    return ASMAtomicDecS32(pVal);
}
#define _MD_ATOMIC_DECREMENT(pVal)      _PR_IPRT_AtomicDecrement(pVal)

DECLINLINE(PRInt32) _PR_IPRT_AtomicSet(PRInt32 *pVal, PRInt32 NewVal)
{
    return ASMAtomicXchgS32(pVal, NewVal);
}
#define _MD_ATOMIC_SET(pVal, NewVal)    _PR_IPRT_AtomicSet(pVal, NewVal)

DECLINLINE(PRInt32) _PR_IPRT_AtomicAdd(PRInt32 *pVal, PRInt32 ToAdd)
{
    return ASMAtomicAddS32(pVal, ToAdd) + ToAdd;
}
#define _MD_ATOMIC_ADD(pVal, ToAdd)     _PR_IPRT_AtomicAdd(pVal, ToAdd)

#endif /* !VBOX_INCLUDED_SRC_md__iprt_atomic_h */

