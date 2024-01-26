/** @file
 * MS COM / XPCOM Abstraction Layer - VirtualBox COM Library definitions.
 *
 * @note This is the main header file that COM/XPCOM clients include; however,
 *       it is only a wrapper around another platform-dependent include file
 *       that contains the real COM/XPCOM interface declarations.  That other
 *       include file is generated automatically at build time from
 *       /src/VBox/Main/idl/VirtualBox.xidl, which contains all the VirtualBox
 *       interfaces; the include file is called VirtualBox.h on Windows hosts
 *       and VirtualBox_XPCOM.h on Linux hosts.  The build process places it in
 *       out/{platform}/bin/sdk/include, from where it gets
 *       included by the rest of the VirtualBox code.
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

#ifndef VBOX_INCLUDED_com_VirtualBox_h
#define VBOX_INCLUDED_com_VirtualBox_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* For XPCOM/C++ enum hack checks. */
#include <iprt/assertcompile.h>

/* Generated VirtualBox COM library definition file. */
#if !defined(VBOXCOM_NOINCLUDE)
# if !defined(VBOX_WITH_XPCOM)
#  include <iprt/win/windows.h> /* Included by VirtualBox.h via rpc.h, so include our wrapper with cleanups. */
#  include <VirtualBox.h>
# else
#  define VBOX_WITH_XPCOM_CPP_ENUM_HACK
#  include <VirtualBox_XPCOM.h>
# endif
#endif

/* For convenience. */
#include "VBox/com/defs.h"
#include "VBox/com/ptr.h"

#endif /* !VBOX_INCLUDED_com_VirtualBox_h */

