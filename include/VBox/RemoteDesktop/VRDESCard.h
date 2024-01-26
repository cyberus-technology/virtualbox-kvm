/** @file
 * VBox Remote Desktop Extension (VRDE) - SmartCard interface.
 */

/*
 * Copyright (C) 2011-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_RemoteDesktop_VRDESCard_h
#define VBOX_INCLUDED_RemoteDesktop_VRDESCard_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/RemoteDesktop/VRDE.h>

/*
 * Interface for accessing the smart card reader devices on the client.
 *
 * Async callbacks are used for providing feedback, reporting errors, etc.
 *
 * The caller prepares a VRDESCARD*REQ structure and submits it.
 */

#define VRDE_SCARD_INTERFACE_NAME "SCARD"

/** The VRDE server smart card access interface entry points. Interface version 1. */
typedef struct VRDESCARDINTERFACE
{
    /** The header. */
    VRDEINTERFACEHDR header;

    /** Submit an async IO request to the client.
     *
     * @param hServer The VRDE server instance.
     * @param pvUser  The callers context of this request.
     * @param u32Function The function: VRDE_SCARD_FN_*.
     * @param pvData Function specific data: VRDESCARD*REQ.
     * @param cbData Size of data.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDESCardRequest, (HVRDESERVER hServer,
                                                 void *pvUser,
                                                 uint32_t u32Function,
                                                 const void *pvData,
                                                 uint32_t cbData));

} VRDESCARDINTERFACE;

/* Smartcard interface callbacks. */
typedef struct VRDESCARDCALLBACKS
{
    /** The header. */
    VRDEINTERFACEHDR header;

    /** Notifications.
     *
     * @param pvContext The callbacks context specified in VRDEGetInterface.
     * @param u32Id     The notification identifier: VRDE_SCARD_NOTIFY_*.
     * @param pvData    The notification specific data.
     * @param cbData    The size of buffer pointed by pvData.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDESCardCbNotify, (void *pvContext,
                                                  uint32_t u32Id,
                                                  void *pvData,
                                                  uint32_t cbData));

    /** IO response.
     *
     * @param pvContext The callbacks context specified in VRDEGetInterface.
     * @param rcRequest The IPRT status code for the request.
     * @param pvUser    The pvUser parameter of VRDESCardRequest.
     * @param u32Function The completed function: VRDE_SCARD_FN_*.
     * @param pvData    Function specific response data: VRDESCARD*RSP.
     * @param cbData    The size of the buffer pointed by pvData.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDESCardCbResponse, (void *pvContext,
                                                    int rcRequest,
                                                    void *pvUser,
                                                    uint32_t u32Function,
                                                    void *pvData,
                                                    uint32_t cbData));
} VRDESCARDCALLBACKS;


/*
 * Notifications.
 * u32Id parameter of VRDESCARDCALLBACKS::VRDESCardCbNotify.
 */

#define VRDE_SCARD_NOTIFY_ATTACH 1 /* A SCARD RDPDR device has been attached. */
#define VRDE_SCARD_NOTIFY_DETACH 2 /* A SCARD RDPDR device has been detached. */

/*
 * Notifications.
 * Data structures: pvData of VRDESCARDCALLBACKS::VRDESCardCbNotify.
 */
typedef struct VRDESCARDNOTIFYATTACH
{
    uint32_t u32ClientId;
    uint32_t u32DeviceId;
} VRDESCARDNOTIFYATTACH;

typedef struct VRDESCARDNOTIFYDETACH
{
    uint32_t u32ClientId;
    uint32_t u32DeviceId;
} VRDESCARDNOTIFYDETACH;


/*
 * IO request codes.
 * Must be not 0, which is used internally.
 */

#define VRDE_SCARD_FN_ESTABLISHCONTEXT  1
#define VRDE_SCARD_FN_LISTREADERS       2
#define VRDE_SCARD_FN_RELEASECONTEXT    3
#define VRDE_SCARD_FN_GETSTATUSCHANGE   4
#define VRDE_SCARD_FN_CANCEL            5
#define VRDE_SCARD_FN_CONNECT           6
#define VRDE_SCARD_FN_RECONNECT         7
#define VRDE_SCARD_FN_DISCONNECT        8
#define VRDE_SCARD_FN_BEGINTRANSACTION  9
#define VRDE_SCARD_FN_ENDTRANSACTION   10
#define VRDE_SCARD_FN_STATE            11
#define VRDE_SCARD_FN_STATUS           12
#define VRDE_SCARD_FN_TRANSMIT         13
#define VRDE_SCARD_FN_CONTROL          14
#define VRDE_SCARD_FN_GETATTRIB        15
#define VRDE_SCARD_FN_SETATTRIB        16

#define VRDE_SCARD_MAX_READERS 10
#define VRDE_SCARD_MAX_ATR_LENGTH 36
#define VRDE_SCARD_MAX_PCI_DATA 1024

#define VRDE_SCARD_S_SUCCESS 0x00000000
#define VRDE_SCARD_F_INTERNAL_ERROR 0x80100001
#define VRDE_SCARD_E_CANCELLED 0x80100002
#define VRDE_SCARD_E_INVALID_HANDLE 0x80100003
#define VRDE_SCARD_E_INVALID_PARAMETER 0x80100004
#define VRDE_SCARD_E_INVALID_TARGET 0x80100005
#define VRDE_SCARD_E_NO_MEMORY 0x80100006
#define VRDE_SCARD_F_WAITED_TOO_LONG 0x80100007
#define VRDE_SCARD_E_INSUFFICIENT_BUFFER 0x80100008
#define VRDE_SCARD_E_UNKNOWN_READER 0x80100009
#define VRDE_SCARD_E_TIMEOUT 0x8010000A
#define VRDE_SCARD_E_SHARING_VIOLATION 0x8010000B
#define VRDE_SCARD_E_NO_SMARTCARD 0x8010000C
#define VRDE_SCARD_E_UNKNOWN_CARD 0x8010000D
#define VRDE_SCARD_E_CANT_DISPOSE 0x8010000E
#define VRDE_SCARD_E_PROTO_MISMATCH 0x8010000F
#define VRDE_SCARD_E_NOT_READY 0x80100010
#define VRDE_SCARD_E_INVALID_VALUE 0x80100011
#define VRDE_SCARD_E_SYSTEM_CANCELLED 0x80100012
#define VRDE_SCARD_F_COMM_ERROR 0x80100013
#define VRDE_SCARD_F_UNKNOWN_ERROR 0x80100014
#define VRDE_SCARD_E_INVALID_ATR 0x80100015
#define VRDE_SCARD_E_NOT_TRANSACTED 0x80100016
#define VRDE_SCARD_E_READER_UNAVAILABLE 0x80100017
#define VRDE_SCARD_P_SHUTDOWN 0x80100018
#define VRDE_SCARD_E_PCI_TOO_SMALL 0x80100019
#define VRDE_SCARD_E_ICC_INSTALLATION 0x80100020
#define VRDE_SCARD_E_ICC_CREATEORDER 0x80100021
#define VRDE_SCARD_E_UNSUPPORTED_FEATURE 0x80100022
#define VRDE_SCARD_E_DIR_NOT_FOUND 0x80100023
#define VRDE_SCARD_E_FILE_NOT_FOUND 0x80100024
#define VRDE_SCARD_E_NO_DIR 0x80100025
#define VRDE_SCARD_E_READER_UNSUPPORTED 0x8010001A
#define VRDE_SCARD_E_DUPLICATE_READER 0x8010001B
#define VRDE_SCARD_E_CARD_UNSUPPORTED 0x8010001C
#define VRDE_SCARD_E_NO_SERVICE 0x8010001D
#define VRDE_SCARD_E_SERVICE_STOPPED 0x8010001E
#define VRDE_SCARD_E_UNEXPECTED 0x8010001F
#define VRDE_SCARD_E_NO_FILE 0x80100026
#define VRDE_SCARD_E_NO_ACCESS 0x80100027
#define VRDE_SCARD_E_WRITE_TOO_MANY 0x80100028
#define VRDE_SCARD_E_BAD_SEEK 0x80100029
#define VRDE_SCARD_E_INVALID_CHV 0x8010002A
#define VRDE_SCARD_E_UNKNOWN_RES_MSG 0x8010002B
#define VRDE_SCARD_E_NO_SUCH_CERTIFICATE 0x8010002C
#define VRDE_SCARD_E_CERTIFICATE_UNAVAILABLE 0x8010002D
#define VRDE_SCARD_E_NO_READERS_AVAILABLE 0x8010002E
#define VRDE_SCARD_E_COMM_DATA_LOST 0x8010002F
#define VRDE_SCARD_E_NO_KEY_CONTAINER 0x80100030
#define VRDE_SCARD_E_SERVER_TOO_BUSY 0x80100031
#define VRDE_SCARD_E_PIN_CACHE_EXPIRED 0x80100032
#define VRDE_SCARD_E_NO_PIN_CACHE 0x80100033
#define VRDE_SCARD_E_READ_ONLY_CARD 0x80100034
#define VRDE_SCARD_W_UNSUPPORTED_CARD 0x80100065
#define VRDE_SCARD_W_UNRESPONSIVE_CARD 0x80100066
#define VRDE_SCARD_W_UNPOWERED_CARD 0x80100067
#define VRDE_SCARD_W_RESET_CARD 0x80100068
#define VRDE_SCARD_W_REMOVED_CARD 0x80100069
#define VRDE_SCARD_W_SECURITY_VIOLATION 0x8010006A
#define VRDE_SCARD_W_WRONG_CHV 0x8010006B
#define VRDE_SCARD_W_CHV_BLOCKED 0x8010006C
#define VRDE_SCARD_W_EOF 0x8010006D
#define VRDE_SCARD_W_CANCELLED_BY_USER 0x8010006E
#define VRDE_SCARD_W_CARD_NOT_AUTHENTICATED 0x8010006F
#define VRDE_SCARD_W_CACHE_ITEM_NOT_FOUND 0x80100070
#define VRDE_SCARD_W_CACHE_ITEM_STALE 0x80100071
#define VRDE_SCARD_W_CACHE_ITEM_TOO_BIG 0x80100072

#define VRDE_SCARD_STATE_UNAWARE      0x0000
#define VRDE_SCARD_STATE_IGNORE       0x0001
#define VRDE_SCARD_STATE_CHANGED      0x0002
#define VRDE_SCARD_STATE_UNKNOWN      0x0004
#define VRDE_SCARD_STATE_UNAVAILABLE  0x0008
#define VRDE_SCARD_STATE_EMPTY        0x0010
#define VRDE_SCARD_STATE_PRESENT      0x0020
#define VRDE_SCARD_STATE_ATRMATCH     0x0040
#define VRDE_SCARD_STATE_EXCLUSIVE    0x0080
#define VRDE_SCARD_STATE_INUSE        0x0100
#define VRDE_SCARD_STATE_MUTE         0x0200
#define VRDE_SCARD_STATE_UNPOWERED    0x0400
#define VRDE_SCARD_STATE_MASK         UINT32_C(0x0000FFFF)
#define VRDE_SCARD_STATE_COUNT_MASK   UINT32_C(0xFFFF0000)

#define VRDE_SCARD_PROTOCOL_UNDEFINED 0x00000000
#define VRDE_SCARD_PROTOCOL_T0 0x00000001
#define VRDE_SCARD_PROTOCOL_T1 0x00000002
#define VRDE_SCARD_PROTOCOL_Tx 0x00000003
#define VRDE_SCARD_PROTOCOL_RAW 0x00010000

#define VRDE_SCARD_PROTOCOL_DEFAULT 0x80000000
#define VRDE_SCARD_PROTOCOL_OPTIMAL 0x00000000

#define VRDE_SCARD_SHARE_EXCLUSIVE 0x00000001
#define VRDE_SCARD_SHARE_SHARED 0x00000002
#define VRDE_SCARD_SHARE_DIRECT 0x00000003

/* u32Initialization, u32Disposition */
#define VRDE_SCARD_LEAVE_CARD 0x00000000
#define VRDE_SCARD_RESET_CARD 0x00000001
#define VRDE_SCARD_UNPOWER_CARD 0x00000002
#define VRDE_SCARD_EJECT_CARD 0x00000003

/* VRDESCARDSTATUSRSP::u32State */
#define VRDE_SCARD_UNKNOWN 0x00000000
#define VRDE_SCARD_ABSENT 0x00000001
#define VRDE_SCARD_PRESENT 0x00000002
#define VRDE_SCARD_SWALLOWED 0x00000003
#define VRDE_SCARD_POWERED 0x00000004
#define VRDE_SCARD_NEGOTIABLE 0x00000005
#define VRDE_SCARD_SPECIFICMODE 0x00000006


/*
 * IO request data structures.
 */
typedef struct VRDESCARDCONTEXT
{
    uint32_t u32ContextSize;
    uint8_t au8Context[16];
} VRDESCARDCONTEXT;

typedef struct VRDESCARDHANDLE
{
    VRDESCARDCONTEXT Context;
    uint32_t u32HandleSize;
    uint8_t au8Handle[16];
} VRDESCARDHANDLE;

typedef struct VRDESCARDREADERSTATECALL
{
    char *pszReader; /* UTF8 */
    uint32_t u32CurrentState; /* VRDE_SCARD_STATE_* */
} VRDESCARDREADERSTATECALL;

typedef struct VRDESCARDREADERSTATERETURN
{
    uint32_t u32CurrentState; /* VRDE_SCARD_STATE_* */
    uint32_t u32EventState; /* VRDE_SCARD_STATE_* */
    uint32_t u32AtrLength;
    uint8_t au8Atr[VRDE_SCARD_MAX_ATR_LENGTH];
} VRDESCARDREADERSTATERETURN;

typedef struct VRDESCARDPCI
{
    uint32_t u32Protocol; /* VRDE_SCARD_PROTOCOL_* */
    uint32_t u32PciLength; /* Includes u32Protocol and u32PciLength fields. 8 if no data in au8PciData. */
    uint8_t au8PciData[VRDE_SCARD_MAX_PCI_DATA];
} VRDESCARDPCI;

typedef struct VRDESCARDESTABLISHCONTEXTREQ
{
    uint32_t u32ClientId;
    uint32_t u32DeviceId;
} VRDESCARDESTABLISHCONTEXTREQ;

typedef struct VRDESCARDESTABLISHCONTEXTRSP
{
    uint32_t u32ReturnCode;
    VRDESCARDCONTEXT Context;
} VRDESCARDESTABLISHCONTEXTRSP;

typedef struct VRDESCARDLISTREADERSREQ
{
    VRDESCARDCONTEXT Context;
} VRDESCARDLISTREADERSREQ;

typedef struct VRDESCARDLISTREADERSRSP
{
    uint32_t u32ReturnCode;
    uint32_t cReaders;
    char *apszNames[VRDE_SCARD_MAX_READERS];  /* UTF8 */
} VRDESCARDLISTREADERSRSP;

typedef struct VRDESCARDRELEASECONTEXTREQ
{
    VRDESCARDCONTEXT Context;
} VRDESCARDRELEASECONTEXTREQ;

typedef struct VRDESCARDRELEASECONTEXTRSP
{
    uint32_t u32ReturnCode;
} VRDESCARDRELEASECONTEXTRSP;

typedef struct VRDESCARDGETSTATUSCHANGEREQ
{
    VRDESCARDCONTEXT Context;
    uint32_t u32Timeout; /* Milliseconds. 0xFFFFFFFF = INFINITE */
    uint32_t cReaders;
    VRDESCARDREADERSTATECALL aReaderStates[VRDE_SCARD_MAX_READERS];
} VRDESCARDGETSTATUSCHANGEREQ;

typedef struct VRDESCARDGETSTATUSCHANGERSP
{
    uint32_t u32ReturnCode;
    uint32_t cReaders;
    VRDESCARDREADERSTATERETURN aReaderStates[VRDE_SCARD_MAX_READERS];
} VRDESCARDGETSTATUSCHANGERSP;

typedef struct VRDESCARDCANCELREQ
{
    VRDESCARDCONTEXT Context;
} VRDESCARDCANCELREQ;

typedef struct VRDESCARDCANCELRSP
{
    uint32_t u32ReturnCode;
} VRDESCARDCANCELRSP;

typedef struct VRDESCARDCONNECTREQ
{
    VRDESCARDCONTEXT Context;
    char *pszReader; /* UTF8 */
    uint32_t u32ShareMode; /* VRDE_SCARD_SHARE_* */
    uint32_t u32PreferredProtocols;
} VRDESCARDCONNECTREQ;

typedef struct VRDESCARDCONNECTRSP
{
    uint32_t u32ReturnCode;
    VRDESCARDHANDLE hCard;
    uint32_t u32ActiveProtocol;
} VRDESCARDCONNECTRSP;

typedef struct VRDESCARDRECONNECTREQ
{
    VRDESCARDHANDLE hCard;
    uint32_t u32ShareMode;
    uint32_t u32PreferredProtocols;
    uint32_t u32Initialization;
} VRDESCARDRECONNECTREQ;

typedef struct VRDESCARDRECONNECTRSP
{
    uint32_t u32ReturnCode;
    uint32_t u32ActiveProtocol;
} VRDESCARDRECONNECTRSP;

typedef struct VRDESCARDDISCONNECTREQ
{
    VRDESCARDHANDLE hCard;
    uint32_t u32Disposition;
} VRDESCARDDISCONNECTREQ;

typedef struct VRDESCARDDISCONNECTRSP
{
    uint32_t u32ReturnCode;
} VRDESCARDDISCONNECTRSP;

typedef struct VRDESCARDBEGINTRANSACTIONREQ
{
    VRDESCARDHANDLE hCard;
    uint32_t u32Disposition;
} VRDESCARDBEGINTRANSACTIONREQ;

typedef struct VRDESCARDBEGINTRANSACTIONRSP
{
    uint32_t u32ReturnCode;
} VRDESCARDBEGINTRANSACTIONRSP;

typedef struct VRDESCARDENDTRANSACTIONREQ
{
    VRDESCARDHANDLE hCard;
    uint32_t u32Disposition;
} VRDESCARDENDTRANSACTIONREQ;

typedef struct VRDESCARDENDTRANSACTIONRSP
{
    uint32_t u32ReturnCode;
} VRDESCARDENDTRANSACTIONRSP;

typedef struct VRDESCARDSTATEREQ
{
    VRDESCARDHANDLE hCard;
} VRDESCARDSTATEREQ;

typedef struct VRDESCARDSTATERSP
{
    uint32_t u32ReturnCode;
    uint32_t u32State;
    uint32_t u32Protocol;
    uint32_t u32AtrLength;
    uint8_t au8Atr[VRDE_SCARD_MAX_ATR_LENGTH];
} VRDESCARDSTATERSP;

typedef struct VRDESCARDSTATUSREQ
{
    VRDESCARDHANDLE hCard;
} VRDESCARDSTATUSREQ;

typedef struct VRDESCARDSTATUSRSP
{
    uint32_t u32ReturnCode;
    char *szReader;
    uint32_t u32State;
    uint32_t u32Protocol;
    uint32_t u32AtrLength;
    uint8_t au8Atr[VRDE_SCARD_MAX_ATR_LENGTH];
} VRDESCARDSTATUSRSP;

typedef struct VRDESCARDTRANSMITREQ
{
    VRDESCARDHANDLE hCard;
    VRDESCARDPCI ioSendPci;
    uint32_t u32SendLength;
    uint8_t *pu8SendBuffer;
    uint32_t u32RecvLength;
} VRDESCARDTRANSMITREQ;

typedef struct VRDESCARDTRANSMITRSP
{
    uint32_t u32ReturnCode;
    VRDESCARDPCI ioRecvPci;
    uint32_t u32RecvLength;
    uint8_t *pu8RecvBuffer;
} VRDESCARDTRANSMITRSP;

typedef struct VRDESCARDCONTROLREQ
{
    VRDESCARDHANDLE hCard;
    uint32_t u32ControlCode;
    uint32_t u32InBufferSize;
    uint8_t *pu8InBuffer;
    uint32_t u32OutBufferSize;
} VRDESCARDCONTROLREQ;

typedef struct VRDESCARDCONTROLRSP
{
    uint32_t u32ReturnCode;
    uint32_t u32OutBufferSize;
    uint8_t *pu8OutBuffer;
} VRDESCARDCONTROLRSP;

typedef struct VRDESCARDGETATTRIBREQ
{
    VRDESCARDHANDLE hCard;
    uint32_t u32AttrId;
    uint32_t u32AttrLen;
} VRDESCARDGETATTRIBREQ;

typedef struct VRDESCARDGETATTRIBRSP
{
    uint32_t u32ReturnCode;
    uint32_t u32AttrLength;
    uint8_t *pu8Attr;
} VRDESCARDGETATTRIBRSP;

typedef struct VRDESCARDSETATTRIBREQ
{
    VRDESCARDHANDLE hCard;
    uint32_t u32AttrId;
    uint32_t u32AttrLen;
    uint8_t *pu8Attr;
} VRDESCARDSETATTRIBREQ;

typedef struct VRDESCARDSETATTRIBRSP
{
    uint32_t u32ReturnCode;
} VRDESCARDSETATTRIBRSP;

#endif /* !VBOX_INCLUDED_RemoteDesktop_VRDESCard_h */
