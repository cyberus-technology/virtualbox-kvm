/* $Id: darwin-pasteboard.h $ */
/** @file
 * Shared Clipboard Service - Mac OS X host implementation.
 */

/*
 * Copyright (C) 2008-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_SRC_SharedClipboard_darwin_pasteboard_h
#define VBOX_INCLUDED_SRC_SharedClipboard_darwin_pasteboard_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

typedef struct OpaquePasteboardRef *PasteboardRef;

int initPasteboard(PasteboardRef *pPasteboardRef);
void destroyPasteboard(PasteboardRef *pPasteboardRef);

int queryNewPasteboardFormats(PasteboardRef hPasteboard, uint64_t idOwnership, void *hStrOwnershipFlavor,
                              uint32_t *pfFormats, bool *pfChanged);
int readFromPasteboard(PasteboardRef pPasteboard, uint32_t fFormat, void *pv, uint32_t cb, uint32_t *pcbActual);
int takePasteboardOwnership(PasteboardRef pPasteboard, uint64_t idOwnership, const char *pszOwnershipFlavor,
                            const char *pszOwnershipValue, void **phStrOwnershipFlavor);
int writeToPasteboard(PasteboardRef hPasteboard, uint64_t idOwnership, void const *pv, uint32_t cb, uint32_t fFormat);

#endif /* !VBOX_INCLUDED_SRC_SharedClipboard_darwin_pasteboard_h */

