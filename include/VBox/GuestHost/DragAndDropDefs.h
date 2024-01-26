/** @file
 * Drag and Drop definitions - Common header for host service and guest clients.
 */

/*
 * Copyright (C) 2018-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_GuestHost_DragAndDropDefs_h
#define VBOX_INCLUDED_GuestHost_DragAndDropDefs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

/*
 * The mode of operations.
 */
#define VBOX_DRAG_AND_DROP_MODE_OFF           0
#define VBOX_DRAG_AND_DROP_MODE_HOST_TO_GUEST 1
#define VBOX_DRAG_AND_DROP_MODE_GUEST_TO_HOST 2
#define VBOX_DRAG_AND_DROP_MODE_BIDIRECTIONAL 3

#define VBOX_DND_ACTION_IGNORE     UINT32_C(0)
#define VBOX_DND_ACTION_COPY       RT_BIT_32(0)
#define VBOX_DND_ACTION_MOVE       RT_BIT_32(1)
#define VBOX_DND_ACTION_LINK       RT_BIT_32(2)

/** A single DnD action. */
typedef uint32_t VBOXDNDACTION;
/** A list of (OR'ed) DnD actions. */
typedef uint32_t VBOXDNDACTIONLIST;

#define hasDnDCopyAction(a)   ((a) & VBOX_DND_ACTION_COPY)
#define hasDnDMoveAction(a)   ((a) & VBOX_DND_ACTION_MOVE)
#define hasDnDLinkAction(a)   ((a) & VBOX_DND_ACTION_LINK)

#define isDnDIgnoreAction(a)  ((a) == VBOX_DND_ACTION_IGNORE)
#define isDnDCopyAction(a)    ((a) == VBOX_DND_ACTION_COPY)
#define isDnDMoveAction(a)    ((a) == VBOX_DND_ACTION_MOVE)
#define isDnDLinkAction(a)    ((a) == VBOX_DND_ACTION_LINK)

/** @def VBOX_DND_FORMATS_DEFAULT
 * Default drag'n drop formats.
 * Note: If you add new entries here, make sure you test those
 *       with all supported guest OSes!
 */
#define VBOX_DND_FORMATS_DEFAULT                                                                \
    "text/uri-list",                                                                            \
    /* Text. */                                                                                 \
    "text/html",                                                                                \
    "text/plain;charset=utf-8",                                                                 \
    "text/plain;charset=utf-16",                                                                \
    "text/plain",                                                                               \
    "text/richtext",                                                                            \
    "UTF8_STRING",                                                                              \
    "TEXT",                                                                                     \
    "STRING",                                                                                   \
    /* OpenOffice formats. */                                                                   \
    /* See: https://wiki.openoffice.org/wiki/Documentation/DevGuide/OfficeDev/Common_Application_Features#OpenOffice.org_Clipboard_Data_Formats */ \
    "application/x-openoffice-embed-source-xml;windows_formatname=\"Star Embed Source (XML)\"", \
    "application/x-openoffice;windows_formatname=\"Bitmap\""

/**
 * Enumeration for keeping a DnD state.
 */
typedef enum
{
    VBOXDNDSTATE_UNKNOWN = 0,
    VBOXDNDSTATE_ENTERED,
    VBOXDNDSTATE_LEFT,
    VBOXDNDSTATE_QUERY_FORMATS,
    VBOXDNDSTATE_QUERY_STATUS,
    VBOXDNDSTATE_DRAGGING,
    VBOXDNDSTATE_DROP_STARTED,
    VBOXDNDSTATE_DROP_ENDED,
    VBOXDNDSTATE_CANCELLED,
    VBOXDNDSTATE_ERROR
} VBOXDNDSTATE;
/** Pointer to a DnD state. */
typedef VBOXDNDSTATE *PVBOXDNDSTATE;

#endif /* !VBOX_INCLUDED_GuestHost_DragAndDropDefs_h */

