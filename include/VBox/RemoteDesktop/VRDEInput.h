/** @file
 * VBox Remote Desktop Extension (VRDE) - Input interface.
 */

/*
 * Copyright (C) 2013-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_RemoteDesktop_VRDEInput_h
#define VBOX_INCLUDED_RemoteDesktop_VRDEInput_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/RemoteDesktop/VRDE.h>

/*
 * Interface for receiving input events from the client.
 */

/* All structures in this file are packed.
 * Everything is little-endian.
 */
#pragma pack(1)

/*
 * The application interface between VirtualBox and the VRDE server.
 */

#define VRDE_INPUT_INTERFACE_NAME "VRDE::INPUT"

/*
 * Supported input methods.
 */
#define VRDE_INPUT_METHOD_TOUCH 1

/*
 * fu32Flags for VRDEInputSetup
 */
#define VRDE_INPUT_F_ENABLE 1

/* The interface entry points. Interface version 1. */
typedef struct VRDEINPUTINTERFACE
{
    /* The header. */
    VRDEINTERFACEHDR header;

    /* Tell the server that an input method will be used or disabled, etc.
     * VRDECallbackInputSetup will be called with a result.
     *
     * @param hServer   The VRDE server instance.
     * @param u32Method The method VRDE_INPUT_METHOD_*.
     * @param fu32Flags What to do with the method VRDE_INPUT_F_*.
     * @param pvSetup   Method specific parameters (optional).
     * @param cbSetup   Size of method specific parameters (optional).
     */
    DECLR3CALLBACKMEMBER(void, VRDEInputSetup, (HVRDESERVER hServer,
                                                uint32_t u32Method,
                                                uint32_t fu32Flags,
                                                const void *pvSetup,
                                                uint32_t cbSetup));
} VRDEINPUTINTERFACE;


/* Interface callbacks. */
typedef struct VRDEINPUTCALLBACKS
{
    /* The header. */
    VRDEINTERFACEHDR header;

    /* VRDPInputSetup async result.
     *
     * @param pvCallback The callbacks context specified in VRDEGetInterface.
     * @param rcSetup    The result code of the request.
     * @param u32Method  The method VRDE_INPUT_METHOD_*.
     * @param pvResult   The result information.
     * @param cbResult   The size of buffer pointed by pvResult.
     */
    DECLR3CALLBACKMEMBER(void, VRDECallbackInputSetup,(void *pvCallback,
                                                      int rcRequest,
                                                      uint32_t u32Method,
                                                      const void *pvResult,
                                                      uint32_t cbResult));

    /* Input event.
     *
     * @param pvCallback The callbacks context specified in VRDEGetInterface.
     * @param u32Method  The method VRDE_INPUT_METHOD_*.
     * @param pvEvent    The event data.
     * @param cbEvent    The size of buffer pointed by pvEvent.
     */
    DECLR3CALLBACKMEMBER(void, VRDECallbackInputEvent,(void *pvCallback,
                                                       uint32_t u32Method,
                                                       const void *pvEvent,
                                                       uint32_t cbEvent));
} VRDEINPUTCALLBACKS;


/*
 * Touch input definitions VRDE_INPUT_METHOD_TOUCH.
 */

/* pvResult is not used */

/* RDPINPUT_HEADER */
typedef struct VRDEINPUTHEADER
{
    uint16_t u16EventId;
    uint32_t u32PDULength;
} VRDEINPUTHEADER;

/* VRDEINPUTHEADER::u16EventId */
#define VRDEINPUT_EVENTID_SC_READY                 0x0001
#define VRDEINPUT_EVENTID_CS_READY                 0x0002
#define VRDEINPUT_EVENTID_TOUCH                    0x0003
#define VRDEINPUT_EVENTID_SUSPEND_TOUCH            0x0004
#define VRDEINPUT_EVENTID_RESUME_TOUCH             0x0005
#define VRDEINPUT_EVENTID_DISMISS_HOVERING_CONTACT 0x0006

/* RDPINPUT_SC_READY_PDU */
typedef struct VRDEINPUT_SC_READY_PDU
{
    VRDEINPUTHEADER header;
    uint32_t u32ProtocolVersion;
} VRDEINPUT_SC_READY_PDU;

#define VRDEINPUT_PROTOCOL_V1 0x00010000
#define VRDEINPUT_PROTOCOL_V101 0x00010001

/* RDPINPUT_CS_READY_PDU */
typedef struct VRDEINPUT_CS_READY_PDU
{
    VRDEINPUTHEADER header;
    uint32_t u32Flags;
    uint32_t u32ProtocolVersion;
    uint16_t u16MaxTouchContacts;
} VRDEINPUT_CS_READY_PDU;

#define VRDEINPUT_READY_FLAGS_SHOW_TOUCH_VISUALS 0x00000001
#define VRDEINPUT_READY_FLAGS_DISABLE_TIMESTAMP_INJECTION 0x00000002

/* RDPINPUT_CONTACT_DATA */
typedef struct VRDEINPUT_CONTACT_DATA
{
    uint8_t u8ContactId;
    uint16_t u16FieldsPresent;
    int32_t i32X;
    int32_t i32Y;
    uint32_t u32ContactFlags;
    int16_t i16ContactRectLeft;
    int16_t i16ContactRectTop;
    int16_t i16ContactRectRight;
    int16_t i16ContactRectBottom;
    uint32_t u32Orientation;
    uint32_t u32Pressure;
} VRDEINPUT_CONTACT_DATA;

#define VRDEINPUT_CONTACT_DATA_CONTACTRECT_PRESENT 0x0001
#define VRDEINPUT_CONTACT_DATA_ORIENTATION_PRESENT 0x0002
#define VRDEINPUT_CONTACT_DATA_PRESSURE_PRESENT 0x0004

#define VRDEINPUT_CONTACT_FLAG_DOWN 0x0001
#define VRDEINPUT_CONTACT_FLAG_UPDATE 0x0002
#define VRDEINPUT_CONTACT_FLAG_UP 0x0004
#define VRDEINPUT_CONTACT_FLAG_INRANGE 0x0008
#define VRDEINPUT_CONTACT_FLAG_INCONTACT 0x0010
#define VRDEINPUT_CONTACT_FLAG_CANCELED 0x0020

/* RDPINPUT_TOUCH_FRAME */
typedef struct VRDEINPUT_TOUCH_FRAME
{
    uint16_t u16ContactCount;
    uint64_t u64FrameOffset;
    VRDEINPUT_CONTACT_DATA aContacts[1];
} VRDEINPUT_TOUCH_FRAME;

/* RDPINPUT_TOUCH_EVENT_PDU */
typedef struct VRDEINPUT_TOUCH_EVENT_PDU
{
    VRDEINPUTHEADER header;
    uint32_t u32EncodeTime;
    uint16_t u16FrameCount;
    VRDEINPUT_TOUCH_FRAME aFrames[1];
} VRDEINPUT_TOUCH_EVENT_PDU;

/* RDPINPUT_SUSPEND_TOUCH_PDU */
typedef struct VRDEINPUT_SUSPEND_TOUCH_PDU
{
    VRDEINPUTHEADER header;
} VRDEINPUT_SUSPEND_TOUCH_PDU;

/* RDPINPUT_RESUME_TOUCH_PDU */
typedef struct VRDEINPUT_RESUME_TOUCH_PDU
{
    VRDEINPUTHEADER header;
} VRDEINPUT_RESUME_TOUCH_PDU;

/* RDPINPUT_DISMISS_HOVERING_CONTACT_PDU */
typedef struct VRDEINPUT_DISMISS_HOVERING_CONTACT_PDU
{
    VRDEINPUTHEADER header;
    uint8_t u8ContactId;
} VRDEINPUT_DISMISS_HOVERING_CONTACT_PDU;

#pragma pack()

#endif /* !VBOX_INCLUDED_RemoteDesktop_VRDEInput_h */
