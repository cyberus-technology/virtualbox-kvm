/** @file
 * USB - Universal Serial Bus. (DEV,Main?)
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

#ifndef VBOX_INCLUDED_usb_h
#define VBOX_INCLUDED_usb_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_usblib_usb    USB Device Structures & Types
 * @ingroup grp_usblib
 * @{
 */

/**
 * The USB host device state.
 */
typedef enum USBDEVICESTATE
{
    /** The device is unsupported. */
    USBDEVICESTATE_UNSUPPORTED = 1,
    /** The device is in use by the host. */
    USBDEVICESTATE_USED_BY_HOST,
    /** The device is in use by the host but could perhaps be captured even so. */
    USBDEVICESTATE_USED_BY_HOST_CAPTURABLE,
    /** The device is not used by the host or any guest. */
    USBDEVICESTATE_UNUSED,
    /** The device is held by the proxy for later guest usage. */
    USBDEVICESTATE_HELD_BY_PROXY,
    /** The device in use by a guest. */
    USBDEVICESTATE_USED_BY_GUEST,
    /** The usual 32-bit hack. */
    USBDEVICESTATE_32BIT_HACK = 0x7fffffff
} USBDEVICESTATE;


/**
 * The USB device speed.
 */
typedef enum USBDEVICESPEED
{
    /** Unknown. */
    USBDEVICESPEED_UNKNOWN = 0,
    /** Low speed (1.5 Mbit/s). */
    USBDEVICESPEED_LOW,
    /** Full speed (12 Mbit/s). */
    USBDEVICESPEED_FULL,
    /** High speed (480 Mbit/s). */
    USBDEVICESPEED_HIGH,
    /** Variable speed - USB 2.5 / wireless. */
    USBDEVICESPEED_VARIABLE,
    /** Super speed - USB 3.0 (5Gbit/s). */
    USBDEVICESPEED_SUPER,
    /** The usual 32-bit hack. */
    USBDEVICESPEED_32BIT_HACK = 0x7fffffff
} USBDEVICESPEED;


/**
 * USB host device description.
 * Used for enumeration of USB devices.
 */
typedef struct USBDEVICE
{
    /** If linked, this is the pointer to the next device in the list. */
    struct USBDEVICE *pNext;
    /** If linked doubly, this is the pointer to the prev device in the list. */
    struct USBDEVICE *pPrev;
    /** Manufacturer string. */
    const char     *pszManufacturer;
    /** Product string. */
    const char     *pszProduct;
    /** Serial number string. */
    const char     *pszSerialNumber;
    /** The address of the device. */
    const char     *pszAddress;
    /** The backend to use for this device. */
    const char     *pszBackend;

    /** Vendor ID. */
    uint16_t        idVendor;
    /** Product ID. */
    uint16_t        idProduct;
    /** Revision, integer part. */
    uint16_t        bcdDevice;
    /** USB version number. */
    uint16_t        bcdUSB;
    /** Device class. */
    uint8_t         bDeviceClass;
    /** Device subclass. */
    uint8_t         bDeviceSubClass;
    /** Device protocol */
    uint8_t         bDeviceProtocol;
    /** Number of configurations. */
    uint8_t         bNumConfigurations;
    /** The device state. */
    USBDEVICESTATE  enmState;
    /** The device speed. */
    USBDEVICESPEED  enmSpeed;
    /** Serial hash. */
    uint64_t        u64SerialHash;
    /** The USB Bus number. */
    uint8_t         bBus;
    /** The port number. */
    uint8_t         bPort;
    /** The hub+port path. */
    char           *pszPortPath;
#if defined(RT_OS_LINUX) || defined(RT_OS_FREEBSD)
    /** Device number. */
    uint8_t         bDevNum;
#endif
#ifdef RT_OS_WINDOWS
    /** Alternate address. Can be NULL. */
    char           *pszAltAddress;
    /** The hub name. */
    char           *pszHubName;
#endif
#ifdef RT_OS_SOLARIS
    /** The /devices path of the device. */
    char           *pszDevicePath;
    /** Do we have a partial or full device descriptor here. */
    bool            fPartialDescriptor;
#endif
} USBDEVICE;
/** Pointer to a USB device. */
typedef USBDEVICE *PUSBDEVICE;
/** Pointer to a const USB device. */
typedef USBDEVICE *PCUSBDEVICE;


#ifdef VBOX_USB_H_INCL_DESCRIPTORS /* for the time being, since this may easily conflict with system headers */

/**
 * USB device descriptor.
 */
#pragma pack(1)
typedef struct USBDESCHDR
{
    /** The descriptor length. */
    uint8_t         bLength;
    /** The descriptor type. */
    uint8_t         bDescriptorType;
} USBDESCHDR;
#pragma pack()
/** Pointer to an USB descriptor header. */
typedef USBDESCHDR *PUSBDESCHDR;

/** @name Descriptor Type values (bDescriptorType)
 * {@ */
#if !defined(USB_DT_DEVICE) && !defined(USB_DT_ENDPOINT)
# define USB_DT_DEVICE              0x01
# define USB_DT_CONFIG              0x02
# define USB_DT_STRING              0x03
# define USB_DT_INTERFACE           0x04
# define USB_DT_ENDPOINT            0x05

# define USB_DT_HID                 0x21
# define USB_DT_REPORT              0x22
# define USB_DT_PHYSICAL            0x23
# define USB_DT_HUB                 0x29
#endif
/** @} */


/**
 * USB device descriptor.
 */
#pragma pack(1)
typedef struct USBDEVICEDESC
{
    /** The descriptor length. (Usually sizeof(USBDEVICEDESC).) */
    uint8_t         bLength;
    /** The descriptor type. (USB_DT_DEVICE) */
    uint8_t         bDescriptorType;
    /** USB version number. */
    uint16_t        bcdUSB;
    /** Device class. */
    uint8_t         bDeviceClass;
    /** Device subclass. */
    uint8_t         bDeviceSubClass;
    /** Device protocol */
    uint8_t         bDeviceProtocol;
    /** The max packet size of the default control pipe. */
    uint8_t         bMaxPacketSize0;
    /** Vendor ID. */
    uint16_t        idVendor;
    /** Product ID. */
    uint16_t        idProduct;
    /** Revision, integer part. */
    uint16_t        bcdDevice;
    /** Manufacturer string index. */
    uint8_t         iManufacturer;
    /** Product string index. */
    uint8_t         iProduct;
    /** Serial number string index. */
    uint8_t         iSerialNumber;
    /** Number of configurations. */
    uint8_t         bNumConfigurations;
} USBDEVICEDESC;
#pragma pack()
/** Pointer to an USB device descriptor. */
typedef USBDEVICEDESC *PUSBDEVICEDESC;

/** @name Class codes (bDeviceClass)
 * @{ */
#ifndef USB_HUB_CLASSCODE
# define USB_HUB_CLASSCODE  0x09
#endif
/** @} */

/**
 * USB configuration descriptor.
 */
#pragma pack(1)
typedef struct USBCONFIGDESC
{
    /** The descriptor length. (Usually sizeof(USBCONFIGDESC).) */
    uint8_t         bLength;
    /** The descriptor type. (USB_DT_CONFIG) */
    uint8_t         bDescriptorType;
    /** The length of the configuration descriptor plus all associated descriptors. */
    uint16_t        wTotalLength;
    /** Number of interfaces. */
    uint8_t         bNumInterfaces;
    /** Configuration number. (For SetConfiguration().) */
    uint8_t         bConfigurationValue;
    /** Configuration description string. */
    uint8_t         iConfiguration;
    /** Configuration characteristics. */
    uint8_t         bmAttributes;
    /** Maximum power consumption of the USB device in this config. */
    uint8_t         MaxPower;
} USBCONFIGDESC;
#pragma pack()
/** Pointer to an USB configuration descriptor. */
typedef USBCONFIGDESC *PUSBCONFIGDESC;

#endif /* VBOX_USB_H_INCL_DESCRIPTORS */

/** @} */
RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_usb_h */

