/** @file
 * USBLib - Library for wrapping up the VBoxUSB functionality, Darwin flavor.
 * (DEV,HDrv,Main)
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

#ifndef VBOX_INCLUDED_usblib_darwin_h
#define VBOX_INCLUDED_usblib_darwin_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/cdefs.h>
#include <VBox/usbfilter.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_usblib_darwin     Darwin USB Specifics
 * @ingroup grp_usblib
 * @{
 */

/** @name VBoxUSB specific device properties.
 * VBoxUSB makes use of the OWNER property for communicating between the probe and
 * start stage.
 * USBProxyServiceDarwin makes use of all of them to correctly determine the state
 * of the device.
 * @{ */
/** Contains the pid of the current client. If 0, the kernel is the current client. */
#define VBOXUSB_CLIENT_KEY  "VBoxUSB-Client"
/** Contains the pid of the filter owner (i.e. the VBoxSVC pid). */
#define VBOXUSB_OWNER_KEY   "VBoxUSB-Owner"
/** Contains the ID of the matching filter. */
#define VBOXUSB_FILTER_KEY  "VBoxUSB-Filter"
/** @} */

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_usblib_darwin_h */

