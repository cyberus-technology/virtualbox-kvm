/** @file
 * VBox Remote Desktop Extension (VRDE) - Public APIs.
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

#ifndef VBOX_INCLUDED_RemoteDesktop_VRDE_h
#define VBOX_INCLUDED_RemoteDesktop_VRDE_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>

/** @defgroup grp_vrdp VRDE
 * VirtualBox Remote Desktop Extension (VRDE) interface that lets to use
 * a Remote Desktop server like RDP.
 * @{
 */

RT_C_DECLS_BEGIN

/* Forward declaration of the VRDE server instance handle.
 * This is an opaque pointer for VirtualBox.
 * The VRDE library uses it as a pointer to some internal data.
 */
#ifdef __cplusplus
class VRDEServer;
typedef class VRDEServerType *HVRDESERVER;
#else
struct VRDEServer;
typedef struct VRDEServerType *HVRDESERVER;
#endif /* !__cplusplus */

/* Callback based VRDE server interface declarations. */

/** The color mouse pointer information. */
typedef struct _VRDECOLORPOINTER
{
    uint16_t u16HotX;
    uint16_t u16HotY;
    uint16_t u16Width;
    uint16_t u16Height;
    uint16_t u16MaskLen;
    uint16_t u16DataLen;
    /* The 1BPP mask and the 24BPP bitmap follow. */
} VRDECOLORPOINTER;

/** Audio format information packed in a 32 bit value. */
typedef uint32_t VRDEAUDIOFORMAT;

/** Constructs 32 bit value for given frequency, number of channel and bits per sample. */
#define VRDE_AUDIO_FMT_MAKE(freq, c, bps, s) ((((s) & 0x1) << 28) + (((bps) & 0xFF) << 20) + (((c) & 0xF) << 16) + ((freq) & 0xFFFF))

/** Decode frequency. */
#define VRDE_AUDIO_FMT_SAMPLE_FREQ(a) ((a) & 0xFFFF)
/** Decode number of channels. */
#define VRDE_AUDIO_FMT_CHANNELS(a) (((a) >> 16) & 0xF)
/** Decode number signess. */
#define VRDE_AUDIO_FMT_SIGNED(a) (((a) >> 28) & 0x1)
/** Decode number of bits per sample. */
#define VRDE_AUDIO_FMT_BITS_PER_SAMPLE(a) (((a) >> 20) & 0xFF)
/** Decode number of bytes per sample. */
#define VRDE_AUDIO_FMT_BYTES_PER_SAMPLE(a) ((VRDE_AUDIO_FMT_BITS_PER_SAMPLE(a) + 7) / 8)


/*
 * Audio input.
 */

/* Audio input notifications. */
#define VRDE_AUDIOIN_BEGIN     1
#define VRDE_AUDIOIN_DATA      2
#define VRDE_AUDIOIN_END       3

typedef struct VRDEAUDIOINBEGIN
{
    VRDEAUDIOFORMAT fmt; /* Actual format of data, which will be sent in VRDE_AUDIOIN_DATA events. */
} VRDEAUDIOINBEGIN, *PVRDEAUDIOINBEGIN;


/*
 * Remote USB protocol.
 */

/* The initial version 1. */
#define VRDE_USB_VERSION_1 (1)
/* Version 2: look for VRDE_USB_VERSION_2 comments in the code. */
#define VRDE_USB_VERSION_2 (2)
/* Version 3: look for VRDE_USB_VERSION_3 comments in the code. */
#define VRDE_USB_VERSION_3 (3)

/* The default VRDE server version of Remote USB Protocol. */
#define VRDE_USB_VERSION VRDE_USB_VERSION_3


/** USB backend operations. */
#define VRDE_USB_REQ_OPEN              (0)
#define VRDE_USB_REQ_CLOSE             (1)
#define VRDE_USB_REQ_RESET             (2)
#define VRDE_USB_REQ_SET_CONFIG        (3)
#define VRDE_USB_REQ_CLAIM_INTERFACE   (4)
#define VRDE_USB_REQ_RELEASE_INTERFACE (5)
#define VRDE_USB_REQ_INTERFACE_SETTING (6)
#define VRDE_USB_REQ_QUEUE_URB         (7)
#define VRDE_USB_REQ_REAP_URB          (8)
#define VRDE_USB_REQ_CLEAR_HALTED_EP   (9)
#define VRDE_USB_REQ_CANCEL_URB        (10)

/** USB service operations. */
#define VRDE_USB_REQ_DEVICE_LIST       (11)
#define VRDE_USB_REQ_NEGOTIATE         (12)

/** An operation completion status is a byte. */
typedef uint8_t VRDEUSBSTATUS;

/** USB device identifier is an 32 bit value. */
typedef uint32_t VRDEUSBDEVID;

/** Status codes. */
#define VRDE_USB_STATUS_SUCCESS        ((VRDEUSBSTATUS)0)
#define VRDE_USB_STATUS_ACCESS_DENIED  ((VRDEUSBSTATUS)1)
#define VRDE_USB_STATUS_DEVICE_REMOVED ((VRDEUSBSTATUS)2)

/*
 * Data structures to use with VRDEUSBRequest.
 * The *RET* structures always represent the layout of VRDE data.
 * The *PARM* structures normally the same as VRDE layout.
 * However the VRDE_USB_REQ_QUEUE_URB_PARM has a pointer to
 * URB data in place where actual data will be in VRDE layout.
 *
 * Since replies (*RET*) are asynchronous, the 'success'
 * replies are not required for operations which return
 * only the status code (VRDEUSBREQRETHDR only):
 *  VRDE_USB_REQ_OPEN
 *  VRDE_USB_REQ_RESET
 *  VRDE_USB_REQ_SET_CONFIG
 *  VRDE_USB_REQ_CLAIM_INTERFACE
 *  VRDE_USB_REQ_RELEASE_INTERFACE
 *  VRDE_USB_REQ_INTERFACE_SETTING
 *  VRDE_USB_REQ_CLEAR_HALTED_EP
 *
 */

/* VRDE layout has no alignments. */
#pragma pack(1)
/* Common header for all VRDE USB packets. After the reply hdr follows *PARM* or *RET* data. */
typedef struct _VRDEUSBPKTHDR
{
    /* Total length of the reply NOT including the 'length' field. */
    uint32_t length;
    /* The operation code for which the reply was sent by the client. */
    uint8_t code;
} VRDEUSBPKTHDR;

/* Common header for all return structures. */
typedef struct _VRDEUSBREQRETHDR
{
    /* Device status. */
    VRDEUSBSTATUS status;
    /* Device id. */
    VRDEUSBDEVID id;
} VRDEUSBREQRETHDR;


/* VRDE_USB_REQ_OPEN
 */
typedef struct _VRDE_USB_REQ_OPEN_PARM
{
    uint8_t code;
    VRDEUSBDEVID id;
} VRDE_USB_REQ_OPEN_PARM;

typedef struct _VRDE_USB_REQ_OPEN_RET
{
    VRDEUSBREQRETHDR hdr;
} VRDE_USB_REQ_OPEN_RET;


/* VRDE_USB_REQ_CLOSE
 */
typedef struct _VRDE_USB_REQ_CLOSE_PARM
{
    uint8_t code;
    VRDEUSBDEVID id;
} VRDE_USB_REQ_CLOSE_PARM;

/* The close request has no returned data. */


/* VRDE_USB_REQ_RESET
 */
typedef struct _VRDE_USB_REQ_RESET_PARM
{
    uint8_t code;
    VRDEUSBDEVID id;
} VRDE_USB_REQ_RESET_PARM;

typedef struct _VRDE_USB_REQ_RESET_RET
{
    VRDEUSBREQRETHDR hdr;
} VRDE_USB_REQ_RESET_RET;


/* VRDE_USB_REQ_SET_CONFIG
 */
typedef struct _VRDE_USB_REQ_SET_CONFIG_PARM
{
    uint8_t code;
    VRDEUSBDEVID id;
    uint8_t configuration;
} VRDE_USB_REQ_SET_CONFIG_PARM;

typedef struct _VRDE_USB_REQ_SET_CONFIG_RET
{
    VRDEUSBREQRETHDR hdr;
} VRDE_USB_REQ_SET_CONFIG_RET;


/* VRDE_USB_REQ_CLAIM_INTERFACE
 */
typedef struct _VRDE_USB_REQ_CLAIM_INTERFACE_PARM
{
    uint8_t code;
    VRDEUSBDEVID id;
    uint8_t iface;
} VRDE_USB_REQ_CLAIM_INTERFACE_PARM;

typedef struct _VRDE_USB_REQ_CLAIM_INTERFACE_RET
{
    VRDEUSBREQRETHDR hdr;
} VRDE_USB_REQ_CLAIM_INTERFACE_RET;


/* VRDE_USB_REQ_RELEASE_INTERFACE
 */
typedef struct _VRDE_USB_REQ_RELEASE_INTERFACE_PARM
{
    uint8_t code;
    VRDEUSBDEVID id;
    uint8_t iface;
} VRDE_USB_REQ_RELEASE_INTERFACE_PARM;

typedef struct _VRDE_USB_REQ_RELEASE_INTERFACE_RET
{
    VRDEUSBREQRETHDR hdr;
} VRDE_USB_REQ_RELEASE_INTERFACE_RET;


/* VRDE_USB_REQ_INTERFACE_SETTING
 */
typedef struct _VRDE_USB_REQ_INTERFACE_SETTING_PARM
{
    uint8_t code;
    VRDEUSBDEVID id;
    uint8_t iface;
    uint8_t setting;
} VRDE_USB_REQ_INTERFACE_SETTING_PARM;

typedef struct _VRDE_USB_REQ_INTERFACE_SETTING_RET
{
    VRDEUSBREQRETHDR hdr;
} VRDE_USB_REQ_INTERFACE_SETTING_RET;


/* VRDE_USB_REQ_QUEUE_URB
 */

#define VRDE_USB_TRANSFER_TYPE_CTRL (0)
#define VRDE_USB_TRANSFER_TYPE_ISOC (1)
#define VRDE_USB_TRANSFER_TYPE_BULK (2)
#define VRDE_USB_TRANSFER_TYPE_INTR (3)
#define VRDE_USB_TRANSFER_TYPE_MSG  (4)

#define VRDE_USB_DIRECTION_SETUP (0)
#define VRDE_USB_DIRECTION_IN    (1)
#define VRDE_USB_DIRECTION_OUT   (2)

typedef struct _VRDE_USB_REQ_QUEUE_URB_PARM
{
    uint8_t code;
    VRDEUSBDEVID id;
    uint32_t handle;    /* Distinguishes that particular URB. Later used in CancelURB and returned by ReapURB */
    uint8_t type;
    uint8_t ep;
    uint8_t direction;
    uint32_t urblen;    /* Length of the URB. */
    uint32_t datalen;   /* Length of the data. */
    void *data;         /* In RDP layout the data follow. */
} VRDE_USB_REQ_QUEUE_URB_PARM;

/* The queue URB has no explicit return. The reap URB reply will be
 * eventually the indirect result.
 */


/* VRDE_USB_REQ_REAP_URB
 * Notificationg from server to client that server expects an URB
 * from any device.
 * Only sent if negotiated URB return method is polling.
 * Normally, the client will send URBs back as soon as they are ready.
 */
typedef struct _VRDE_USB_REQ_REAP_URB_PARM
{
    uint8_t code;
} VRDE_USB_REQ_REAP_URB_PARM;


#define VRDE_USB_XFER_OK    (0)
#define VRDE_USB_XFER_STALL (1)
#define VRDE_USB_XFER_DNR   (2)
#define VRDE_USB_XFER_CRC   (3)
/* VRDE_USB_VERSION_2: New error codes. OHCI Completion Codes. */
#define VRDE_USB_XFER_BS    (4)  /* BitStuffing */
#define VRDE_USB_XFER_DTM   (5)  /* DataToggleMismatch */
#define VRDE_USB_XFER_PCF   (6)  /* PIDCheckFailure */
#define VRDE_USB_XFER_UPID  (7)  /* UnexpectedPID */
#define VRDE_USB_XFER_DO    (8)  /* DataOverrun */
#define VRDE_USB_XFER_DU    (9)  /* DataUnderrun */
#define VRDE_USB_XFER_BO    (10) /* BufferOverrun */
#define VRDE_USB_XFER_BU    (11) /* BufferUnderrun */
#define VRDE_USB_XFER_ERR   (12) /* VBox protocol error. */

#define VRDE_USB_REAP_FLAG_CONTINUED (0x0)
#define VRDE_USB_REAP_FLAG_LAST      (0x1)
/* VRDE_USB_VERSION_3: Fragmented URBs. */
#define VRDE_USB_REAP_FLAG_FRAGMENT  (0x2)

#define VRDE_USB_REAP_VALID_FLAGS    (VRDE_USB_REAP_FLAG_LAST)
/* VRDE_USB_VERSION_3: Fragmented URBs. */
#define VRDE_USB_REAP_VALID_FLAGS_3  (VRDE_USB_REAP_FLAG_LAST | VRDE_USB_REAP_FLAG_FRAGMENT)

typedef struct _VRDEUSBREQREAPURBBODY
{
    VRDEUSBDEVID     id;        /* From which device the URB arrives. */
    uint8_t          flags;     /* VRDE_USB_REAP_FLAG_* */
    uint8_t          error;     /* VRDE_USB_XFER_* */
    uint32_t         handle;    /* Handle of returned URB. Not 0. */
    uint32_t         len;       /* Length of data actually transferred. */
    /* 'len' bytes of data follow if direction of this URB was VRDE_USB_DIRECTION_IN. */
} VRDEUSBREQREAPURBBODY;

typedef struct _VRDE_USB_REQ_REAP_URB_RET
{
    /* The REAP URB has no header, only completed URBs are returned. */
    VRDEUSBREQREAPURBBODY body;
    /* Another body may follow, depending on flags. */
} VRDE_USB_REQ_REAP_URB_RET;


/* VRDE_USB_REQ_CLEAR_HALTED_EP
 */
typedef struct _VRDE_USB_REQ_CLEAR_HALTED_EP_PARM
{
    uint8_t code;
    VRDEUSBDEVID id;
    uint8_t ep;
} VRDE_USB_REQ_CLEAR_HALTED_EP_PARM;

typedef struct _VRDE_USB_REQ_CLEAR_HALTED_EP_RET
{
    VRDEUSBREQRETHDR hdr;
} VRDE_USB_REQ_CLEAR_HALTED_EP_RET;


/* VRDE_USB_REQ_CANCEL_URB
 */
typedef struct _VRDE_USB_REQ_CANCEL_URB_PARM
{
    uint8_t code;
    VRDEUSBDEVID id;
    uint32_t handle;
} VRDE_USB_REQ_CANCEL_URB_PARM;

/* The cancel URB request has no return. */


/* VRDE_USB_REQ_DEVICE_LIST
 *
 * Server polls USB devices on client by sending this request
 * periodically. Client sends back a list of all devices
 * connected to it. Each device is assigned with an identifier,
 * that is used to distinguish the particular device.
 */
typedef struct _VRDE_USB_REQ_DEVICE_LIST_PARM
{
    uint8_t code;
} VRDE_USB_REQ_DEVICE_LIST_PARM;

/* Data is a list of the following variable length structures. */
typedef struct _VRDEUSBDEVICEDESC
{
    /* Offset of the next structure. 0 if last. */
    uint16_t oNext;

    /* Identifier of the device assigned by client. */
    VRDEUSBDEVID id;

    /** USB version number. */
    uint16_t        bcdUSB;
    /** Device class. */
    uint8_t         bDeviceClass;
    /** Device subclass. */
    uint8_t         bDeviceSubClass;
    /** Device protocol */
    uint8_t         bDeviceProtocol;
    /** Vendor ID. */
    uint16_t        idVendor;
    /** Product ID. */
    uint16_t        idProduct;
    /** Revision, integer part. */
    uint16_t        bcdRev;
    /** Offset of the UTF8 manufacturer string relative to the structure start. */
    uint16_t        oManufacturer;
    /** Offset of the UTF8 product string relative to the structure start. */
    uint16_t        oProduct;
    /** Offset of the UTF8 serial number string relative to the structure start. */
    uint16_t        oSerialNumber;
    /** Physical USB port the device is connected to. */
    uint16_t        idPort;

} VRDEUSBDEVICEDESC;

#define VRDE_USBDEVICESPEED_UNKNOWN    0 /* Unknown. */
#define VRDE_USBDEVICESPEED_LOW        1 /* Low speed (1.5 Mbit/s). */
#define VRDE_USBDEVICESPEED_FULL       2 /* Full speed (12 Mbit/s). */
#define VRDE_USBDEVICESPEED_HIGH       3 /* High speed (480 Mbit/s). */
#define VRDE_USBDEVICESPEED_VARIABLE   4 /* Variable speed - USB 2.5 / wireless. */
#define VRDE_USBDEVICESPEED_SUPERSPEED 5 /* Super Speed - USB 3.0 */

typedef struct _VRDEUSBDEVICEDESCEXT
{
    VRDEUSBDEVICEDESC desc;

    /* Extended info.
     */

    /** The USB device speed: VRDE_USBDEVICESPEED_*. */
    uint16_t        u16DeviceSpeed;
} VRDEUSBDEVICEDESCEXT;

typedef struct _VRDE_USB_REQ_DEVICE_LIST_RET
{
    VRDEUSBDEVICEDESC body;
    /* Other devices may follow.
     * The list ends with (uint16_t)0,
     * which means that an empty list consists of 2 zero bytes.
     */
} VRDE_USB_REQ_DEVICE_LIST_RET;

typedef struct _VRDE_USB_REQ_DEVICE_LIST_EXT_RET
{
    VRDEUSBDEVICEDESCEXT body;
    /* Other devices may follow.
     * The list ends with (uint16_t)0,
     * which means that an empty list consists of 2 zero bytes.
     */
} VRDE_USB_REQ_DEVICE_LIST_EXT_RET;

/* The server requests the version of the port the device is attached to.
 * The client must use VRDEUSBDEVICEDESCEXT structure.
 */
#define VRDE_USB_SERVER_CAPS_PORT_VERSION 0x0001

typedef struct _VRDEUSBREQNEGOTIATEPARM
{
    uint8_t code;

    /* Remote USB Protocol version. */
    /* VRDE_USB_VERSION_3: the 32 bit field is splitted to 16 bit version and 16 bit flags.
     * Version 1 and 2 servers therefore have 'flags' == 0.
     * Version 3+ servers can send some capabilities in this field, this way it is possible to add
     *  a new capability without increasing the protocol version.
     */
    uint16_t version;
    uint16_t flags; /* See VRDE_USB_SERVER_CAPS_* */

} VRDEUSBREQNEGOTIATEPARM;

/* VRDEUSBREQNEGOTIATERET flags. */
#define VRDE_USB_CAPS_FLAG_ASYNC    (0x0)
#define VRDE_USB_CAPS_FLAG_POLL     (0x1)
/* VRDE_USB_VERSION_2: New flag. */
#define VRDE_USB_CAPS2_FLAG_VERSION (0x2) /* The client is negotiating the protocol version. */
/* VRDE_USB_VERSION_3: New flag. */
#define VRDE_USB_CAPS3_FLAG_EXT     (0x4) /* The client is negotiating the extended flags.
                                           * If this flag is set, then the VRDE_USB_CAPS2_FLAG_VERSION
                                           * must also be set.
                                           */


#define VRDE_USB_CAPS_VALID_FLAGS   (VRDE_USB_CAPS_FLAG_POLL)
/* VRDE_USB_VERSION_2: A set of valid flags. */
#define VRDE_USB_CAPS2_VALID_FLAGS  (VRDE_USB_CAPS_FLAG_POLL | VRDE_USB_CAPS2_FLAG_VERSION)
/* VRDE_USB_VERSION_3: A set of valid flags. */
#define VRDE_USB_CAPS3_VALID_FLAGS  (VRDE_USB_CAPS_FLAG_POLL | VRDE_USB_CAPS2_FLAG_VERSION | VRDE_USB_CAPS3_FLAG_EXT)

typedef struct _VRDEUSBREQNEGOTIATERET
{
    uint8_t flags;
} VRDEUSBREQNEGOTIATERET;

typedef struct _VRDEUSBREQNEGOTIATERET_2
{
    uint8_t flags;
    uint32_t u32Version; /* This field presents only if the VRDE_USB_CAPS2_FLAG_VERSION flag is set. */
} VRDEUSBREQNEGOTIATERET_2;

/* The server requests the version of the port the device is attached to.
 * The client must use VRDEUSBDEVICEDESCEXT structure.
 */
#define VRDE_USB_CLIENT_CAPS_PORT_VERSION 0x00000001

typedef struct _VRDEUSBREQNEGOTIATERET_3
{
    uint8_t flags;
    uint32_t u32Version; /* This field presents only if the VRDE_USB_CAPS2_FLAG_VERSION flag is set. */
    uint32_t u32Flags;   /* This field presents only if both VRDE_USB_CAPS2_FLAG_VERSION and
                          * VRDE_USB_CAPS2_FLAG_EXT flag are set.
                          * See VRDE_USB_CLIENT_CAPS_*
                          */
} VRDEUSBREQNEGOTIATERET_3;
#pragma pack()

#define VRDE_CLIPBOARD_FORMAT_NULL         (0x0)
#define VRDE_CLIPBOARD_FORMAT_UNICODE_TEXT (0x1)
#define VRDE_CLIPBOARD_FORMAT_BITMAP       (0x2)
#define VRDE_CLIPBOARD_FORMAT_HTML         (0x4)

#define VRDE_CLIPBOARD_FUNCTION_FORMAT_ANNOUNCE (0)
#define VRDE_CLIPBOARD_FUNCTION_DATA_READ       (1)
#define VRDE_CLIPBOARD_FUNCTION_DATA_WRITE      (2)


/** Indexes of information values. */

/** Whether a client is connected at the moment.
 * uint32_t
 */
#define VRDE_QI_ACTIVE                 (0)

/** How many times a client connected up to current moment.
 * uint32_t
 */
#define VRDE_QI_NUMBER_OF_CLIENTS      (1)

/** When last connection was established.
 * int64_t time in milliseconds since 1970-01-01 00:00:00 UTC
 */
#define VRDE_QI_BEGIN_TIME             (2)

/** When last connection was terminated or current time if connection still active.
 * int64_t time in milliseconds since 1970-01-01 00:00:00 UTC
 */
#define VRDE_QI_END_TIME               (3)

/** How many bytes were sent in last (current) connection.
 * uint64_t
 */
#define VRDE_QI_BYTES_SENT             (4)

/** How many bytes were sent in all connections.
 * uint64_t
 */
#define VRDE_QI_BYTES_SENT_TOTAL       (5)

/** How many bytes were received in last (current) connection.
 * uint64_t
 */
#define VRDE_QI_BYTES_RECEIVED         (6)

/** How many bytes were received in all connections.
 * uint64_t
 */
#define VRDE_QI_BYTES_RECEIVED_TOTAL   (7)

/** Login user name supplied by the client.
 * UTF8 nul terminated string.
 */
#define VRDE_QI_USER                   (8)

/** Login domain supplied by the client.
 * UTF8 nul terminated string.
 */
#define VRDE_QI_DOMAIN                 (9)

/** The client name supplied by the client.
 * UTF8 nul terminated string.
 */
#define VRDE_QI_CLIENT_NAME            (10)

/** IP address of the client.
 * UTF8 nul terminated string.
 */
#define VRDE_QI_CLIENT_IP              (11)

/** The client software version number.
 * uint32_t.
 */
#define VRDE_QI_CLIENT_VERSION         (12)

/** Public key exchange method used when connection was established.
 *  Values: 0 - RDP4 public key exchange scheme.
 *          1 - X509 sertificates were sent to client.
 * uint32_t.
 */
#define VRDE_QI_ENCRYPTION_STYLE       (13)

/** TCP port where the server listens.
 *  Values: 0 - VRDE server failed to start.
 *          -1 - .
 * int32_t.
 */
#define VRDE_QI_PORT                   (14)


/** Hints what has been intercepted by the application. */
#define VRDE_CLIENT_INTERCEPT_AUDIO       RT_BIT(0)
#define VRDE_CLIENT_INTERCEPT_USB         RT_BIT(1)
#define VRDE_CLIENT_INTERCEPT_CLIPBOARD   RT_BIT(2)
#define VRDE_CLIENT_INTERCEPT_AUDIO_INPUT RT_BIT(3)


/** The version of the VRDE server interface. */
#define VRDE_INTERFACE_VERSION_1 (1)
#define VRDE_INTERFACE_VERSION_2 (2)
#define VRDE_INTERFACE_VERSION_3 (3)
#define VRDE_INTERFACE_VERSION_4 (4)

/** The header that does not change when the interface changes. */
typedef struct _VRDEINTERFACEHDR
{
    /** The version of the interface. */
    uint64_t u64Version;

    /** The size of the structure. */
    uint64_t u64Size;

} VRDEINTERFACEHDR;

/** The VRDE server entry points. Interface version 1. */
typedef struct _VRDEENTRYPOINTS_1
{
    /** The header. */
    VRDEINTERFACEHDR header;

    /** Destroy the server instance.
     *
     * @param hServer The server instance handle.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(void, VRDEDestroy,(HVRDESERVER hServer));

    /** The server should start to accept clients connections.
     *
     * @param hServer The server instance handle.
     * @param fEnable Whether to enable or disable client connections.
     *                When is false, all existing clients are disconnected.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDEEnableConnections,(HVRDESERVER hServer,
                                                   bool fEnable));

    /** The server should disconnect the client.
     *
     * @param hServer     The server instance handle.
     * @param u32ClientId The client identifier.
     * @param fReconnect  Whether to send a "REDIRECT to the same server" packet to the
     *                    client before disconnecting.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(void, VRDEDisconnect,(HVRDESERVER hServer,
                                             uint32_t u32ClientId,
                                             bool fReconnect));

    /**
     * Inform the server that the display was resized.
     * The server will query information about display
     * from the application via callbacks.
     *
     * @param hServer Handle of VRDE server instance.
     */
    DECLR3CALLBACKMEMBER(void, VRDEResize,(HVRDESERVER hServer));

    /**
     * Send a update.
     *
     * Note: the server must access the framebuffer bitmap only when VRDEUpdate is called.
     *       If the have to access the bitmap later or from another thread, then
     *       it must used an intermediate buffer and copy the framebuffer data to the
     *       intermediate buffer in VRDEUpdate.
     *
     * @param hServer   Handle of VRDE server instance.
     * @param uScreenId The screen index.
     * @param pvUpdate  Pointer to VRDEOrders.h::VRDEORDERHDR structure with extra data.
     * @param cbUpdate  Size of the update data.
     */
    DECLR3CALLBACKMEMBER(void, VRDEUpdate,(HVRDESERVER hServer,
                                         unsigned uScreenId,
                                         void *pvUpdate,
                                         uint32_t cbUpdate));

    /**
     * Set the mouse pointer shape.
     *
     * @param hServer  Handle of VRDE server instance.
     * @param pPointer The pointer shape information.
     */
    DECLR3CALLBACKMEMBER(void, VRDEColorPointer,(HVRDESERVER hServer,
                                               const VRDECOLORPOINTER *pPointer));

    /**
     * Hide the mouse pointer.
     *
     * @param hServer Handle of VRDE server instance.
     */
    DECLR3CALLBACKMEMBER(void, VRDEHidePointer,(HVRDESERVER hServer));

    /**
     * Queues the samples to be sent to clients.
     *
     * @param hServer    Handle of VRDE server instance.
     * @param pvSamples  Address of samples to be sent.
     * @param cSamples   Number of samples.
     * @param format     Encoded audio format for these samples.
     *
     * @note Initialized to NULL when the application audio callbacks are NULL.
     */
    DECLR3CALLBACKMEMBER(void, VRDEAudioSamples,(HVRDESERVER hServer,
                                               const void *pvSamples,
                                               uint32_t cSamples,
                                               VRDEAUDIOFORMAT format));

    /**
     * Sets the sound volume on clients.
     *
     * @param hServer    Handle of VRDE server instance.
     * @param left       0..0xFFFF volume level for left channel.
     * @param right      0..0xFFFF volume level for right channel.
     *
     * @note Initialized to NULL when the application audio callbacks are NULL.
     */
    DECLR3CALLBACKMEMBER(void, VRDEAudioVolume,(HVRDESERVER hServer,
                                              uint16_t u16Left,
                                              uint16_t u16Right));

    /**
     * Sends a USB request.
     *
     * @param hServer      Handle of VRDE server instance.
     * @param u32ClientId  An identifier that allows the server to find the corresponding client.
     *                     The identifier is always passed by the server as a parameter
     *                     of the FNVRDEUSBCALLBACK. Note that the value is the same as
     *                     in the VRDESERVERCALLBACK functions.
     * @param pvParm       Function specific parameters buffer.
     * @param cbParm       Size of the buffer.
     *
     * @note Initialized to NULL when the application USB callbacks are NULL.
     */
    DECLR3CALLBACKMEMBER(void, VRDEUSBRequest,(HVRDESERVER hServer,
                                             uint32_t u32ClientId,
                                             void *pvParm,
                                             uint32_t cbParm));

    /**
     * Called by the application when (VRDE_CLIPBOARD_FUNCTION_*):
     *   - (0) guest announces available clipboard formats;
     *   - (1) guest requests clipboard data;
     *   - (2) guest responds to the client's request for clipboard data.
     *
     * @param hServer     The VRDE server handle.
     * @param u32Function The cause of the call.
     * @param u32Format   Bitmask of announced formats or the format of data.
     * @param pvData      Points to: (1) buffer to be filled with clients data;
     *                               (2) data from the host.
     * @param cbData      Size of 'pvData' buffer in bytes.
     * @param pcbActualRead Size of the copied data in bytes.
     *
     * @note Initialized to NULL when the application clipboard callbacks are NULL.
     */
    DECLR3CALLBACKMEMBER(void, VRDEClipboard,(HVRDESERVER hServer,
                                            uint32_t u32Function,
                                            uint32_t u32Format,
                                            void *pvData,
                                            uint32_t cbData,
                                            uint32_t *pcbActualRead));

    /**
     * Query various information from the VRDE server.
     *
     * @param hServer   The VRDE server handle.
     * @param index     VRDE_QI_* identifier of information to be returned.
     * @param pvBuffer  Address of memory buffer to which the information must be written.
     * @param cbBuffer  Size of the memory buffer in bytes.
     * @param pcbOut    Size in bytes of returned information value.
     *
     * @remark The caller must check the *pcbOut. 0 there means no information was returned.
     *         A value greater than cbBuffer means that information is too big to fit in the
     *         buffer, in that case no information was placed to the buffer.
     */
    DECLR3CALLBACKMEMBER(void, VRDEQueryInfo,(HVRDESERVER hServer,
                                            uint32_t index,
                                            void *pvBuffer,
                                            uint32_t cbBuffer,
                                            uint32_t *pcbOut));
} VRDEENTRYPOINTS_1;

/** The VRDE server entry points. Interface version 2.
 *  A new entry point VRDERedirect has been added relative to version 1.
 */
typedef struct _VRDEENTRYPOINTS_2
{
    /** The header. */
    VRDEINTERFACEHDR header;

    /** Destroy the server instance.
     *
     * @param hServer The server instance handle.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(void, VRDEDestroy,(HVRDESERVER hServer));

    /** The server should start to accept clients connections.
     *
     * @param hServer The server instance handle.
     * @param fEnable Whether to enable or disable client connections.
     *                When is false, all existing clients are disconnected.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDEEnableConnections,(HVRDESERVER hServer,
                                                   bool fEnable));

    /** The server should disconnect the client.
     *
     * @param hServer     The server instance handle.
     * @param u32ClientId The client identifier.
     * @param fReconnect  Whether to send a "REDIRECT to the same server" packet to the
     *                    client before disconnecting.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(void, VRDEDisconnect,(HVRDESERVER hServer,
                                             uint32_t u32ClientId,
                                             bool fReconnect));

    /**
     * Inform the server that the display was resized.
     * The server will query information about display
     * from the application via callbacks.
     *
     * @param hServer Handle of VRDE server instance.
     */
    DECLR3CALLBACKMEMBER(void, VRDEResize,(HVRDESERVER hServer));

    /**
     * Send a update.
     *
     * Note: the server must access the framebuffer bitmap only when VRDEUpdate is called.
     *       If the have to access the bitmap later or from another thread, then
     *       it must used an intermediate buffer and copy the framebuffer data to the
     *       intermediate buffer in VRDEUpdate.
     *
     * @param hServer   Handle of VRDE server instance.
     * @param uScreenId The screen index.
     * @param pvUpdate  Pointer to VRDEOrders.h::VRDEORDERHDR structure with extra data.
     * @param cbUpdate  Size of the update data.
     */
    DECLR3CALLBACKMEMBER(void, VRDEUpdate,(HVRDESERVER hServer,
                                         unsigned uScreenId,
                                         void *pvUpdate,
                                         uint32_t cbUpdate));

    /**
     * Set the mouse pointer shape.
     *
     * @param hServer  Handle of VRDE server instance.
     * @param pPointer The pointer shape information.
     */
    DECLR3CALLBACKMEMBER(void, VRDEColorPointer,(HVRDESERVER hServer,
                                               const VRDECOLORPOINTER *pPointer));

    /**
     * Hide the mouse pointer.
     *
     * @param hServer Handle of VRDE server instance.
     */
    DECLR3CALLBACKMEMBER(void, VRDEHidePointer,(HVRDESERVER hServer));

    /**
     * Queues the samples to be sent to clients.
     *
     * @param hServer    Handle of VRDE server instance.
     * @param pvSamples  Address of samples to be sent.
     * @param cSamples   Number of samples.
     * @param format     Encoded audio format for these samples.
     *
     * @note Initialized to NULL when the application audio callbacks are NULL.
     */
    DECLR3CALLBACKMEMBER(void, VRDEAudioSamples,(HVRDESERVER hServer,
                                               const void *pvSamples,
                                               uint32_t cSamples,
                                               VRDEAUDIOFORMAT format));

    /**
     * Sets the sound volume on clients.
     *
     * @param hServer    Handle of VRDE server instance.
     * @param left       0..0xFFFF volume level for left channel.
     * @param right      0..0xFFFF volume level for right channel.
     *
     * @note Initialized to NULL when the application audio callbacks are NULL.
     */
    DECLR3CALLBACKMEMBER(void, VRDEAudioVolume,(HVRDESERVER hServer,
                                              uint16_t u16Left,
                                              uint16_t u16Right));

    /**
     * Sends a USB request.
     *
     * @param hServer      Handle of VRDE server instance.
     * @param u32ClientId  An identifier that allows the server to find the corresponding client.
     *                     The identifier is always passed by the server as a parameter
     *                     of the FNVRDEUSBCALLBACK. Note that the value is the same as
     *                     in the VRDESERVERCALLBACK functions.
     * @param pvParm       Function specific parameters buffer.
     * @param cbParm       Size of the buffer.
     *
     * @note Initialized to NULL when the application USB callbacks are NULL.
     */
    DECLR3CALLBACKMEMBER(void, VRDEUSBRequest,(HVRDESERVER hServer,
                                             uint32_t u32ClientId,
                                             void *pvParm,
                                             uint32_t cbParm));

    /**
     * Called by the application when (VRDE_CLIPBOARD_FUNCTION_*):
     *   - (0) guest announces available clipboard formats;
     *   - (1) guest requests clipboard data;
     *   - (2) guest responds to the client's request for clipboard data.
     *
     * @param hServer     The VRDE server handle.
     * @param u32Function The cause of the call.
     * @param u32Format   Bitmask of announced formats or the format of data.
     * @param pvData      Points to: (1) buffer to be filled with clients data;
     *                               (2) data from the host.
     * @param cbData      Size of 'pvData' buffer in bytes.
     * @param pcbActualRead Size of the copied data in bytes.
     *
     * @note Initialized to NULL when the application clipboard callbacks are NULL.
     */
    DECLR3CALLBACKMEMBER(void, VRDEClipboard,(HVRDESERVER hServer,
                                            uint32_t u32Function,
                                            uint32_t u32Format,
                                            void *pvData,
                                            uint32_t cbData,
                                            uint32_t *pcbActualRead));

    /**
     * Query various information from the VRDE server.
     *
     * @param hServer   The VRDE server handle.
     * @param index     VRDE_QI_* identifier of information to be returned.
     * @param pvBuffer  Address of memory buffer to which the information must be written.
     * @param cbBuffer  Size of the memory buffer in bytes.
     * @param pcbOut    Size in bytes of returned information value.
     *
     * @remark The caller must check the *pcbOut. 0 there means no information was returned.
     *         A value greater than cbBuffer means that information is too big to fit in the
     *         buffer, in that case no information was placed to the buffer.
     */
    DECLR3CALLBACKMEMBER(void, VRDEQueryInfo,(HVRDESERVER hServer,
                                            uint32_t index,
                                            void *pvBuffer,
                                            uint32_t cbBuffer,
                                            uint32_t *pcbOut));

    /**
     * The server should redirect the client to the specified server.
     *
     * @param hServer       The server instance handle.
     * @param u32ClientId   The client identifier.
     * @param pszServer     The server to redirect the client to.
     * @param pszUser       The username to use for the redirection.
     *                      Can be NULL.
     * @param pszDomain     The domain. Can be NULL.
     * @param pszPassword   The password. Can be NULL.
     * @param u32SessionId  The ID of the session to redirect to.
     * @param pszCookie     The routing token used by a load balancer to
     *                      route the redirection. Can be NULL.
     */
    DECLR3CALLBACKMEMBER(void, VRDERedirect,(HVRDESERVER hServer,
                                             uint32_t u32ClientId,
                                             const char *pszServer,
                                             const char *pszUser,
                                             const char *pszDomain,
                                             const char *pszPassword,
                                             uint32_t u32SessionId,
                                             const char *pszCookie));
} VRDEENTRYPOINTS_2;

/** The VRDE server entry points. Interface version 3.
 *  New entry points VRDEAudioInOpen and VRDEAudioInClose has been added relative to version 2.
 */
typedef struct _VRDEENTRYPOINTS_3
{
    /* The header. */
    VRDEINTERFACEHDR header;

    /*
     * Same as version 2. See comment in VRDEENTRYPOINTS_2.
     */

    DECLR3CALLBACKMEMBER(void, VRDEDestroy,(HVRDESERVER hServer));

    DECLR3CALLBACKMEMBER(int, VRDEEnableConnections,(HVRDESERVER hServer,
                                                   bool fEnable));

    DECLR3CALLBACKMEMBER(void, VRDEDisconnect,(HVRDESERVER hServer,
                                             uint32_t u32ClientId,
                                             bool fReconnect));

    DECLR3CALLBACKMEMBER(void, VRDEResize,(HVRDESERVER hServer));

    DECLR3CALLBACKMEMBER(void, VRDEUpdate,(HVRDESERVER hServer,
                                         unsigned uScreenId,
                                         void *pvUpdate,
                                         uint32_t cbUpdate));

    DECLR3CALLBACKMEMBER(void, VRDEColorPointer,(HVRDESERVER hServer,
                                               const VRDECOLORPOINTER *pPointer));

    DECLR3CALLBACKMEMBER(void, VRDEHidePointer,(HVRDESERVER hServer));

    DECLR3CALLBACKMEMBER(void, VRDEAudioSamples,(HVRDESERVER hServer,
                                               const void *pvSamples,
                                               uint32_t cSamples,
                                               VRDEAUDIOFORMAT format));

    DECLR3CALLBACKMEMBER(void, VRDEAudioVolume,(HVRDESERVER hServer,
                                              uint16_t u16Left,
                                              uint16_t u16Right));

    DECLR3CALLBACKMEMBER(void, VRDEUSBRequest,(HVRDESERVER hServer,
                                             uint32_t u32ClientId,
                                             void *pvParm,
                                             uint32_t cbParm));

    DECLR3CALLBACKMEMBER(void, VRDEClipboard,(HVRDESERVER hServer,
                                            uint32_t u32Function,
                                            uint32_t u32Format,
                                            void *pvData,
                                            uint32_t cbData,
                                            uint32_t *pcbActualRead));

    DECLR3CALLBACKMEMBER(void, VRDEQueryInfo,(HVRDESERVER hServer,
                                            uint32_t index,
                                            void *pvBuffer,
                                            uint32_t cbBuffer,
                                            uint32_t *pcbOut));

    DECLR3CALLBACKMEMBER(void, VRDERedirect,(HVRDESERVER hServer,
                                             uint32_t u32ClientId,
                                             const char *pszServer,
                                             const char *pszUser,
                                             const char *pszDomain,
                                             const char *pszPassword,
                                             uint32_t u32SessionId,
                                             const char *pszCookie));

    /*
     * New for version 3.
     */

    /**
     * Audio input open request.
     *
     * @param hServer      Handle of VRDE server instance.
     * @param pvCtx        To be used in VRDECallbackAudioIn.
     * @param u32ClientId  An identifier that allows the server to find the corresponding client.
     * @param audioFormat  Preferred format of audio data.
     * @param u32SamplesPerBlock Preferred number of samples in one block of audio input data.
     *
     * @note Initialized to NULL when the VRDECallbackAudioIn callback is NULL.
     */
    DECLR3CALLBACKMEMBER(void, VRDEAudioInOpen,(HVRDESERVER hServer,
                                                void *pvCtx,
                                                uint32_t u32ClientId,
                                                VRDEAUDIOFORMAT audioFormat,
                                                uint32_t u32SamplesPerBlock));

    /**
     * Audio input close request.
     *
     * @param hServer      Handle of VRDE server instance.
     * @param u32ClientId  An identifier that allows the server to find the corresponding client.
     *
     * @note Initialized to NULL when the VRDECallbackAudioIn callback is NULL.
     */
    DECLR3CALLBACKMEMBER(void, VRDEAudioInClose,(HVRDESERVER hServer,
                                                 uint32_t u32ClientId));
} VRDEENTRYPOINTS_3;


/* Indexes for VRDECallbackProperty.
 * *_QP_* queries a property.
 * *_SP_* sets a property.
 */
#define VRDE_QP_NETWORK_PORT      (1) /* Obsolete. Use VRDE_QP_NETWORK_PORT_RANGE instead. */
#define VRDE_QP_NETWORK_ADDRESS   (2) /* UTF8 string. Host network interface IP address to bind to. */
#define VRDE_QP_NUMBER_MONITORS   (3) /* 32 bit. Number of monitors in the VM. */
#define VRDE_QP_NETWORK_PORT_RANGE (4) /* UTF8 string. List of ports. The server must bind to one of
                                        * free ports from the list. Example: "3000,3010-3012,4000",
                                        * which tells the server to bind to either of ports:
                                        * 3000, 3010, 3011, 3012, 4000.
                                        */
#define VRDE_QP_VIDEO_CHANNEL         (5)
#define VRDE_QP_VIDEO_CHANNEL_QUALITY (6)
#define VRDE_QP_VIDEO_CHANNEL_SUNFLSH (7)
#define VRDE_QP_FEATURE           (8) /* VRDEFEATURE structure. Generic interface to query named VRDE properties. */
#define VRDE_QP_UNIX_SOCKET_PATH  (9) /* Path to a UNIX Socket for incoming connections */

#define VRDE_SP_BASE 0x1000
#define VRDE_SP_NETWORK_BIND_PORT (VRDE_SP_BASE + 1) /* 32 bit. The port number actually used by the server.
                                                      * If VRDECreateServer fails, it should set the port to 0.
                                                      * If VRDECreateServer succeeds, then the port must be set
                                                      * in VRDEEnableConnections to the actually used value.
                                                      * VRDEDestroy must set the port to 0xFFFFFFFF.
                                                      */
#define VRDE_SP_CLIENT_STATUS     (VRDE_SP_BASE + 2) /* UTF8 string. The change of the generic client status:
                                                      * "ATTACH"   - the client is attached;
                                                      * "DETACH"   - the client is detached;
                                                      * "NAME=..." - the client name changes.
                                                      * Can be used for other notifications.
                                                      */

#pragma pack(1)
/* VRDE_QP_FEATURE data. */
typedef struct _VRDEFEATURE
{
    uint32_t u32ClientId;
    char     achInfo[1]; /* UTF8 property input name and output value. */
} VRDEFEATURE;

/* VRDE_SP_CLIENT_STATUS data. */
typedef struct VRDECLIENTSTATUS
{
    uint32_t u32ClientId;
    uint32_t cbStatus;
    char     achStatus[1]; /* UTF8 status string. */
} VRDECLIENTSTATUS;

/* A framebuffer description. */
typedef struct _VRDEFRAMEBUFFERINFO
{
    const uint8_t *pu8Bits;
    int            xOrigin;
    int            yOrigin;
    unsigned       cWidth;
    unsigned       cHeight;
    unsigned       cBitsPerPixel;
    unsigned       cbLine;
} VRDEFRAMEBUFFERINFO;

#define VRDE_INPUT_SCANCODE 0
#define VRDE_INPUT_POINT    1
#define VRDE_INPUT_CAD      2
#define VRDE_INPUT_RESET    3
#define VRDE_INPUT_SYNCH    4

typedef struct _VRDEINPUTSCANCODE
{
    unsigned uScancode;
} VRDEINPUTSCANCODE;

#define VRDE_INPUT_POINT_BUTTON1    0x01
#define VRDE_INPUT_POINT_BUTTON2    0x02
#define VRDE_INPUT_POINT_BUTTON3    0x04
#define VRDE_INPUT_POINT_WHEEL_UP   0x08
#define VRDE_INPUT_POINT_WHEEL_DOWN 0x10

typedef struct _VRDEINPUTPOINT
{
    int x;
    int y;
    unsigned uButtons;
} VRDEINPUTPOINT;

#define VRDE_INPUT_SYNCH_SCROLL   0x01
#define VRDE_INPUT_SYNCH_NUMLOCK  0x02
#define VRDE_INPUT_SYNCH_CAPITAL  0x04

typedef struct _VRDEINPUTSYNCH
{
    unsigned uLockStatus;
} VRDEINPUTSYNCH;
#pragma pack()

/** The VRDE server callbacks. Interface version 1. */
typedef struct _VRDECALLBACKS_1
{
    /** The header. */
    VRDEINTERFACEHDR header;

    /**
     * Query or set various information, on how the VRDE server operates, from or to the application.
     * Queries for properties will always return success, and if the key is not known or has no
     * value associated with it an empty string is returned.
     *
     *
     * @param pvCallback  The callback specific pointer.
     * @param index       VRDE_[Q|S]P_* identifier of information to be returned or set.
     * @param pvBuffer    Address of memory buffer to which the information must be written or read.
     * @param cbBuffer    Size of the memory buffer in bytes.
     * @param pcbOut      Size in bytes of returned information value.
     *
     * @return IPRT status code. VINF_BUFFER_OVERFLOW if the buffer is too small for the value.
     */
    DECLR3CALLBACKMEMBER(int, VRDECallbackProperty,(void *pvCallback,
                                                    uint32_t index,
                                                    void *pvBuffer,
                                                    uint32_t cbBuffer,
                                                    uint32_t *pcbOut));

    /* A client is logging in, the application must decide whether
     * to let to connect the client. The server will drop the connection,
     * when an error code is returned by the callback.
     *
     * @param pvCallback   The callback specific pointer.
     * @param u32ClientId  An unique client identifier generated by the server.
     * @param pszUser      The username.
     * @param pszPassword  The password.
     * @param pszDomain    The domain.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDECallbackClientLogon,(void *pvCallback,
                                                     uint32_t u32ClientId,
                                                     const char *pszUser,
                                                     const char *pszPassword,
                                                     const char *pszDomain));

    /* The client has been successfully connected. That is logon was successful and the
     * remote desktop protocol connection completely established.
     *
     * @param pvCallback      The callback specific pointer.
     * @param u32ClientId     An unique client identifier generated by the server.
     */
    DECLR3CALLBACKMEMBER(void, VRDECallbackClientConnect,(void *pvCallback,
                                                        uint32_t u32ClientId));

    /* The client has been disconnected.
     *
     * @param pvCallback      The callback specific pointer.
     * @param u32ClientId     An unique client identifier generated by the server.
     * @param fu32Intercepted What was intercepted by the client (VRDE_CLIENT_INTERCEPT_*).
     */
    DECLR3CALLBACKMEMBER(void, VRDECallbackClientDisconnect,(void *pvCallback,
                                                           uint32_t u32ClientId,
                                                           uint32_t fu32Intercepted));
    /* The client supports one of RDP channels.
     *
     * @param pvCallback      The callback specific pointer.
     * @param u32ClientId     An unique client identifier generated by the server.
     * @param fu32Intercept   What the client wants to intercept. One of VRDE_CLIENT_INTERCEPT_* flags.
     * @param ppvIntercept    The value to be passed to the channel specific callback.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDECallbackIntercept,(void *pvCallback,
                                                   uint32_t u32ClientId,
                                                   uint32_t fu32Intercept,
                                                   void **ppvIntercept));

    /**
     * Called by the server when a reply is received from a client.
     *
     * @param pvCallback   The callback specific pointer.
     * @param ppvIntercept The value returned by VRDECallbackIntercept for the VRDE_CLIENT_INTERCEPT_USB.
     * @param u32ClientId  Identifies the client that sent the reply.
     * @param u8Code       The operation code VRDE_USB_REQ_*.
     * @param pvRet        Points to data received from the client.
     * @param cbRet        Size of the data in bytes.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDECallbackUSB,(void *pvCallback,
                                             void *pvIntercept,
                                             uint32_t u32ClientId,
                                             uint8_t u8Code,
                                             const void *pvRet,
                                             uint32_t cbRet));

    /**
     * Called by the server when (VRDE_CLIPBOARD_FUNCTION_*):
     *   - (0) client announces available clipboard formats;
     *   - (1) client requests clipboard data.
     *
     * @param pvCallback   The callback specific pointer.
     * @param ppvIntercept The value returned by VRDECallbackIntercept for the VRDE_CLIENT_INTERCEPT_CLIPBOARD.
     * @param u32ClientId Identifies the RDP client that sent the reply.
     * @param u32Function The cause of the callback.
     * @param u32Format   Bitmask of reported formats or the format of received data.
     * @param pvData      Reserved.
     * @param cbData      Reserved.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDECallbackClipboard,(void *pvCallback,
                                                   void *pvIntercept,
                                                   uint32_t u32ClientId,
                                                   uint32_t u32Function,
                                                   uint32_t u32Format,
                                                   const void *pvData,
                                                   uint32_t cbData));

    /* The framebuffer information is queried.
     *
     * @param pvCallback      The callback specific pointer.
     * @param uScreenId       The framebuffer index.
     * @param pInfo           The information structure to ber filled.
     *
     * @return Whether the framebuffer is available.
     */
    DECLR3CALLBACKMEMBER(bool, VRDECallbackFramebufferQuery,(void *pvCallback,
                                                           unsigned uScreenId,
                                                           VRDEFRAMEBUFFERINFO *pInfo));

    /* Request the exclusive access to the framebuffer bitmap.
     * Currently not used because VirtualBox makes sure that the framebuffer is available
     * when VRDEUpdate is called.
     *
     * @param pvCallback      The callback specific pointer.
     * @param uScreenId       The framebuffer index.
     */
    DECLR3CALLBACKMEMBER(void, VRDECallbackFramebufferLock,(void *pvCallback,
                                                          unsigned uScreenId));

    /* Release the exclusive access to the framebuffer bitmap.
     * Currently not used because VirtualBox makes sure that the framebuffer is available
     * when VRDEUpdate is called.
     *
     * @param pvCallback      The callback specific pointer.
     * @param uScreenId       The framebuffer index.
     */
    DECLR3CALLBACKMEMBER(void, VRDECallbackFramebufferUnlock,(void *pvCallback,
                                                            unsigned uScreenId));

    /* Input from the client.
     *
     * @param pvCallback      The callback specific pointer.
     * @param pvInput         The input information.
     * @param cbInput         The size of the input information.
     */
    DECLR3CALLBACKMEMBER(void, VRDECallbackInput,(void *pvCallback,
                                                int type,
                                                const void *pvInput,
                                                unsigned cbInput));

    /* Video mode hint from the client.
     *
     * @param pvCallback      The callback specific pointer.
     * @param cWidth          Requested width.
     * @param cHeight         Requested height.
     * @param cBitsPerPixel   Requested color depth.
     * @param uScreenId       The framebuffer index.
     */
    DECLR3CALLBACKMEMBER(void, VRDECallbackVideoModeHint,(void *pvCallback,
                                                        unsigned cWidth,
                                                        unsigned cHeight,
                                                        unsigned cBitsPerPixel,
                                                        unsigned uScreenId));

} VRDECALLBACKS_1;

/* Callbacks are the same for the version 1 and version 2 interfaces. */
typedef VRDECALLBACKS_1 VRDECALLBACKS_2;

/** The VRDE server callbacks. Interface version 3. */
typedef struct _VRDECALLBACKS_3
{
    /* The header. */
    VRDEINTERFACEHDR header;

    /*
     * Same as in version 1 and 2. See comment in VRDECALLBACKS_1.
     */
    DECLR3CALLBACKMEMBER(int, VRDECallbackProperty,(void *pvCallback,
                                                    uint32_t index,
                                                    void *pvBuffer,
                                                    uint32_t cbBuffer,
                                                    uint32_t *pcbOut));

    DECLR3CALLBACKMEMBER(int, VRDECallbackClientLogon,(void *pvCallback,
                                                     uint32_t u32ClientId,
                                                     const char *pszUser,
                                                     const char *pszPassword,
                                                     const char *pszDomain));

    DECLR3CALLBACKMEMBER(void, VRDECallbackClientConnect,(void *pvCallback,
                                                        uint32_t u32ClientId));

    DECLR3CALLBACKMEMBER(void, VRDECallbackClientDisconnect,(void *pvCallback,
                                                           uint32_t u32ClientId,
                                                           uint32_t fu32Intercepted));
    DECLR3CALLBACKMEMBER(int, VRDECallbackIntercept,(void *pvCallback,
                                                   uint32_t u32ClientId,
                                                   uint32_t fu32Intercept,
                                                   void **ppvIntercept));

    DECLR3CALLBACKMEMBER(int, VRDECallbackUSB,(void *pvCallback,
                                             void *pvIntercept,
                                             uint32_t u32ClientId,
                                             uint8_t u8Code,
                                             const void *pvRet,
                                             uint32_t cbRet));

    DECLR3CALLBACKMEMBER(int, VRDECallbackClipboard,(void *pvCallback,
                                                   void *pvIntercept,
                                                   uint32_t u32ClientId,
                                                   uint32_t u32Function,
                                                   uint32_t u32Format,
                                                   const void *pvData,
                                                   uint32_t cbData));

    DECLR3CALLBACKMEMBER(bool, VRDECallbackFramebufferQuery,(void *pvCallback,
                                                           unsigned uScreenId,
                                                           VRDEFRAMEBUFFERINFO *pInfo));

    DECLR3CALLBACKMEMBER(void, VRDECallbackFramebufferLock,(void *pvCallback,
                                                          unsigned uScreenId));

    DECLR3CALLBACKMEMBER(void, VRDECallbackFramebufferUnlock,(void *pvCallback,
                                                            unsigned uScreenId));

    DECLR3CALLBACKMEMBER(void, VRDECallbackInput,(void *pvCallback,
                                                int type,
                                                const void *pvInput,
                                                unsigned cbInput));

    DECLR3CALLBACKMEMBER(void, VRDECallbackVideoModeHint,(void *pvCallback,
                                                        unsigned cWidth,
                                                        unsigned cHeight,
                                                        unsigned cBitsPerPixel,
                                                        unsigned uScreenId));

    /*
     * New for version 3.
     */

    /**
     * Called by the server when something happens with audio input.
     *
     * @param pvCallback   The callback specific pointer.
     * @param pvCtx        The value passed in VRDEAudioInOpen.
     * @param u32ClientId  Identifies the client that sent the reply.
     * @param u32Event     The event code VRDE_AUDIOIN_*.
     * @param pvData       Points to data received from the client.
     * @param cbData       Size of the data in bytes.
     */
    DECLR3CALLBACKMEMBER(void, VRDECallbackAudioIn,(void *pvCallback,
                                                    void *pvCtx,
                                                    uint32_t u32ClientId,
                                                    uint32_t u32Event,
                                                    const void *pvData,
                                                    uint32_t cbData));
} VRDECALLBACKS_3;

/** The VRDE server entry points. Interface version 4.
 *  New entry point VRDEGetInterface has been added relative to version 3.
 */
typedef struct _VRDEENTRYPOINTS_4
{
    /* The header. */
    VRDEINTERFACEHDR header;

    /*
     * Same as version 3. See comment in VRDEENTRYPOINTS_3.
     */

    DECLR3CALLBACKMEMBER(void, VRDEDestroy,(HVRDESERVER hServer));
    DECLR3CALLBACKMEMBER(int,  VRDEEnableConnections,(HVRDESERVER hServer, bool fEnable));
    DECLR3CALLBACKMEMBER(void, VRDEDisconnect,(HVRDESERVER hServer, uint32_t u32ClientId, bool fReconnect));
    DECLR3CALLBACKMEMBER(void, VRDEResize,(HVRDESERVER hServer));
    DECLR3CALLBACKMEMBER(void, VRDEUpdate,(HVRDESERVER hServer, unsigned uScreenId, void *pvUpdate,
                                           uint32_t cbUpdate));
    DECLR3CALLBACKMEMBER(void, VRDEColorPointer,(HVRDESERVER hServer, const VRDECOLORPOINTER *pPointer));
    DECLR3CALLBACKMEMBER(void, VRDEHidePointer,(HVRDESERVER hServer));
    DECLR3CALLBACKMEMBER(void, VRDEAudioSamples,(HVRDESERVER hServer, const void *pvSamples, uint32_t cSamples,
                                                 VRDEAUDIOFORMAT format));
    DECLR3CALLBACKMEMBER(void, VRDEAudioVolume,(HVRDESERVER hServer, uint16_t u16Left, uint16_t u16Right));
    DECLR3CALLBACKMEMBER(void, VRDEUSBRequest,(HVRDESERVER hServer, uint32_t u32ClientId, void *pvParm,
                                               uint32_t cbParm));
    DECLR3CALLBACKMEMBER(void, VRDEClipboard,(HVRDESERVER hServer, uint32_t u32Function, uint32_t u32Format,
                                              void *pvData, uint32_t cbData, uint32_t *pcbActualRead));
    DECLR3CALLBACKMEMBER(void, VRDEQueryInfo,(HVRDESERVER hServer, uint32_t index, void *pvBuffer, uint32_t cbBuffer,
                                              uint32_t *pcbOut));
    DECLR3CALLBACKMEMBER(void, VRDERedirect,(HVRDESERVER hServer, uint32_t u32ClientId, const char *pszServer,
                                             const char *pszUser, const char *pszDomain, const char *pszPassword,
                                             uint32_t u32SessionId, const char *pszCookie));
    DECLR3CALLBACKMEMBER(void, VRDEAudioInOpen,(HVRDESERVER hServer, void *pvCtx, uint32_t u32ClientId,
                                                VRDEAUDIOFORMAT audioFormat, uint32_t u32SamplesPerBlock));
    DECLR3CALLBACKMEMBER(void, VRDEAudioInClose,(HVRDESERVER hServer, uint32_t u32ClientId));

    /**
     * Generic interface query. An interface is a set of entry points and callbacks.
     * It is not a reference counted interface.
     *
     * @param hServer    Handle of VRDE server instance.
     * @param pszId      String identifier of the interface, like uuid.
     * @param pInterface The interface structure to be initialized by the VRDE server.
     *                   Only VRDEINTERFACEHDR is initialized by the caller.
     * @param pCallbacks Callbacks required by the interface. The server makes a local copy.
     *                   VRDEINTERFACEHDR version must correspond to the requested interface version.
     * @param pvContext  The context to be used in callbacks.
     */

    DECLR3CALLBACKMEMBER(int, VRDEGetInterface, (HVRDESERVER hServer,
                                                 const char *pszId,
                                                 VRDEINTERFACEHDR *pInterface,
                                                 const VRDEINTERFACEHDR *pCallbacks,
                                                 void *pvContext));
} VRDEENTRYPOINTS_4;

/* Callbacks are the same for the version 3 and version 4 interfaces. */
typedef VRDECALLBACKS_3 VRDECALLBACKS_4;

/**
 * Create a new VRDE server instance. The instance is fully functional but refuses
 * client connections until the entry point VRDEEnableConnections is called by the application.
 *
 * The caller prepares the VRDECALLBACKS_* structure. The header.u64Version field of the
 * structure must be initialized with the version of the interface to use.
 * The server will return pointer to VRDEENTRYPOINTS_* table in *ppEntryPoints
 * to match the requested interface.
 * That is if pCallbacks->header.u64Version == VRDE_INTERFACE_VERSION_1, then the server
 * expects pCallbacks to point to VRDECALLBACKS_1 and will return a pointer to VRDEENTRYPOINTS_1.
 *
 * @param pCallback     Pointer to the application callbacks which let the server to fetch
 *                      the configuration data and to access the desktop.
 * @param pvCallback    The callback specific pointer to be passed back to the application.
 * @param ppEntryPoints Where to store the pointer to the VRDE entry points structure.
 * @param phServer      Pointer to the created server instance handle.
 *
 * @return IPRT status code.
 */
DECLEXPORT(int) VRDECreateServer (const VRDEINTERFACEHDR *pCallbacks,
                                  void *pvCallback,
                                  VRDEINTERFACEHDR **ppEntryPoints,
                                  HVRDESERVER *phServer);

typedef DECLCALLBACKTYPE(int, FNVRDECREATESERVER,(const VRDEINTERFACEHDR *pCallbacks,
                                                  void *pvCallback,
                                                  VRDEINTERFACEHDR **ppEntryPoints,
                                                  HVRDESERVER *phServer));
typedef FNVRDECREATESERVER *PFNVRDECREATESERVER;

/**
 * List of names of the VRDE properties, which are recognized by the VRDE.
 *
 * For example VRDESupportedProperties should return gapszProperties declared as:
 *
 * static const char * const gapszProperties[] =
 * {
 *   "TCP/Ports",
 *   "TCP/Address",
 *   NULL
 * };
 *
 * @returns pointer to array of pointers to name strings (UTF8).
 */
DECLEXPORT(const char * const *) VRDESupportedProperties (void);

typedef DECLCALLBACKTYPE(const char * const *, FNVRDESUPPORTEDPROPERTIES,(void));
typedef FNVRDESUPPORTEDPROPERTIES *PFNVRDESUPPORTEDPROPERTIES;

RT_C_DECLS_END

/** @} */

#endif /* !VBOX_INCLUDED_RemoteDesktop_VRDE_h */
