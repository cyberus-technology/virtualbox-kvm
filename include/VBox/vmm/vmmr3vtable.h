/** @file
 * VM - The Virtual Machine Monitor, VTable ring-3 API.
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

#ifndef VBOX_INCLUDED_vmm_vmmr3vtable_h
#define VBOX_INCLUDED_vmm_vmmr3vtable_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/dbgfflowtrace.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/pdmasynccompletion.h>
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/vmm/pdmnetshaper.h>
#include <VBox/vmm/pdmqueue.h>
#include <VBox/vmm/pdmusb.h>
#include <VBox/vmm/pdmthread.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/tm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/dbg.h>

#include <iprt/stdarg.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_vmm_vtable    VMM Function Table
 * @ingroup grp_vmm
 * @{ */


/** Magic and version for the VMM vtable.  (Magic: Emmet Cohen)   */
#define VMMR3VTABLE_MAGIC_VERSION         RT_MAKE_U64(0x19900525, 0x00030000)
/** Compatibility mask: These bits must match - magic and major version. */
#define VMMR3VTABLE_MAGIC_VERSION_MASK    RT_MAKE_U64(0xffffffff, 0xffff0000)

/** Checks if @a a_uTableMagicVersion can be used by code compiled
 *  against @a a_CompiledMagicVersion */
#define VMMR3VTABLE_IS_COMPATIBLE_EX(a_uTableMagicVersion, a_CompiledMagicVersion) \
    (   (a_uTableMagicVersion) >= (a_CompiledMagicVersion) /* table must be same or later version */ \
     && ((a_uTableMagicVersion) & VMMR3VTABLE_MAGIC_VERSION_MASK) == ((a_CompiledMagicVersion) & VMMR3VTABLE_MAGIC_VERSION_MASK) )

/** Checks if @a a_uTableMagicVersion can be used by this us. */
#define VMMR3VTABLE_IS_COMPATIBLE(a_uTableMagicVersion) \
     VMMR3VTABLE_IS_COMPATIBLE_EX(a_uTableMagicVersion, VMMR3VTABLE_MAGIC_VERSION)


/**
 * Function for getting the vtable of a VMM DLL/SO/DyLib.
 *
 * @returns the pointer to the vtable.
 */
typedef DECLCALLBACKTYPE(PCVMMR3VTABLE, FNVMMGETVTABLE,(void));
/** Pointer to VMM vtable getter. */
typedef FNVMMGETVTABLE                 *PFNVMMGETVTABLE;
/** The name of the FNVMMGETVTABLE function. */
#define VMMR3VTABLE_GETTER_NAME         "VMMR3GetVTable"


/**
 * VTable for the ring-3 VMM API.
 */
typedef struct VMMR3VTABLE
{
    /** VMMR3VTABLE_MAGIC_VERSION. */
    uint64_t    uMagicVersion;
    /** Flags (TBD). */
    uint64_t    fFlags;
    /** The description of this VMM. */
    const char *pszDescription;

/** @def VTABLE_ENTRY
 * Define a VTable entry for the given function. */
#if defined(DOXYGEN_RUNNING) \
 || (defined(__cplusplus) && (defined(__clang_major__) || RT_GNUC_PREREQ_EX(4, 8, /*non-gcc: */1) /* For 4.8+ we enable c++11 */))
# define VTABLE_ENTRY(a_Api) /** @copydoc a_Api */ decltype(a_Api) *pfn ## a_Api;
#elif defined(__GNUC__)
# define VTABLE_ENTRY(a_Api) /** @copydoc a_Api */ typeof(a_Api)   *pfn ## a_Api;
#else
# error "Unsupported compiler"
#endif
/** @def VTABLE_RESERVED
 * Define a reserved VTable entry with the given name. */
#define VTABLE_RESERVED(a_Name)  DECLCALLBACKMEMBER(int, a_Name,(void));

#include "vmmr3vtable-def.h"

#undef VTABLE_ENTRY
#undef VTABLE_RESERVED

    /** VMMR3VTABLE_MAGIC_VERSION. */
    uint64_t    uMagicVersionEnd;
} VMMR3VTABLE;

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_vmmr3vtable_h */

