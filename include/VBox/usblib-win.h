/** @file
 * USBLib - Library for wrapping up the VBoxUSB functionality, Windows flavor.
 * (DEV,HDrv,Main)
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

#ifndef VBOX_INCLUDED_usblib_win_h
#define VBOX_INCLUDED_usblib_win_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/usb.h>

#include <initguid.h>


/** @defgroup grp_usblib_win    Windows USB Specifics
 * @ingroup grp_usblib
 * @{
 */

// {6068EB61-98E7-4c98-9E20-1F068295909A}
DEFINE_GUID(GUID_CLASS_VBOXUSB, 0x873fdf, 0xCAFE, 0x80EE, 0xaa, 0x5e, 0x0, 0xc0, 0x4f, 0xb1, 0x72, 0xb);

#define USBFLT_SERVICE_NAME              "\\\\.\\VBoxUSBFlt"
#define USBFLT_NTDEVICE_NAME_STRING      L"\\Device\\VBoxUSBFlt"
#define USBFLT_SYMBOLIC_NAME_STRING      L"\\DosDevices\\VBoxUSBFlt"

#define USBMON_SERVICE_NAME_W              L"VBoxUSBMon"
#define USBMON_DEVICE_NAME               "\\\\.\\VBoxUSBMon"
#define USBMON_DEVICE_NAME_NT            L"\\Device\\VBoxUSBMon"
#define USBMON_DEVICE_NAME_DOS           L"\\DosDevices\\VBoxUSBMon"

/*
 * IOCtl numbers.
 * We're using the Win32 type of numbers here, thus the macros below.
 */

#ifndef CTL_CODE
# if defined(RT_OS_WINDOWS)
#  define CTL_CODE(DeviceType, Function, Method, Access) \
    ( ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#else /* unix: */
#  define CTL_CODE(DeviceType, Function, Method_ignored, Access_ignored) \
    ( (3 << 30) | ((DeviceType) << 8) | (Function) | (sizeof(SUPDRVIOCTLDATA) << 16) )
# endif
#endif
#ifndef METHOD_BUFFERED
# define METHOD_BUFFERED        0
#endif
#ifndef FILE_WRITE_ACCESS
# define FILE_WRITE_ACCESS      0x0002
#endif
#ifndef FILE_DEVICE_UNKNOWN
# define FILE_DEVICE_UNKNOWN    0x00000022
#endif

#define USBMON_MAJOR_VERSION              5
#define USBMON_MINOR_VERSION              0

#define USBDRV_MAJOR_VERSION              5
#define USBDRV_MINOR_VERSION              0

#define SUPUSB_IOCTL_TEST                 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x601, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSB_IOCTL_GET_DEVICE           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x603, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSB_IOCTL_SEND_URB             CTL_CODE(FILE_DEVICE_UNKNOWN, 0x607, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSB_IOCTL_USB_RESET            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x608, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSB_IOCTL_USB_SELECT_INTERFACE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x609, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSB_IOCTL_USB_SET_CONFIG       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x60A, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSB_IOCTL_USB_CLAIM_DEVICE     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x60B, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSB_IOCTL_USB_RELEASE_DEVICE   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x60C, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSB_IOCTL_IS_OPERATIONAL       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x60D, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSB_IOCTL_USB_CLEAR_ENDPOINT   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x60E, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSB_IOCTL_GET_VERSION          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x60F, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSB_IOCTL_USB_ABORT_ENDPOINT   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x610, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define SUPUSBFLT_IOCTL_GET_NUM_DEVICES   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x602, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSBFLT_IOCTL_USB_CHANGE        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x604, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSBFLT_IOCTL_DISABLE_CAPTURE   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x605, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSBFLT_IOCTL_ENABLE_CAPTURE    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x606, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSBFLT_IOCTL_IGNORE_DEVICE     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x60F, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSBFLT_IOCTL_GET_VERSION       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x610, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSBFLT_IOCTL_ADD_FILTER        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x611, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSBFLT_IOCTL_REMOVE_FILTER     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x612, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSBFLT_IOCTL_CAPTURE_DEVICE    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x613, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSBFLT_IOCTL_RELEASE_DEVICE    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x614, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define SUPUSBFLT_IOCTL_RUN_FILTERS       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x615, METHOD_BUFFERED, FILE_WRITE_ACCESS)
/* Used to be SUPUSBFLT_IOCTL_SET_NOTIFY_EVENT, 0x616 */
#define SUPUSBFLT_IOCTL_GET_DEVICE        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x617, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#pragma pack(4)

#define MAX_FILTER_NAME                 128
#define MAX_USB_SERIAL_STRING           64

/* a user-mode handle that could be used for retriving device information
 * from the monitor driver */
typedef void* HVBOXUSBDEVUSR;

typedef struct
{
    HVBOXUSBDEVUSR  hDevice;
} USBSUP_GETDEV, *PUSBSUP_GETDEV;

typedef struct
{
    USBDEVICESTATE enmState;
} USBSUP_GETDEV_MON, *PUSBSUP_GETDEV_MON;

typedef struct
{
    uint32_t        u32Major;
    uint32_t        u32Minor;
} USBSUP_VERSION, *PUSBSUP_VERSION;


typedef struct USBSUP_FLTADDOUT
{
    uintptr_t       uId;    /* The ID. */
    int             rc;     /* The return code. */
} USBSUP_FLTADDOUT, *PUSBSUP_FLTADDOUT;

typedef struct
{
    uint16_t        usVendorId;
    uint16_t        usProductId;
    uint16_t        usRevision;
} USBSUP_CAPTURE, *PUSBSUP_CAPTURE;

typedef USBSUP_CAPTURE      USBSUP_RELEASE;
typedef PUSBSUP_CAPTURE     PUSBSUP_RELEASE;

typedef struct
{
    uint8_t         bInterfaceNumber;
    uint8_t         fClaimed;
} USBSUP_CLAIMDEV, *PUSBSUP_CLAIMDEV;

typedef USBSUP_CLAIMDEV  USBSUP_RELEASEDEV;
typedef PUSBSUP_CLAIMDEV PUSBSUP_RELEASEDEV;

typedef struct
{
    uint32_t         cUSBDevices;
} USBSUP_GETNUMDEV, *PUSBSUP_GETNUMDEV;

typedef struct
{
    uint8_t          fUSBChange;
    uint32_t         cUSBStateChange;
} USBSUP_USB_CHANGE, *PUSBSUP_USB_CHANGE;

typedef struct
{
    uint8_t         bConfigurationValue;
} USBSUP_SET_CONFIG, *PUSBSUP_SET_CONFIG;

typedef struct
{
    uint8_t         bInterfaceNumber;
    uint8_t         bAlternateSetting;
} USBSUP_SELECT_INTERFACE, *PUSBSUP_SELECT_INTERFACE;

typedef struct
{
    uint8_t         bEndpoint;
} USBSUP_CLEAR_ENDPOINT, *PUSBSUP_CLEAR_ENDPOINT;

typedef enum
{
    USBSUP_TRANSFER_TYPE_CTRL = 0,
    USBSUP_TRANSFER_TYPE_ISOC = 1,
    USBSUP_TRANSFER_TYPE_BULK = 2,
    USBSUP_TRANSFER_TYPE_INTR = 3,
    USBSUP_TRANSFER_TYPE_MSG  = 4
} USBSUP_TRANSFER_TYPE;

typedef enum
{
    USBSUP_DIRECTION_SETUP = 0,
    USBSUP_DIRECTION_IN    = 1,
    USBSUP_DIRECTION_OUT   = 2
} USBSUP_DIRECTION;

typedef enum
{
    USBSUP_FLAG_NONE       = 0,
    USBSUP_FLAG_SHORT_OK   = 1
} USBSUP_XFER_FLAG;

typedef enum
{
    USBSUP_XFER_OK         = 0,
    USBSUP_XFER_STALL      = 1,
    USBSUP_XFER_DNR        = 2,
    USBSUP_XFER_CRC        = 3,
    USBSUP_XFER_NAC        = 4,
    USBSUP_XFER_UNDERRUN   = 5,
    USBSUP_XFER_OVERRUN    = 6
} USBSUP_ERROR;

typedef struct USBSUP_ISOCPKT
{
    uint16_t        cb;     /* [in/out] packet size/size transferred */
    uint16_t        off;    /* [in] offset of packet in buffer */
    USBSUP_ERROR    stat;   /* [out] packet status */
} USBSUP_ISOCPKT;

typedef struct
{
    USBSUP_TRANSFER_TYPE    type;           /* [in] USBSUP_TRANSFER_TYPE_XXX */
    uint32_t                ep;             /* [in] index to dev->pipe */
    USBSUP_DIRECTION        dir;            /* [in] USBSUP_DIRECTION_XXX */
    USBSUP_XFER_FLAG        flags;          /* [in] USBSUP_FLAG_XXX */
    USBSUP_ERROR            error;          /* [out] USBSUP_XFER_XXX */
    size_t                  len;            /* [in/out] may change */
    void                    *buf;           /* [in/out] depends on dir */
    uint32_t                numIsoPkts;     /* [in] number of isochronous packets (8 max) */
    USBSUP_ISOCPKT          aIsoPkts[8];    /* [in/out] isochronous packet descriptors */
} USBSUP_URB, *PUSBSUP_URB;

typedef struct
{
    union
    {
        /* in: event handle */
        void* hEvent;
        /* out: result */
        int rc;
    } u;
} USBSUP_SET_NOTIFY_EVENT, *PUSBSUP_SET_NOTIFY_EVENT;

typedef struct
{
    uint16_t        usVendorId;
    uint16_t        usProductId;
    uint16_t        usRevision;
    uint16_t        usAlignment;
    char            DrvKeyName[512];
} USBSUP_DEVID, *PUSBSUP_DEVID;

#pragma pack()                          /* paranoia */


RT_C_DECLS_BEGIN

#ifdef IN_RING3

/** @defgroup   grp_usblib_r3     USBLIB Host Context Ring 3 API
 * @{
 */

/**
 * Return all attached USB devices.
 *
 * @returns VBox status code
 * @param ppDevices         Receives pointer to list of devices
 * @param pcbNumDevices     Number of USB devices in the list
 */
USBLIB_DECL(int) USBLibGetDevices(PUSBDEVICE *ppDevices, uint32_t *pcbNumDevices);

USBLIB_DECL(int) USBLibWaitChange(RTMSINTERVAL cMillies);

USBLIB_DECL(int) USBLibInterruptWaitChange(void);

USBLIB_DECL(int) USBLibRunFilters(void);

/** @} */
#endif

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_usblib_win_h */

