/** @file
 *
 * VBox frontends: Framebuffer (FB, DirectFB):
 * Helper routines
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
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef VBOX_INCLUDED_SRC_VBoxFB_Helper_h
#define VBOX_INCLUDED_SRC_VBoxFB_Helper_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#define MAX_VIDEOMODES 64
struct videoMode
{
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
};
extern videoMode g_videoModes[];
extern uint32_t g_numVideoModes;

DFBEnumerationResult enumVideoModesHandler(int width, int height, int bpp, void *callbackdata);
int32_t getBestVideoMode(uint32_t width, uint32_t height, uint32_t bpp);


#endif /* !VBOX_INCLUDED_SRC_VBoxFB_Helper_h */
