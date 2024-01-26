/* $Id: VBoxServiceUtils.h $ */
/** @file
 * VBoxServiceUtils - Guest Additions Services (Utilities).
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

#ifndef GA_INCLUDED_SRC_common_VBoxService_VBoxServiceUtils_h
#define GA_INCLUDED_SRC_common_VBoxService_VBoxServiceUtils_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "VBoxServiceInternal.h"

#ifdef VBOX_WITH_GUEST_PROPS
int VGSvcReadProp(uint32_t u32ClientId, const char *pszPropName, char **ppszValue, char **ppszFlags, uint64_t *puTimestamp);
int VGSvcReadPropUInt32(uint32_t u32ClientId, const char *pszPropName, uint32_t *pu32, uint32_t u32Min, uint32_t u32Max);
int VGSvcReadHostProp(uint32_t u32ClientId, const char *pszPropName, bool fReadOnly, char **ppszValue, char **ppszFlags,
                      uint64_t *puTimestamp);
int VGSvcWritePropF(uint32_t u32ClientId, const char *pszName, const char *pszValueFormat, ...);
#endif

#ifdef RT_OS_WINDOWS
int VGSvcUtilWinGetFileVersionString(const char *pszPath, const char *pszFileName, char *pszVersion, size_t cbVersion);
#endif

#endif /* !GA_INCLUDED_SRC_common_VBoxService_VBoxServiceUtils_h */

