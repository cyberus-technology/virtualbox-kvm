/** @file
 * PDM - Pluggable Device Manager, Async Task.
 */

/*
 * Copyright (C) 2007-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_vmm_pdmasynctask_h
#define VBOX_INCLUDED_vmm_pdmasynctask_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_pdm_async_task    The PDM Async Task API
 * @ingroup grp_pdm
 * @{
 */

/** Pointer to a PDM async task template handle. */
typedef struct PDMASYNCTASKTEMPLATE *PPDMASYNCTASKTEMPLATE;
/** Pointer to a PDM async task template handle pointer. */
typedef PPDMASYNCTASKTEMPLATE *PPPDMASYNCTASKTEMPLATE;

/** Pointer to a PDM async task handle. */
typedef struct PDMASYNCTASK *PPDMASYNCTASK;
/** Pointer to a PDM async task handle pointer. */
typedef PPDMASYNCTASK *PPPDMASYNCTASK;

/* This should be similar to VMReq, only difference there will be a pool
   of worker threads instead of EMT. The actual implementation should be
   made in IPRT so we can reuse it for other stuff later. The reason why
   it should be put in PDM is because we need to manage it wrt to VM
   state changes (need exception - add a flag for this). */

/** @} */


RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_pdmasynctask_h */

