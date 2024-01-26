/** @file
 * Shared Clipboard - Common header for the service extension.
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

#ifndef VBOX_INCLUDED_HostServices_VBoxClipboardExt_h
#define VBOX_INCLUDED_HostServices_VBoxClipboardExt_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
# include <VBox/GuestHost/SharedClipboard-transfers.h>
#endif

#define VBOX_CLIPBOARD_EXT_FN_SET_CALLBACK         (0)
#define VBOX_CLIPBOARD_EXT_FN_FORMAT_ANNOUNCE      (1)
#define VBOX_CLIPBOARD_EXT_FN_DATA_READ            (2)
#define VBOX_CLIPBOARD_EXT_FN_DATA_WRITE           (3)

typedef DECLCALLBACKTYPE(int, FNVRDPCLIPBOARDEXTCALLBACK,(uint32_t u32Function, uint32_t u32Format, void *pvData, uint32_t cbData));
typedef FNVRDPCLIPBOARDEXTCALLBACK *PFNVRDPCLIPBOARDEXTCALLBACK;

typedef struct _SHCLEXTPARMS
{
    uint32_t                        uFormat;
    union
    {
        void                       *pvData;
        PFNVRDPCLIPBOARDEXTCALLBACK pfnCallback;
    } u;
    uint32_t   cbData;
} SHCLEXTPARMS;

#endif /* !VBOX_INCLUDED_HostServices_VBoxClipboardExt_h */
