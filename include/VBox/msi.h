/** @file
 * MSI - Message signalled interrupts support.
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_msi_h
#define VBOX_INCLUDED_msi_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <iprt/assert.h>

#include <VBox/pci.h>

/* Constants for Intel APIC MSI messages */
#define VBOX_MSI_DATA_VECTOR_SHIFT           0
#define VBOX_MSI_DATA_VECTOR_MASK            0x000000ff
#define VBOX_MSI_DATA_VECTOR(v)              (((v) << VBOX_MSI_DATA_VECTOR_SHIFT) & \
                                                  VBOX_MSI_DATA_VECTOR_MASK)
#define VBOX_MSI_DATA_DELIVERY_MODE_SHIFT    8
#define  VBOX_MSI_DATA_DELIVERY_FIXED        (0 << VBOX_MSI_DATA_DELIVERY_MODE_SHIFT)
#define  VBOX_MSI_DATA_DELIVERY_LOWPRI       (1 << VBOX_MSI_DATA_DELIVERY_MODE_SHIFT)

#define VBOX_MSI_DATA_LEVEL_SHIFT            14
#define  VBOX_MSI_DATA_LEVEL_DEASSERT        (0 << VBOX_MSI_DATA_LEVEL_SHIFT)
#define  VBOX_MSI_DATA_LEVEL_ASSERT          (1 << VBOX_MSI_DATA_LEVEL_SHIFT)

#define VBOX_MSI_DATA_TRIGGER_SHIFT          15
#define  VBOX_MSI_DATA_TRIGGER_EDGE          (0 << VBOX_MSI_DATA_TRIGGER_SHIFT)
#define  VBOX_MSI_DATA_TRIGGER_LEVEL         (1 << VBOX_MSI_DATA_TRIGGER_SHIFT)

/**
 * MSI Interrupt Delivery modes.
 * In accordance with the Intel spec.
 * See Intel spec. "10.11.2 Message Data Register Format".
 */
#define VBOX_MSI_DELIVERY_MODE_FIXED         (0)
#define VBOX_MSI_DELIVERY_MODE_LOWEST_PRIO   (1)
#define VBOX_MSI_DELIVERY_MODE_SMI           (2)
#define VBOX_MSI_DELIVERY_MODE_NMI           (4)
#define VBOX_MSI_DELIVERY_MODE_INIT          (5)
#define VBOX_MSI_DELIVERY_MODE_EXT_INT       (7)

/**
 * MSI region, actually same as LAPIC MMIO region, but listens on bus,
 * not CPU, accesses.
 */
#define VBOX_MSI_ADDR_BASE                   0xfee00000
#define VBOX_MSI_ADDR_SIZE                   0x100000

#define VBOX_MSI_ADDR_SHIFT                  20

#define VBOX_MSI_ADDR_DEST_MODE_SHIFT        2
#define  VBOX_MSI_ADDR_DEST_MODE_PHYSICAL    (0 << VBOX_MSI_ADDR_DEST_MODE_SHIFT)
#define  VBOX_MSI_ADDR_DEST_MODE_LOGICAL     (1 << VBOX_MSI_ADDR_DEST_MODE_SHIFT)

#define VBOX_MSI_ADDR_REDIRECTION_SHIFT      3
#define  VBOX_MSI_ADDR_REDIRECTION_CPU       (0 << VBOX_MSI_ADDR_REDIRECTION_SHIFT)
                                        /* dedicated cpu */
#define  VBOX_MSI_ADDR_REDIRECTION_LOWPRI    (1 << VBOX_MSI_ADDR_REDIRECTION_SHIFT)
                                        /* lowest priority */

#define VBOX_MSI_ADDR_DEST_ID_SHIFT          12
#define  VBOX_MSI_ADDR_DEST_ID_MASK          0x00ffff0
#define  VBOX_MSI_ADDR_DEST_ID(dest)         (((dest) << VBOX_MSI_ADDR_DEST_ID_SHIFT) & \
                                         VBOX_MSI_ADDR_DEST_ID_MASK)
#define VBOX_MSI_ADDR_EXT_DEST_ID(dest)      ((dest) & 0xffffff00)

#define VBOX_MSI_ADDR_IR_EXT_INT             (1 << 4)
#define VBOX_MSI_ADDR_IR_SHV                 (1 << 3)
#define VBOX_MSI_ADDR_IR_INDEX1(index)       ((index & 0x8000) >> 13)
#define VBOX_MSI_ADDR_IR_INDEX2(index)       ((index & 0x7fff) << 5)

/* Maximum number of vectors, per device/function */
#define VBOX_MSI_MAX_ENTRIES                  32

/* Offsets in MSI PCI capability structure (VBOX_PCI_CAP_ID_MSI) */
#define VBOX_MSI_CAP_MESSAGE_CONTROL          0x02
#define VBOX_MSI_CAP_MESSAGE_ADDRESS_32       0x04
#define VBOX_MSI_CAP_MESSAGE_ADDRESS_LO       0x04
#define VBOX_MSI_CAP_MESSAGE_ADDRESS_HI       0x08
#define VBOX_MSI_CAP_MESSAGE_DATA_32          0x08
#define VBOX_MSI_CAP_MESSAGE_DATA_64          0x0c
#define VBOX_MSI_CAP_MASK_BITS_32             0x0c
#define VBOX_MSI_CAP_PENDING_BITS_32          0x10
#define VBOX_MSI_CAP_MASK_BITS_64             0x10
#define VBOX_MSI_CAP_PENDING_BITS_64          0x14

/* We implement MSI with per-vector masking */
#define VBOX_MSI_CAP_SIZE_32                  0x14
#define VBOX_MSI_CAP_SIZE_64                  0x18

/**
 * MSI-X differs from MSI by the fact that a dedicated physical page (in device
 * memory) is assigned for MSI-X table, and Pending Bit Array (PBA), which is
 * recommended to be separated from the main table by at least 2K.
 *
 * @{
 */
/** Size of a MSI-X page */
#define VBOX_MSIX_PAGE_SIZE                   0x1000
/** Pending interrupts (PBA) */
#define VBOX_MSIX_PAGE_PENDING                (VBOX_MSIX_PAGE_SIZE / 2)
/** Maximum number of vectors, per device/function */
#define VBOX_MSIX_MAX_ENTRIES                 2048
/** Size of MSI-X PCI capability */
#define VBOX_MSIX_CAP_SIZE                    12
/** Offsets in MSI-X PCI capability structure (VBOX_PCI_CAP_ID_MSIX) */
#define VBOX_MSIX_CAP_MESSAGE_CONTROL         0x02
#define VBOX_MSIX_TABLE_BIROFFSET             0x04
#define VBOX_MSIX_PBA_BIROFFSET               0x08
/** Size of single MSI-X table entry */
#define VBOX_MSIX_ENTRY_SIZE                  16
/** @} */

/**
 * MSI Address Register.
 */
typedef union MSIADDR
{
    /*
     * Intel and AMD xAPIC format.
     * See Intel spec. 10.11.1 "Message Address Register Format".
     * This also conforms to the AMD IOMMU spec. which omits specifying
     * individual fields but specifies reserved bits.
     */
    struct
    {
        uint32_t   u2Ign0      :  2;    /**< Bits 1:0   - Ignored (read as 0, writes ignored). */
        uint32_t   u1DestMode  :  1;    /**< Bit  2     - DM: Destination Mode. */
        uint32_t   u1RedirHint :  1;    /**< Bit  3     - RH: Redirection Hint. */
        uint32_t   u8Rsvd0     :  8;    /**< Bits 11:4  - Reserved. */
        uint32_t   u8DestId    :  8;    /**< Bits 19:12 - Destination Id. */
        uint32_t   u12Addr     : 12;    /**< Bits 31:20 - Address. */
        uint32_t   u32Rsvd0;            /**< Bits 63:32 - Reserved. */
    } n;

    /*
     * Intel x2APIC Format.
     * See Intel VT-d spec. 5.1.6.2 "Programming in Intel 64 x2APIC Mode".
     */
    struct
    {
        uint32_t   u2Ign0      :  2;    /**< Bits 1:0   - Ignored (read as 0, writes ignored). */
        uint32_t   u1DestMode  :  1;    /**< Bit  2     - DM: Destination Mode. */
        uint32_t   u1RedirHint :  1;    /**< Bit  3     - RH: Redirection Hint. */
        uint32_t   u8Rsvd0     :  8;    /**< Bits 11:4  - Reserved. */
        uint32_t   u8DestIdLo  :  8;    /**< Bits 19:12 - Destination Id (bits 7:0). */
        uint32_t   u12Addr     : 12;    /**< Bits 31:20 - Address. */
        uint32_t   u8Rsvd      :  8;    /**< Bits 39:32 - Reserved. */
        uint32_t   u24DestIdHi : 24;    /**< Bits 63:40 - Destination Id (bits 31:8). */
    } x2apic;

    /*
     * Intel IOMMU Remappable Interrupt Format.
     * See Intel VT-d spec. 5.1.2.2 "Interrupt Requests in Remappable Format".
     */
    struct
    {
        uint32_t   u2Ign0         :  2; /**< Bits 1:0   - Ignored (read as 0, writes ignored). */
        uint32_t   u1IntrIndexHi  :  1; /**< Bit  2     - Interrupt Index[15]. */
        uint32_t   fShv           :  1; /**< Bit  3     - Sub-Handle Valid. */
        uint32_t   fIntrFormat    :  1; /**< Bit  4     - Interrupt Format (1=remappable, 0=compatibility). */
        uint32_t   u14IntrIndexLo : 15; /**< Bits 19:5  - Interrupt Index[14:0]. */
        uint32_t   u12Addr        : 12; /**< Bits 31:20 - Address. */
        uint32_t   u32Rsvd0;            /**< Bits 63:32 - Reserved. */
    } dmar_remap;

    /** The 32-bit unsigned integer view. */
    uint32_t    au32[2];

    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} MSIADDR;
AssertCompileSize(MSIADDR, 8);
/** Pointer to an MSI address register. */
typedef MSIADDR *PMSIADDR;
/** Pointer to a const MSI address register. */
typedef MSIADDR const *PCMSIADDR;

/** Mask of valid bits in the MSI address register. According to the AMD IOMMU spec.
 *  and presumably the PCI spec., the top 32-bits are not reserved. From a PCI/IOMMU
 *  standpoint this makes sense. However, when dealing with the CPU side of things
 *  we might want to ensure the upper bits are reserved. Does x86/x64 really
 *  support a 64-bit MSI address? */
#define VBOX_MSI_ADDR_VALID_MASK           UINT64_C(0xfffffffffffffffc)
#define VBOX_MSI_ADDR_ADDR_MASK            UINT64_C(0x00000000fff00000)

/**
 * MSI Data Register.
 */
typedef union MSIDATA
{
    /*
     * Intel and AMD xAPIC format.
     * See Intel spec. 10.11.2 "Message Data Register Format".
     * This also conforms to the AMD IOMMU spec. which omits specifying
     * individual fields but specifies reserved bits.
     */
    struct
    {
        uint32_t    u8Vector       : 8;     /**< Bits 7:0   - Vector. */
        uint32_t    u3DeliveryMode : 3;     /**< Bits 10:8  - Delivery Mode. */
        uint32_t    u3Rsvd0        : 3;     /**< Bits 13:11 - Reserved. */
        uint32_t    u1Level        : 1;     /**< Bit  14    - Level. */
        uint32_t    u1TriggerMode  : 1;     /**< Bit  15    - Trigger Mode (0=edge, 1=level). */
        uint32_t    u16Rsvd0       : 16;    /**< Bits 31:16 - Reserved. */
    } n;

    /*
     * Intel x2APIC Format.
     * See Intel VT-d spec. 5.1.6.2 "Programming in Intel 64 x2APIC Mode".
     */
    struct
    {
        uint32_t    u8Vector       :  8;    /**< Bits 7:0   - Vector. */
        uint32_t    u1DeliveryMode :  1;    /**< Bit  8     - Delivery Mode (0=fixed, 1=lowest priority). */
        uint32_t    u23Rsvd0       : 23;    /**< Bits 31:9  - Reserved. */
    } x2apic;

    /*
     * Intel IOMMU Remappable Interrupt Format.
     * See Intel VT-d spec. 5.1.2.2 "Interrupt Requests in Remappable Format".
     */
    struct
    {
        uint16_t    u16SubHandle;
        uint16_t    u16Rsvd0;
    } dmar_remap;

    /** The 32-bit unsigned integer view. */
    uint32_t    u32;
} MSIDATA;
AssertCompileSize(MSIDATA, 4);
/** Pointer to an MSI data register. */
typedef MSIDATA *PMSIDATA;
/** Pointer to a const MSI data register. */
typedef MSIDATA const *PCMSIDATA;

/** Mask of valid bits in the MSI data register. */
#define VBOX_MSI_DATA_VALID_MASK           UINT64_C(0x000000000000ffff)

/**
 * MSI Message (Address and Data Register Pair).
 */
typedef struct MSIMSG
{
    /** The MSI Address Register. */
    MSIADDR      Addr;
    /** The MSI Data Register. */
    MSIDATA     Data;
} MSIMSG;

#endif /* !VBOX_INCLUDED_msi_h */
