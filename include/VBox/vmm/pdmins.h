/** @file
 * PDM - Pluggable Device Manager, Common Instance Macros.
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

#ifndef VBOX_INCLUDED_vmm_pdmins_h
#define VBOX_INCLUDED_vmm_pdmins_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/** @defgroup grp_pdm_ins       Common PDM Instance Macros
 * @ingroup grp_pdm
 * @{
 */

/** @def PDMBOTHCBDECL
 * Macro for declaring a callback which is static in HC and exported in GC.
 */
#if defined(IN_RC) || defined(IN_RING0)
# ifdef __cplusplus
#  define PDMBOTHCBDECL(type)   extern "C" DECLEXPORT(type)
# else
#  define PDMBOTHCBDECL(type)   DECLEXPORT(type)
# endif
#else
# define PDMBOTHCBDECL(type)    static DECLCALLBACK(type)
#endif

/** @def PDMINS_2_DATA
 * Gets the shared instance data for a PDM device, USB device, or driver instance.
 * @note For devices using PDMDEVINS_2_DATA is highly recommended.
 */
#define PDMINS_2_DATA(pIns, type)       ( (type)(pIns)->CTX_SUFF(pvInstanceData) )

/** @def PDMINS_2_DATA_CC
 * Gets the current context instance data for a PDM device, USB device, or driver instance.
 * @note For devices using PDMDEVINS_2_DATA_CC is highly recommended.
 */
#define PDMINS_2_DATA_CC(pIns, type)    ( (type)(void *)&(pIns)->achInstanceData[0] )

/* @def PDMINS_2_DATA_RC
 * Gets the raw-mode context instance data for a PDM device instance.
 */
#define PDMINS_2_DATA_RC(pIns, type)    ( (type)(pIns)->CTX_SUFF(pvInstanceDataForRC) )


/** @def PDMINS_2_DATA_RCPTR
 * Converts a PDM Device, USB Device, or Driver instance pointer to a RC pointer to the instance data.
 * @deprecated
 */
#define PDMINS_2_DATA_RCPTR(pIns)   ( (pIns)->pvInstanceDataRC )

/** @def PDMINS_2_DATA_R3PTR
 * Converts a PDM Device, USB Device, or Driver instance pointer to a HC pointer to the instance data.
 * @deprecated
 */
#define PDMINS_2_DATA_R3PTR(pIns)   ( (pIns)->pvInstanceDataR3 )

/** @def PDMINS_2_DATA_R0PTR
 * Converts a PDM Device, USB Device, or Driver instance pointer to a R0 pointer to the instance data.
 * @deprecated
 */
#define PDMINS_2_DATA_R0PTR(pIns)   ( (pIns)->pvInstanceDataR0 )

/** @} */

#endif /* !VBOX_INCLUDED_vmm_pdmins_h */
