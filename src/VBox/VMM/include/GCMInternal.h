/** @file
 * GCM - Internal header file.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef VMM_INCLUDED_SRC_include_GCMInternal_h
#define VMM_INCLUDED_SRC_include_GCMInternal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/vmm/gcm.h>
#include <VBox/vmm/pgm.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_gcm_int       Internal
 * @ingroup grp_gcm
 * @internal
 * @{
 */

/** The saved state version. */
#define GCM_SAVED_STATE_VERSION         1

/**
 * GCM VM Instance data.
 */
typedef struct GCM
{
    /** The provider that is active for this VM. */
    int32_t                         enmFixerIds;
    /** The interface implementation version. */
    uint32_t                        u32Version;

} GCM;
/** Pointer to GCM VM instance data. */
typedef GCM *PGCM;

/** @} */

RT_C_DECLS_END

#endif /* !VMM_INCLUDED_SRC_include_GCMInternal_h */

