/** @file
 * USBLib - Library for wrapping up the VBoxUSB functionality, Solaris flavor.
 * (DEV,HDrv,Main)
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

#ifndef VBOX_INCLUDED_usblib_solaris_h
#define VBOX_INCLUDED_usblib_solaris_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/cdefs.h>
#include <VBox/usbfilter.h>
#include <VBox/vusb.h>
#include <sys/types.h>
#include <sys/ioccom.h>
#include <sys/param.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_usblib_solaris    Solaris USB Specifics
 * @ingroup grp_usblib
 * @{
 */

/** @name VBoxUSB specific IOCtls.
 * VBoxUSB uses them for resetting USB devices requests from userland.
 * USBProxyService/Device makes use of them to communicate with VBoxUSB.
 * @{ */

/** Ring-3 request wrapper for big requests.
 *
 * This is necessary because the ioctl number scheme on many Unixy OSes (esp. Solaris)
 * only allows a relatively small size to be encoded into the request. So, for big
 * request this generic form is used instead. */
typedef struct VBOXUSBREQ
{
    /** Magic value (VBOXUSB(MON)_MAGIC). */
    uint32_t    u32Magic;
    /** The size of the data buffer (In & Out). */
    uint32_t    cbData;
    /** Result code of the request filled by driver. */
    int32_t     rc;
    /** The user address of the data buffer. */
    RTR3PTR     pvDataR3;
} VBOXUSBREQ;
/** Pointer to a request wrapper for solaris. */
typedef VBOXUSBREQ *PVBOXUSBREQ;
/** Pointer to a const request wrapper for solaris. */
typedef const VBOXUSBREQ *PCVBOXUSBREQ;

#pragma pack(1)
typedef struct
{
    /* Pointer to the Filter. */
    USBFILTER      Filter;
    /* Where to store the added Filter (Id). */
    uintptr_t      uId;
} VBOXUSBREQ_ADD_FILTER;

typedef struct
{
    /* Pointer to Filter (Id) to be removed. */
    uintptr_t      uId;
} VBOXUSBREQ_REMOVE_FILTER;

typedef struct
{
    /** Whether to re-attach the driver. */
    bool           fReattach;
    /* Physical path of the USB device. */
    char           szDevicePath[1];
} VBOXUSBREQ_RESET_DEVICE;

typedef struct
{
    /* Where to store the instance. */
    int           *pInstance;
    /* Physical path of the USB device. */
    char           szDevicePath[1];
} VBOXUSBREQ_DEVICE_INSTANCE;

typedef struct
{
    /** Where to store the instance. */
    int            Instance;
    /* Where to store the client path. */
    char           szClientPath[MAXPATHLEN];
    /** Device identifier (VendorId:ProductId:Release:StaticPath) */
    char           szDeviceIdent[MAXPATHLEN+48];
    /** Callback from monitor specifying client consumer (VM) credentials */
    DECLR0CALLBACKMEMBER(int, pfnSetConsumerCredentials,(RTPROCESS Process, int Instance, void *pvReserved));
} VBOXUSBREQ_CLIENT_INFO, *PVBOXUSBREQ_CLIENT_INFO;
typedef VBOXUSBREQ_CLIENT_INFO VBOXUSB_CLIENT_INFO;
typedef PVBOXUSBREQ_CLIENT_INFO PVBOXUSB_CLIENT_INFO;

/** Isoc packet descriptor (Must mirror exactly Solaris USBA's usb_isoc_pkt_descr_t) */
typedef struct
{
    ushort_t                cbPkt;              /* Size of the packet */
    ushort_t                cbActPkt;           /* Size of the packet actually transferred */
    VUSBSTATUS              enmStatus;          /* Per frame transfer status */
} VUSBISOC_PKT_DESC;

/** VBoxUSB IOCtls */
typedef struct
{
    void                   *pvUrbR3;            /* Pointer to userland URB (untouched by kernel driver) */
    uint8_t                 bEndpoint;          /* Endpoint address */
    VUSBXFERTYPE            enmType;            /* Xfer type */
    VUSBDIRECTION           enmDir;             /* Xfer direction */
    VUSBSTATUS              enmStatus;          /* URB status */
    bool                    fShortOk;           /* Whether receiving less data than requested is acceptable. */
    size_t                  cbData;             /* Size of the data */
    void                   *pvData;             /* Pointer to the data */
    uint32_t                cIsocPkts;          /* Number of Isoc packets */
    VUSBISOC_PKT_DESC       aIsocPkts[8];       /* Array of Isoc packet descriptors */
} VBOXUSBREQ_URB, *PVBOXUSBREQ_URB;

typedef struct
{
    uint8_t                 bEndpoint;          /* Endpoint address */
} VBOXUSBREQ_CLEAR_EP, *PVBOXUSBREQ_CLEAR_EP;


typedef struct
{
    uint8_t                 bConfigValue;       /* Configuration value */
} VBOXUSBREQ_SET_CONFIG, *PVBOXUSBREQ_SET_CONFIG;
typedef VBOXUSBREQ_SET_CONFIG  VBOXUSBREQ_GET_CONFIG;
typedef PVBOXUSBREQ_SET_CONFIG PVBOXUSBREQ_GET_CONFIG;

typedef struct
{
    uint8_t                 bInterface;         /* Interface number */
    uint8_t                 bAlternate;         /* Alternate setting */
} VBOXUSBREQ_SET_INTERFACE, *PVBOXUSBREQ_SET_INTERFACE;

typedef enum
{
    /** Close device not a reset. */
    VBOXUSB_RESET_LEVEL_CLOSE     = 0,
    /** Hard reset resulting in device replug behaviour. */
    VBOXUSB_RESET_LEVEL_REATTACH  = 2,
    /** Device-level reset. */
    VBOXUSB_RESET_LEVEL_SOFT      = 4
} VBOXUSB_RESET_LEVEL;

typedef struct
{
    VBOXUSB_RESET_LEVEL     ResetLevel;         /* Reset level after closing */
} VBOXUSBREQ_CLOSE_DEVICE, *PVBOXUSBREQ_CLOSE_DEVICE;

typedef struct
{
    uint8_t                 bEndpoint;          /* Endpoint address */
} VBOXUSBREQ_ABORT_PIPE, *PVBOXUSBREQ_ABORT_PIPE;

typedef struct
{
    uint32_t                u32Major;           /* Driver major number */
    uint32_t                u32Minor;           /* Driver minor number */
} VBOXUSBREQ_GET_VERSION, *PVBOXUSBREQ_GET_VERSION;

#pragma pack()

/** The VBOXUSBREQ::u32Magic value for VBoxUSBMon. */
#define VBOXUSBMON_MAGIC           0xba5eba11
/** The VBOXUSBREQ::u32Magic value for VBoxUSB.*/
#define VBOXUSB_MAGIC              0x601fba11
/** The USBLib entry point for userland. */
#define VBOXUSB_DEVICE_NAME        "/dev/vboxusbmon"

/** The USBMonitor Major version. */
#define VBOXUSBMON_VERSION_MAJOR   2
/** The USBMonitor Minor version. */
#define VBOXUSBMON_VERSION_MINOR   1

/** The USB Major version. */
#define VBOXUSB_VERSION_MAJOR      1
/** The USB Minor version. */
#define VBOXUSB_VERSION_MINOR      1

#ifdef RT_ARCH_AMD64
# define VBOXUSB_IOCTL_FLAG     128
#elif defined(RT_ARCH_X86)
# define VBOXUSB_IOCTL_FLAG     0
#else
# error "dunno which arch this is!"
#endif

/** USB driver name*/
#define VBOXUSB_DRIVER_NAME     "vboxusb"

/* No automatic buffering, size limited to 255 bytes => use VBOXUSBREQ for everything. */
#define VBOXUSB_IOCTL_CODE(Function, Size)  _IOWRN('V', (Function) | VBOXUSB_IOCTL_FLAG, sizeof(VBOXUSBREQ))
#define VBOXUSB_IOCTL_CODE_FAST(Function)   _IO(   'V', (Function) | VBOXUSB_IOCTL_FLAG)
#define VBOXUSB_IOCTL_STRIP_SIZE(Code)      (Code)

#define VBOXUSBMON_IOCTL_ADD_FILTER         VBOXUSB_IOCTL_CODE(1, (sizeof(VBoxUSBAddFilterReq)))
#define VBOXUSBMON_IOCTL_REMOVE_FILTER      VBOXUSB_IOCTL_CODE(2, (sizeof(VBoxUSBRemoveFilterReq)))
#define VBOXUSBMON_IOCTL_RESET_DEVICE       VBOXUSB_IOCTL_CODE(3, (sizeof(VBOXUSBREQ_RESET_DEVICE)))
#define VBOXUSBMON_IOCTL_DEVICE_INSTANCE    VBOXUSB_IOCTL_CODE(4, (sizeof(VBOXUSBREQ_DEVICE_INSTANCE)))
#define VBOXUSBMON_IOCTL_CLIENT_INFO        VBOXUSB_IOCTL_CODE(5, (sizeof(VBOXUSBREQ_CLIENT_PATH)))
#define VBOXUSBMON_IOCTL_GET_VERSION        VBOXUSB_IOCTL_CODE(6, (sizeof(VBOXUSBREQ_GET_VERSION)))

/* VBoxUSB ioctls */
#define VBOXUSB_IOCTL_SEND_URB              VBOXUSB_IOCTL_CODE(20, (sizeof(VBOXUSBREQ_URB)))            /* 1072146796 */
#define VBOXUSB_IOCTL_REAP_URB              VBOXUSB_IOCTL_CODE(21, (sizeof(VBOXUSBREQ_URB)))            /* 1072146795 */
#define VBOXUSB_IOCTL_CLEAR_EP              VBOXUSB_IOCTL_CODE(22, (sizeof(VBOXUSBREQ_CLEAR_EP)))       /* 1072146794 */
#define VBOXUSB_IOCTL_SET_CONFIG            VBOXUSB_IOCTL_CODE(23, (sizeof(VBOXUSBREQ_SET_CONFIG)))     /* 1072146793 */
#define VBOXUSB_IOCTL_SET_INTERFACE         VBOXUSB_IOCTL_CODE(24, (sizeof(VBOXUSBREQ_SET_INTERFACE)))  /* 1072146792 */
#define VBOXUSB_IOCTL_CLOSE_DEVICE          VBOXUSB_IOCTL_CODE(25, (sizeof(VBOXUSBREQ_CLOSE_DEVICE)))   /* 1072146791 0xc0185699 */
#define VBOXUSB_IOCTL_ABORT_PIPE            VBOXUSB_IOCTL_CODE(26, (sizeof(VBOXUSBREQ_ABORT_PIPE)))     /* 1072146790 */
#define VBOXUSB_IOCTL_GET_CONFIG            VBOXUSB_IOCTL_CODE(27, (sizeof(VBOXUSBREQ_GET_CONFIG)))     /* 1072146789 */
#define VBOXUSB_IOCTL_GET_VERSION           VBOXUSB_IOCTL_CODE(28, (sizeof(VBOXUSBREQ_GET_VERSION)))    /* 1072146788 */

/** @} */

/* USBLibHelper data for resetting the device. */
typedef struct VBOXUSBHELPERDATA_RESET
{
    /** Path of the USB device. */
    const char  *pszDevicePath;
    /** Re-enumerate or not. */
    bool        fHardReset;
} VBOXUSBHELPERDATA_RESET;
typedef VBOXUSBHELPERDATA_RESET *PVBOXUSBHELPERDATA_RESET;
typedef const VBOXUSBHELPERDATA_RESET *PCVBOXUSBHELPERDATA_RESET;

/* USBLibHelper data for device hijacking. */
typedef struct VBOXUSBHELPERDATA_ALIAS
{
    /** Vendor ID. */
    uint16_t        idVendor;
    /** Product ID. */
    uint16_t        idProduct;
    /** Revision, integer part. */
    uint16_t        bcdDevice;
    /** Path of the USB device. */
    const char      *pszDevicePath;
} VBOXUSBHELPERDATA_ALIAS;
typedef VBOXUSBHELPERDATA_ALIAS *PVBOXUSBHELPERDATA_ALIAS;
typedef const VBOXUSBHELPERDATA_ALIAS *PCVBOXUSBHELPERDATA_ALIAS;

USBLIB_DECL(int) USBLibResetDevice(char *pszDevicePath, bool fReattach);
USBLIB_DECL(int) USBLibDeviceInstance(char *pszDevicePath, int *pInstance);
USBLIB_DECL(int) USBLibGetClientInfo(char *pszDeviceIdent, char **ppszClientPath, int *pInstance);
USBLIB_DECL(int) USBLibAddDeviceAlias(PUSBDEVICE pDevice);
USBLIB_DECL(int) USBLibRemoveDeviceAlias(PUSBDEVICE pDevice);
/*USBLIB_DECL(int) USBLibConfigureDevice(PUSBDEVICE pDevice);*/

/** @} */
RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_usblib_solaris_h */

