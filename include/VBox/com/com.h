/** @file
 * MS COM / XPCOM Abstraction Layer - COM initialization / shutdown.
 */

/*
 * Copyright (C) 2005-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_com_com_h
#define VBOX_INCLUDED_com_com_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "VBox/com/defs.h"
#include "VBox/com/utils.h"

/** @defgroup grp_com   MS COM / XPCOM Abstraction Layer
 * @{
 */

namespace com
{

/** @name VBOX_COM_INIT_F_XXX - flags for com::Initialize().
 * @{ */
/** Windows: Caller is the GUI and needs a STA rather than MTA apartment. */
#define VBOX_COM_INIT_F_GUI             RT_BIT_32(0)
/** Windows: Auto registration updating, if privileged enough. */
#define VBOX_COM_INIT_F_AUTO_REG_UPDATE RT_BIT_32(1)
/** Windows: Opt-out of COM patching (client code should do this). */
#define VBOX_COM_INIT_F_NO_COM_PATCHING RT_BIT_32(2)
/** The default flags. */
#define VBOX_COM_INIT_F_DEFAULT         (VBOX_COM_INIT_F_AUTO_REG_UPDATE)
/** @} */

/**
 *  Initializes the COM runtime.
 *
 *  Must be called on the main thread, before any COM activity in any thread, and by any thread
 *  willing to perform COM operations.
 *
 *  @return COM result code
 */
HRESULT Initialize(uint32_t fInitFlags = VBOX_COM_INIT_F_DEFAULT);

/**
 *  Shuts down the COM runtime.
 *
 *  Must be called on the main thread before termination.
 *  No COM calls may be made in any thread after this method returns.
 */
HRESULT Shutdown();

/**
 *  Resolves a given interface ID to a string containing the interface name.
 *
 *  If, for some reason, the given IID cannot be resolved to a name, a NULL
 *  string is returned. A non-NULL string returned by this function must be
 *  freed using SysFreeString().
 *
 *  @param aIID     ID of the interface to get a name for
 *  @param aName    Resolved interface name or @c NULL on error
 */
void GetInterfaceNameByIID(const GUID &aIID, BSTR *aName);

#ifdef RT_OS_WINDOWS
void PatchComBugs(void);
#endif

} /* namespace com */

/** @} */
#endif /* !VBOX_INCLUDED_com_com_h */

