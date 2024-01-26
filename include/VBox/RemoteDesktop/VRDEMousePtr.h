/** @file
 * VBox Remote Desktop Extension (VRDE) - Mouse pointer updates interface.
 */

/*
 * Copyright (C) 2012-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_RemoteDesktop_VRDEMousePtr_h
#define VBOX_INCLUDED_RemoteDesktop_VRDEMousePtr_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/RemoteDesktop/VRDE.h>

/*
 * Interface for mouse pointer updates.
 */

#define VRDE_MOUSEPTR_INTERFACE_NAME "MOUSEPTR"

#pragma pack(1)
/* The color mouse pointer information: maximum allowed pointer size is 256x256. */
typedef struct VRDEMOUSEPTRDATA
{
    uint16_t u16HotX;
    uint16_t u16HotY;
    uint16_t u16Width;
    uint16_t u16Height;
    uint16_t u16MaskLen; /* 0 for 32BPP pointers with alpha channel. */
    uint32_t u32DataLen;
    /* uint8_t au8Mask[u16MaskLen]; The 1BPP mask. Optional: does not exist if u16MaskLen == 0. */
    /* uint8_t au8Data[u16DataLen]; The color bitmap, 32 bits color depth. */
} VRDEMOUSEPTRDATA;
#pragma pack()

/** The VRDE server external mouse pointer updates interface entry points. Interface version 1. */
typedef struct VRDEMOUSEPTRINTERFACE
{
    /** The header. */
    VRDEINTERFACEHDR header;

    /** Set the mouse pointer.
     *
     * @param hServer     The server instance handle.
     * @param pPointer    The mouse pointer description.
     *
     */
    DECLR3CALLBACKMEMBER(void, VRDEMousePtr, (HVRDESERVER hServer,
                                              const VRDEMOUSEPTRDATA *pPointer));

} VRDEMOUSEPTRINTERFACE;

#endif /* !VBOX_INCLUDED_RemoteDesktop_VRDEMousePtr_h */
