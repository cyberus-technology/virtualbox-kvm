/** @file
 * IPRT - Spinlocks.
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

#ifndef IPRT_INCLUDED_spinlock_h
#define IPRT_INCLUDED_spinlock_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN


/** @defgroup grp_rt_spinlock   RTSpinlock - Spinlocks
 * @ingroup grp_rt
 * @{
 */

/**
 * Creates a spinlock.
 *
 * @returns iprt status code.
 * @param   pSpinlock   Where to store the spinlock handle.
 * @param   fFlags      Creation flags, see RTSPINLOCK_FLAGS_XXX.
 * @param   pszName     Spinlock name, for debugging purposes.  String lifetime
 *                      must be the same as the lock as it won't be copied.
 */
RTDECL(int)  RTSpinlockCreate(PRTSPINLOCK pSpinlock, uint32_t fFlags, const char *pszName);

/** @name RTSPINLOCK_FLAGS_XXX
 * @{ */
/** Disable interrupts when taking the spinlock, making it interrupt safe
 * (sans NMI of course).
 *
 * This is generally the safest option, though it isn't really required unless
 * the data being protect is also accessed from interrupt handler context. */
#define RTSPINLOCK_FLAGS_INTERRUPT_SAFE     RT_BIT(1)
/** No need to disable interrupts, the protect code/data is not used by
 * interrupt handlers. */
#define RTSPINLOCK_FLAGS_INTERRUPT_UNSAFE   RT_BIT(2)
/** @}  */

/**
 * Destroys a spinlock created by RTSpinlockCreate().
 *
 * @returns iprt status code.
 * @param   Spinlock    Spinlock returned by RTSpinlockCreate().
 */
RTDECL(int)  RTSpinlockDestroy(RTSPINLOCK Spinlock);

/**
 * Acquires the spinlock.
 *
 * @param   Spinlock    The spinlock to acquire.
 */
RTDECL(void) RTSpinlockAcquire(RTSPINLOCK Spinlock);

/**
 * Releases the spinlock.
 *
 * @param   Spinlock    The spinlock to acquire.
 */
RTDECL(void) RTSpinlockRelease(RTSPINLOCK Spinlock);


/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_spinlock_h */

