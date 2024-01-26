/** @file
 * IOMMU - Input/Output Memory Management Unit (AMD).
 */

/*
 * Copyright (C) 2020-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_iommu_amd_h
#define VBOX_INCLUDED_iommu_amd_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#include <iprt/assertcompile.h>

/**
 * @name PCI configuration register offsets.
 * In accordance with the AMD spec.
 * @{
 */
#define IOMMU_PCI_OFF_CAP_HDR                       0x40
#define IOMMU_PCI_OFF_BASE_ADDR_REG_LO              0x44
#define IOMMU_PCI_OFF_BASE_ADDR_REG_HI              0x48
#define IOMMU_PCI_OFF_RANGE_REG                     0x4c
#define IOMMU_PCI_OFF_MISCINFO_REG_0                0x50
#define IOMMU_PCI_OFF_MISCINFO_REG_1                0x54
#define IOMMU_PCI_OFF_MSI_CAP_HDR                   0x64
#define IOMMU_PCI_OFF_MSI_ADDR_LO                   0x68
#define IOMMU_PCI_OFF_MSI_ADDR_HI                   0x6c
#define IOMMU_PCI_OFF_MSI_DATA                      0x70
#define IOMMU_PCI_OFF_MSI_MAP_CAP_HDR               0x74
/** @} */

/**
 * @name MMIO register offsets.
 * In accordance with the AMD spec.
 * @{
 */
#define IOMMU_MMIO_OFF_QWORD_TABLE_0_START          IOMMU_MMIO_OFF_DEV_TAB_BAR
#define IOMMU_MMIO_OFF_DEV_TAB_BAR                  0x00
#define IOMMU_MMIO_OFF_CMD_BUF_BAR                  0x08
#define IOMMU_MMIO_OFF_EVT_LOG_BAR                  0x10
#define IOMMU_MMIO_OFF_CTRL                         0x18
#define IOMMU_MMIO_OFF_EXCL_BAR                     0x20
#define IOMMU_MMIO_OFF_EXCL_RANGE_LIMIT             0x28
#define IOMMU_MMIO_OFF_EXT_FEAT                     0x30

#define IOMMU_MMIO_OFF_PPR_LOG_BAR                  0x38
#define IOMMU_MMIO_OFF_HW_EVT_HI                    0x40
#define IOMMU_MMIO_OFF_HW_EVT_LO                    0x48
#define IOMMU_MMIO_OFF_HW_EVT_STATUS                0x50

#define IOMMU_MMIO_OFF_SMI_FLT_FIRST                0x60
#define IOMMU_MMIO_OFF_SMI_FLT_LAST                 0xd8

#define IOMMU_MMIO_OFF_GALOG_BAR                    0xe0
#define IOMMU_MMIO_OFF_GALOG_TAIL_ADDR              0xe8

#define IOMMU_MMIO_OFF_PPR_LOG_B_BAR                0xf0
#define IOMMU_MMIO_OFF_PPR_EVT_B_BAR                0xf8

#define IOMMU_MMIO_OFF_DEV_TAB_SEG_FIRST            0x100
#define IOMMU_MMIO_OFF_DEV_TAB_SEG_1                0x100
#define IOMMU_MMIO_OFF_DEV_TAB_SEG_2                0x108
#define IOMMU_MMIO_OFF_DEV_TAB_SEG_3                0x110
#define IOMMU_MMIO_OFF_DEV_TAB_SEG_4                0x118
#define IOMMU_MMIO_OFF_DEV_TAB_SEG_5                0x120
#define IOMMU_MMIO_OFF_DEV_TAB_SEG_6                0x128
#define IOMMU_MMIO_OFF_DEV_TAB_SEG_7                0x130
#define IOMMU_MMIO_OFF_DEV_TAB_SEG_LAST             0x130

#define IOMMU_MMIO_OFF_DEV_SPECIFIC_FEAT            0x138
#define IOMMU_MMIO_OFF_DEV_SPECIFIC_CTRL            0x140
#define IOMMU_MMIO_OFF_DEV_SPECIFIC_STATUS          0x148

#define IOMMU_MMIO_OFF_MSI_VECTOR_0                 0x150
#define IOMMU_MMIO_OFF_MSI_VECTOR_1                 0x154
#define IOMMU_MMIO_OFF_MSI_CAP_HDR                  0x158
#define IOMMU_MMIO_OFF_MSI_ADDR_LO                  0x15c
#define IOMMU_MMIO_OFF_MSI_ADDR_HI                  0x160
#define IOMMU_MMIO_OFF_MSI_DATA                     0x164
#define IOMMU_MMIO_OFF_MSI_MAPPING_CAP_HDR          0x168

#define IOMMU_MMIO_OFF_PERF_OPT_CTRL                0x16c

#define IOMMU_MMIO_OFF_XT_GEN_INTR_CTRL             0x170
#define IOMMU_MMIO_OFF_XT_PPR_INTR_CTRL             0x178
#define IOMMU_MMIO_OFF_XT_GALOG_INT_CTRL            0x180
#define IOMMU_MMIO_OFF_QWORD_TABLE_0_END            (IOMMU_MMIO_OFF_XT_GALOG_INT_CTRL + 8)

#define IOMMU_MMIO_OFF_QWORD_TABLE_1_START          IOMMU_MMIO_OFF_MARC_APER_BAR_0
#define IOMMU_MMIO_OFF_MARC_APER_BAR_0              0x200
#define IOMMU_MMIO_OFF_MARC_APER_RELOC_0            0x208
#define IOMMU_MMIO_OFF_MARC_APER_LEN_0              0x210
#define IOMMU_MMIO_OFF_MARC_APER_BAR_1              0x218
#define IOMMU_MMIO_OFF_MARC_APER_RELOC_1            0x220
#define IOMMU_MMIO_OFF_MARC_APER_LEN_1              0x228
#define IOMMU_MMIO_OFF_MARC_APER_BAR_2              0x230
#define IOMMU_MMIO_OFF_MARC_APER_RELOC_2            0x238
#define IOMMU_MMIO_OFF_MARC_APER_LEN_2              0x240
#define IOMMU_MMIO_OFF_MARC_APER_BAR_3              0x248
#define IOMMU_MMIO_OFF_MARC_APER_RELOC_3            0x250
#define IOMMU_MMIO_OFF_MARC_APER_LEN_3              0x258
#define IOMMU_MMIO_OFF_QWORD_TABLE_1_END            (IOMMU_MMIO_OFF_MARC_APER_LEN_3 + 8)

#define IOMMU_MMIO_OFF_QWORD_TABLE_2_START          IOMMU_MMIO_OFF_RSVD_REG
#define IOMMU_MMIO_OFF_RSVD_REG                     0x1ff8

#define IOMMU_MMIO_OFF_CMD_BUF_HEAD_PTR             0x2000
#define IOMMU_MMIO_OFF_CMD_BUF_TAIL_PTR             0x2008
#define IOMMU_MMIO_OFF_EVT_LOG_HEAD_PTR             0x2010
#define IOMMU_MMIO_OFF_EVT_LOG_TAIL_PTR             0x2018

#define IOMMU_MMIO_OFF_STATUS                       0x2020

#define IOMMU_MMIO_OFF_PPR_LOG_HEAD_PTR             0x2030
#define IOMMU_MMIO_OFF_PPR_LOG_TAIL_PTR             0x2038

#define IOMMU_MMIO_OFF_GALOG_HEAD_PTR               0x2040
#define IOMMU_MMIO_OFF_GALOG_TAIL_PTR               0x2048

#define IOMMU_MMIO_OFF_PPR_LOG_B_HEAD_PTR           0x2050
#define IOMMU_MMIO_OFF_PPR_LOG_B_TAIL_PTR           0x2058

#define IOMMU_MMIO_OFF_EVT_LOG_B_HEAD_PTR           0x2070
#define IOMMU_MMIO_OFF_EVT_LOG_B_TAIL_PTR           0x2078

#define IOMMU_MMIO_OFF_PPR_LOG_AUTO_RESP            0x2080
#define IOMMU_MMIO_OFF_PPR_LOG_OVERFLOW_EARLY       0x2088
#define IOMMU_MMIO_OFF_PPR_LOG_B_OVERFLOW_EARLY     0x2090
#define IOMMU_MMIO_OFF_QWORD_TABLE_2_END            (IOMMU_MMIO_OFF_PPR_LOG_B_OVERFLOW_EARLY + 8)
/** @} */

/**
 * @name MMIO register-access table offsets.
 * Each table [first..last] (both inclusive) represents the range of registers
 * covered by a distinct register-access table. This is done due to arbitrary large
 * gaps in the MMIO register offsets themselves.
 * @{
 */
#define IOMMU_MMIO_OFF_TABLE_0_FIRST               0x00
#define IOMMU_MMIO_OFF_TABLE_0_LAST                0x258

#define IOMMU_MMIO_OFF_TABLE_1_FIRST               0x1ff8
#define IOMMU_MMIO_OFF_TABLE_1_LAST                0x2090
/** @} */

/**
 * @name Commands.
 * In accordance with the AMD spec.
 * @{
 */
#define IOMMU_CMD_COMPLETION_WAIT                   0x01
#define IOMMU_CMD_INV_DEV_TAB_ENTRY                 0x02
#define IOMMU_CMD_INV_IOMMU_PAGES                   0x03
#define IOMMU_CMD_INV_IOTLB_PAGES                   0x04
#define IOMMU_CMD_INV_INTR_TABLE                    0x05
#define IOMMU_CMD_PREFETCH_IOMMU_PAGES              0x06
#define IOMMU_CMD_COMPLETE_PPR_REQ                  0x07
#define IOMMU_CMD_INV_IOMMU_ALL                     0x08
/** @} */

/**
 * @name Event codes.
 * In accordance with the AMD spec.
 * @{
 */
#define IOMMU_EVT_ILLEGAL_DEV_TAB_ENTRY             0x01
#define IOMMU_EVT_IO_PAGE_FAULT                     0x02
#define IOMMU_EVT_DEV_TAB_HW_ERROR                  0x03
#define IOMMU_EVT_PAGE_TAB_HW_ERROR                 0x04
#define IOMMU_EVT_ILLEGAL_CMD_ERROR                 0x05
#define IOMMU_EVT_COMMAND_HW_ERROR                  0x06
#define IOMMU_EVT_IOTLB_INV_TIMEOUT                 0x07
#define IOMMU_EVT_INVALID_DEV_REQ                   0x08
#define IOMMU_EVT_INVALID_PPR_REQ                   0x09
#define IOMMU_EVT_EVENT_COUNTER_ZERO                0x10
#define IOMMU_EVT_GUEST_EVENT_FAULT                 0x11
/** @} */

/**
 * @name IOMMU Capability Header.
 * In accordance with the AMD spec.
 * @{
 */
/** CapId: Capability ID. */
#define IOMMU_BF_CAPHDR_CAP_ID_SHIFT                0
#define IOMMU_BF_CAPHDR_CAP_ID_MASK                 UINT32_C(0x000000ff)
/** CapPtr: Capability Pointer. */
#define IOMMU_BF_CAPHDR_CAP_PTR_SHIFT               8
#define IOMMU_BF_CAPHDR_CAP_PTR_MASK                UINT32_C(0x0000ff00)
/** CapType: Capability Type. */
#define IOMMU_BF_CAPHDR_CAP_TYPE_SHIFT              16
#define IOMMU_BF_CAPHDR_CAP_TYPE_MASK               UINT32_C(0x00070000)
/** CapRev: Capability Revision. */
#define IOMMU_BF_CAPHDR_CAP_REV_SHIFT               19
#define IOMMU_BF_CAPHDR_CAP_REV_MASK                UINT32_C(0x00f80000)
/** IoTlbSup: IO TLB Support. */
#define IOMMU_BF_CAPHDR_IOTLB_SUP_SHIFT             24
#define IOMMU_BF_CAPHDR_IOTLB_SUP_MASK              UINT32_C(0x01000000)
/** HtTunnel: HyperTransport Tunnel translation support. */
#define IOMMU_BF_CAPHDR_HT_TUNNEL_SHIFT             25
#define IOMMU_BF_CAPHDR_HT_TUNNEL_MASK              UINT32_C(0x02000000)
/** NpCache: Not Present table entries Cached. */
#define IOMMU_BF_CAPHDR_NP_CACHE_SHIFT              26
#define IOMMU_BF_CAPHDR_NP_CACHE_MASK               UINT32_C(0x04000000)
/** EFRSup: Extended Feature Register (EFR) Supported. */
#define IOMMU_BF_CAPHDR_EFR_SUP_SHIFT               27
#define IOMMU_BF_CAPHDR_EFR_SUP_MASK                UINT32_C(0x08000000)
/** CapExt: Miscellaneous Information Register Supported . */
#define IOMMU_BF_CAPHDR_CAP_EXT_SHIFT               28
#define IOMMU_BF_CAPHDR_CAP_EXT_MASK                UINT32_C(0x10000000)
/** Bits 31:29 reserved. */
#define IOMMU_BF_CAPHDR_RSVD_29_31_SHIFT            29
#define IOMMU_BF_CAPHDR_RSVD_29_31_MASK             UINT32_C(0xe0000000)
RT_BF_ASSERT_COMPILE_CHECKS(IOMMU_BF_CAPHDR_, UINT32_C(0), UINT32_MAX,
                            (CAP_ID, CAP_PTR, CAP_TYPE, CAP_REV, IOTLB_SUP, HT_TUNNEL, NP_CACHE, EFR_SUP, CAP_EXT, RSVD_29_31));
/** @} */

/**
 * @name IOMMU Base Address Low Register.
 * In accordance with the AMD spec.
 * @{
 */
/** Enable: Enables access to the address specified in the Base Address Register. */
#define IOMMU_BF_BASEADDR_LO_ENABLE_SHIFT           0
#define IOMMU_BF_BASEADDR_LO_ENABLE_MASK            UINT32_C(0x00000001)
/** Bits 13:1 reserved. */
#define IOMMU_BF_BASEADDR_LO_RSVD_1_13_SHIFT        1
#define IOMMU_BF_BASEADDR_LO_RSVD_1_13_MASK         UINT32_C(0x00003ffe)
/** Base Address[31:14]: Low Base address of IOMMU MMIO control registers. */
#define IOMMU_BF_BASEADDR_LO_ADDR_SHIFT             14
#define IOMMU_BF_BASEADDR_LO_ADDR_MASK              UINT32_C(0xffffc000)
RT_BF_ASSERT_COMPILE_CHECKS(IOMMU_BF_BASEADDR_LO_, UINT32_C(0), UINT32_MAX,
                            (ENABLE, RSVD_1_13, ADDR));
/** @} */

/**
 * @name IOMMU Range Register.
 * In accordance with the AMD spec.
 * @{
 */
/** UnitID: HyperTransport Unit ID. */
#define IOMMU_BF_RANGE_UNIT_ID_SHIFT                0
#define IOMMU_BF_RANGE_UNIT_ID_MASK                 UINT32_C(0x0000001f)
/** Bits 6:5 reserved. */
#define IOMMU_BF_RANGE_RSVD_5_6_SHIFT               5
#define IOMMU_BF_RANGE_RSVD_5_6_MASK                UINT32_C(0x00000060)
/** RngValid: Range valid. */
#define IOMMU_BF_RANGE_VALID_SHIFT                  7
#define IOMMU_BF_RANGE_VALID_MASK                   UINT32_C(0x00000080)
/** BusNumber: Device range bus number. */
#define IOMMU_BF_RANGE_BUS_NUMBER_SHIFT             8
#define IOMMU_BF_RANGE_BUS_NUMBER_MASK              UINT32_C(0x0000ff00)
/** First Device. */
#define IOMMU_BF_RANGE_FIRST_DEVICE_SHIFT           16
#define IOMMU_BF_RANGE_FIRST_DEVICE_MASK            UINT32_C(0x00ff0000)
/** Last Device. */
#define IOMMU_BF_RANGE_LAST_DEVICE_SHIFT            24
#define IOMMU_BF_RANGE_LAST_DEVICE_MASK             UINT32_C(0xff000000)
RT_BF_ASSERT_COMPILE_CHECKS(IOMMU_BF_RANGE_, UINT32_C(0), UINT32_MAX,
                            (UNIT_ID, RSVD_5_6, VALID, BUS_NUMBER, FIRST_DEVICE, LAST_DEVICE));
/** @} */

/**
 * @name IOMMU Miscellaneous Information Register 0.
 * In accordance with the AMD spec.
 * @{
 */
/** MsiNum: MSI message number. */
#define IOMMU_BF_MISCINFO_0_MSI_NUM_SHIFT           0
#define IOMMU_BF_MISCINFO_0_MSI_NUM_MASK            UINT32_C(0x0000001f)
/** GvaSize: Guest Virtual Address Size. */
#define IOMMU_BF_MISCINFO_0_GVA_SIZE_SHIFT          5
#define IOMMU_BF_MISCINFO_0_GVA_SIZE_MASK           UINT32_C(0x000000e0)
/** PaSize: Physical Address Size. */
#define IOMMU_BF_MISCINFO_0_PA_SIZE_SHIFT           8
#define IOMMU_BF_MISCINFO_0_PA_SIZE_MASK            UINT32_C(0x00007f00)
/** VaSize: Virtual Address Size. */
#define IOMMU_BF_MISCINFO_0_VA_SIZE_SHIFT           15
#define IOMMU_BF_MISCINFO_0_VA_SIZE_MASK            UINT32_C(0x003f8000)
/** HtAtsResv: HyperTransport ATS Response Address range Reserved. */
#define IOMMU_BF_MISCINFO_0_HT_ATS_RESV_SHIFT       22
#define IOMMU_BF_MISCINFO_0_HT_ATS_RESV_MASK        UINT32_C(0x00400000)
/** Bits 26:23 reserved. */
#define IOMMU_BF_MISCINFO_0_RSVD_23_26_SHIFT        23
#define IOMMU_BF_MISCINFO_0_RSVD_23_26_MASK         UINT32_C(0x07800000)
/** MsiNumPPR: Peripheral Page Request MSI message number. */
#define IOMMU_BF_MISCINFO_0_MSI_NUM_PPR_SHIFT       27
#define IOMMU_BF_MISCINFO_0_MSI_NUM_PPR_MASK        UINT32_C(0xf8000000)
RT_BF_ASSERT_COMPILE_CHECKS(IOMMU_BF_MISCINFO_0_, UINT32_C(0), UINT32_MAX,
                            (MSI_NUM, GVA_SIZE, PA_SIZE, VA_SIZE, HT_ATS_RESV, RSVD_23_26, MSI_NUM_PPR));
/** @} */

/**
 * @name IOMMU Miscellaneous Information Register 1.
 * In accordance with the AMD spec.
 * @{
 */
/** MsiNumGA: MSI message number for guest virtual-APIC log. */
#define IOMMU_BF_MISCINFO_1_MSI_NUM_GA_SHIFT        0
#define IOMMU_BF_MISCINFO_1_MSI_NUM_GA_MASK         UINT32_C(0x0000001f)
/** Bits 31:5 reserved. */
#define IOMMU_BF_MISCINFO_1_RSVD_5_31_SHIFT         5
#define IOMMU_BF_MISCINFO_1_RSVD_5_31_MASK          UINT32_C(0xffffffe0)
RT_BF_ASSERT_COMPILE_CHECKS(IOMMU_BF_MISCINFO_1_, UINT32_C(0), UINT32_MAX,
                            (MSI_NUM_GA, RSVD_5_31));
/** @} */

/**
 * @name MSI Capability Header Register.
 * In accordance with the AMD spec.
 * @{
 */
/** MsiCapId: Capability ID. */
#define IOMMU_BF_MSI_CAP_HDR_CAP_ID_SHIFT           0
#define IOMMU_BF_MSI_CAP_HDR_CAP_ID_MASK            UINT32_C(0x000000ff)
/** MsiCapPtr: Pointer (PCI config offset) to the next capability. */
#define IOMMU_BF_MSI_CAP_HDR_CAP_PTR_SHIFT          8
#define IOMMU_BF_MSI_CAP_HDR_CAP_PTR_MASK           UINT32_C(0x0000ff00)
/** MsiEn: Message Signal Interrupt enable. */
#define IOMMU_BF_MSI_CAP_HDR_EN_SHIFT               16
#define IOMMU_BF_MSI_CAP_HDR_EN_MASK                UINT32_C(0x00010000)
/** MsiMultMessCap: MSI Multi-Message Capability. */
#define IOMMU_BF_MSI_CAP_HDR_MULTMESS_CAP_SHIFT     17
#define IOMMU_BF_MSI_CAP_HDR_MULTMESS_CAP_MASK      UINT32_C(0x000e0000)
/** MsiMultMessEn: MSI Mult-Message Enable. */
#define IOMMU_BF_MSI_CAP_HDR_MULTMESS_EN_SHIFT      20
#define IOMMU_BF_MSI_CAP_HDR_MULTMESS_EN_MASK       UINT32_C(0x00700000)
/** Msi64BitEn: MSI 64-bit Enabled. */
#define IOMMU_BF_MSI_CAP_HDR_64BIT_EN_SHIFT         23
#define IOMMU_BF_MSI_CAP_HDR_64BIT_EN_MASK          UINT32_C(0x00800000)
/** Bits 31:24 reserved. */
#define IOMMU_BF_MSI_CAP_HDR_RSVD_24_31_SHIFT       24
#define IOMMU_BF_MSI_CAP_HDR_RSVD_24_31_MASK        UINT32_C(0xff000000)
RT_BF_ASSERT_COMPILE_CHECKS(IOMMU_BF_MSI_CAP_HDR_, UINT32_C(0), UINT32_MAX,
                            (CAP_ID, CAP_PTR, EN, MULTMESS_CAP, MULTMESS_EN, 64BIT_EN, RSVD_24_31));
/** @} */

/**
 * @name MSI Mapping Capability Header Register.
 * In accordance with the AMD spec.
 * @{
 */
/** MsiMapCapId: Capability ID. */
#define IOMMU_BF_MSI_MAP_CAPHDR_CAP_ID_SHIFT        0
#define IOMMU_BF_MSI_MAP_CAPHDR_CAP_ID_MASK         UINT32_C(0x000000ff)
/** MsiMapCapPtr: Pointer (PCI config offset) to the next capability. */
#define IOMMU_BF_MSI_MAP_CAPHDR_CAP_PTR_SHIFT       8
#define IOMMU_BF_MSI_MAP_CAPHDR_CAP_PTR_MASK        UINT32_C(0x0000ff00)
/** MsiMapEn: MSI mapping capability enable. */
#define IOMMU_BF_MSI_MAP_CAPHDR_EN_SHIFT            16
#define IOMMU_BF_MSI_MAP_CAPHDR_EN_MASK             UINT32_C(0x00010000)
/** MsiMapFixd: MSI interrupt mapping range is not programmable. */
#define IOMMU_BF_MSI_MAP_CAPHDR_FIXED_SHIFT         17
#define IOMMU_BF_MSI_MAP_CAPHDR_FIXED_MASK          UINT32_C(0x00020000)
/** Bits 18:28 reserved. */
#define IOMMU_BF_MSI_MAP_CAPHDR_RSVD_18_28_SHIFT    18
#define IOMMU_BF_MSI_MAP_CAPHDR_RSVD_18_28_MASK     UINT32_C(0x07fc0000)
/** MsiMapCapType: MSI mapping capability. */
#define IOMMU_BF_MSI_MAP_CAPHDR_CAP_TYPE_SHIFT      27
#define IOMMU_BF_MSI_MAP_CAPHDR_CAP_TYPE_MASK       UINT32_C(0xf8000000)
RT_BF_ASSERT_COMPILE_CHECKS(IOMMU_BF_MSI_MAP_CAPHDR_, UINT32_C(0), UINT32_MAX,
                            (CAP_ID, CAP_PTR, EN, FIXED, RSVD_18_28, CAP_TYPE));
/** @} */

/**
 * @name IOMMU Status Register Bits.
 * In accordance with the AMD spec.
 * @{
 */
/** EventOverflow: Event log overflow. */
#define IOMMU_STATUS_EVT_LOG_OVERFLOW               RT_BIT_64(0)
/** EventLogInt: Event log interrupt. */
#define IOMMU_STATUS_EVT_LOG_INTR                   RT_BIT_64(1)
/** ComWaitInt: Completion wait interrupt. */
#define IOMMU_STATUS_COMPLETION_WAIT_INTR           RT_BIT_64(2)
/** EventLogRun: Event log is running. */
#define IOMMU_STATUS_EVT_LOG_RUNNING                RT_BIT_64(3)
/** CmdBufRun: Command buffer is running. */
#define IOMMU_STATUS_CMD_BUF_RUNNING                RT_BIT_64(4)
/** PprOverflow: Peripheral page request log overflow. */
#define IOMMU_STATUS_PPR_LOG_OVERFLOW               RT_BIT_64(5)
/** PprInt: Peripheral page request log interrupt. */
#define IOMMU_STATUS_PPR_LOG_INTR                   RT_BIT_64(6)
/** PprLogRun: Peripheral page request log is running. */
#define IOMMU_STATUS_PPR_LOG_RUN                    RT_BIT_64(7)
/** GALogRun: Guest virtual-APIC log is running. */
#define IOMMU_STATUS_GA_LOG_RUN                     RT_BIT_64(8)
/** GALOverflow: Guest virtual-APIC log overflow. */
#define IOMMU_STATUS_GA_LOG_OVERFLOW                RT_BIT_64(9)
/** GAInt: Guest virtual-APIC log interrupt. */
#define IOMMU_STATUS_GA_LOG_INTR                    RT_BIT_64(10)
/** PprOvrflwB: PPR Log B overflow. */
#define IOMMU_STATUS_PPR_LOG_B_OVERFLOW             RT_BIT_64(11)
/** PprLogActive: PPR Log B is active. */
#define IOMMU_STATUS_PPR_LOG_B_ACTIVE               RT_BIT_64(12)
/** EvtOvrflwB: Event log B overflow. */
#define IOMMU_STATUS_EVT_LOG_B_OVERFLOW             RT_BIT_64(15)
/** EventLogActive: Event log B active. */
#define IOMMU_STATUS_EVT_LOG_B_ACTIVE               RT_BIT_64(16)
/** PprOvrflwEarlyB: PPR log B overflow early warning. */
#define IOMMU_STATUS_PPR_LOG_B_OVERFLOW_EARLY       RT_BIT_64(17)
/** PprOverflowEarly: PPR log overflow early warning. */
#define IOMMU_STATUS_PPR_LOG_OVERFLOW_EARLY         RT_BIT_64(18)
/** @} */

/** @name IOMMU_IO_PERM_XXX: IOMMU I/O access permissions bits.
 * In accordance with the AMD spec.
 *
 * These values match the shifted values of the IR and IW field of the DTE and the
 * PTE, PDE of the I/O page tables.
 *
 * @{ */
#define IOMMU_IO_PERM_NONE                          (0)
#define IOMMU_IO_PERM_READ                          RT_BIT_64(0)
#define IOMMU_IO_PERM_WRITE                         RT_BIT_64(1)
#define IOMMU_IO_PERM_READ_WRITE                    (IOMMU_IO_PERM_READ | IOMMU_IO_PERM_WRITE)
#define IOMMU_IO_PERM_SHIFT                         61
#define IOMMU_IO_PERM_MASK                          0x3
/** @} */

/** @name SYSMGT_TYPE_XXX: System Management Message Enable Types.
 * In accordance with the AMD spec.
 * @{ */
#define SYSMGTTYPE_DMA_DENY                         (0)
#define SYSMGTTYPE_MSG_ALL_ALLOW                    (1)
#define SYSMGTTYPE_MSG_INT_ALLOW                    (2)
#define SYSMGTTYPE_DMA_ALLOW                        (3)
/** @} */

/** @name IOMMU_INTR_CTRL_XX: DTE::IntCtl field values.
 * These are control bits for handling fixed and arbitrated interrupts.
 * In accordance with the AMD spec.
 * @{ */
#define IOMMU_INTR_CTRL_TARGET_ABORT                (0)
#define IOMMU_INTR_CTRL_FWD_UNMAPPED                (1)
#define IOMMU_INTR_CTRL_REMAP                       (2)
#define IOMMU_INTR_CTRL_RSVD                        (3)
/** @} */

/** Gets the device table length (in bytes) given the device table pointer. */
#define IOMMU_GET_DEV_TAB_LEN(a_pDevTab)            (((a_pDevTab)->n.u9Size + 1) << X86_PAGE_4K_SHIFT)

/**
 * The Device ID.
 * In accordance with VirtualBox's PCI configuration.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint16_t  u3Function : 3;  /**< Bits 2:0   - Function. */
        RT_GCC_EXTENSION uint16_t  u9Device : 9;    /**< Bits 11:3  - Device. */
        RT_GCC_EXTENSION uint16_t  u4Bus : 4;       /**< Bits 15:12 - Bus. */
    } n;
    /** The unsigned integer view. */
    uint16_t        u;
} DEVICE_ID_T;
AssertCompileSize(DEVICE_ID_T, 2);

/**
 * Device Table Entry (DTE).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t  u1Valid : 1;                   /**< Bit  0       - V: Valid. */
        RT_GCC_EXTENSION uint64_t  u1TranslationValid : 1;        /**< Bit  1       - TV: Translation information Valid. */
        RT_GCC_EXTENSION uint64_t  u5Rsvd0 : 5;                   /**< Bits 6:2     - Reserved. */
        RT_GCC_EXTENSION uint64_t  u2Had : 2;                     /**< Bits 8:7     - HAD: Host Access Dirty. */
        RT_GCC_EXTENSION uint64_t  u3Mode : 3;                    /**< Bits 11:9    - Mode: Paging mode. */
        RT_GCC_EXTENSION uint64_t  u40PageTableRootPtrLo : 40;    /**< Bits 51:12   - Page Table Root Pointer. */
        RT_GCC_EXTENSION uint64_t  u1Ppr : 1;                     /**< Bit  52      - PPR: Peripheral Page Request. */
        RT_GCC_EXTENSION uint64_t  u1GstPprRespPasid : 1;         /**< Bit  53      - GRPR: Guest PPR Response with PASID. */
        RT_GCC_EXTENSION uint64_t  u1GstIoValid : 1;              /**< Bit  54      - GIoV: Guest I/O Protection Valid. */
        RT_GCC_EXTENSION uint64_t  u1GstTranslateValid : 1;       /**< Bit  55      - GV: Guest translation Valid. */
        RT_GCC_EXTENSION uint64_t  u2GstMode : 2;                 /**< Bits 57:56   - GLX: Guest Paging mode levels. */
        RT_GCC_EXTENSION uint64_t  u3GstCr3TableRootPtrLo : 3;    /**< Bits 60:58   - GCR3 TRP: Guest CR3 Table Root Ptr (Lo). */
        RT_GCC_EXTENSION uint64_t  u1IoRead : 1;                  /**< Bit  61      - IR: I/O Read permission. */
        RT_GCC_EXTENSION uint64_t  u1IoWrite : 1;                 /**< Bit  62      - IW: I/O Write permission. */
        RT_GCC_EXTENSION uint64_t  u1Rsvd0 : 1;                   /**< Bit  63      - Reserved. */
        RT_GCC_EXTENSION uint64_t  u16DomainId : 16;              /**< Bits 79:64   - Domain ID. */
        RT_GCC_EXTENSION uint64_t  u16GstCr3TableRootPtrMid : 16; /**< Bits 95:80   - GCR3 TRP: Guest CR3 Table Root Ptr (Mid). */
        RT_GCC_EXTENSION uint64_t  u1IoTlbEnable : 1;             /**< Bit  96      - I: IOTLB Enable (remote). */
        RT_GCC_EXTENSION uint64_t  u1SuppressPfEvents : 1;        /**< Bit  97      - SE: Suppress Page-fault events. */
        RT_GCC_EXTENSION uint64_t  u1SuppressAllPfEvents : 1;     /**< Bit  98      - SA: Suppress All Page-fault events. */
        RT_GCC_EXTENSION uint64_t  u2IoCtl : 2;                   /**< Bits 100:99  - IoCtl: Port I/O Control. */
        RT_GCC_EXTENSION uint64_t  u1Cache : 1;                   /**< Bit  101     - Cache: IOTLB Cache Hint. */
        RT_GCC_EXTENSION uint64_t  u1SnoopDisable : 1;            /**< Bit  102     - SD: Snoop Disable. */
        RT_GCC_EXTENSION uint64_t  u1AllowExclusion : 1;          /**< Bit  103     - EX: Allow Exclusion. */
        RT_GCC_EXTENSION uint64_t  u2SysMgt : 2;                  /**< Bits 105:104 - SysMgt: System Management message enable. */
        RT_GCC_EXTENSION uint64_t  u1Rsvd1 : 1;                   /**< Bit  106     - Reserved. */
        RT_GCC_EXTENSION uint64_t  u21GstCr3TableRootPtrHi : 21;  /**< Bits 127:107 - GCR3 TRP: Guest CR3 Table Root Ptr (Hi). */
        RT_GCC_EXTENSION uint64_t  u1IntrMapValid : 1;            /**< Bit  128     - IV: Interrupt map Valid. */
        RT_GCC_EXTENSION uint64_t  u4IntrTableLength : 4;         /**< Bits 132:129 - IntTabLen: Interrupt Table Length. */
        RT_GCC_EXTENSION uint64_t  u1IgnoreUnmappedIntrs : 1;     /**< Bits 133     - IG: Ignore unmapped interrupts. */
        RT_GCC_EXTENSION uint64_t  u46IntrTableRootPtr : 46;      /**< Bits 179:134 - Interrupt Root Table Pointer. */
        RT_GCC_EXTENSION uint64_t  u4Rsvd0 : 4;                   /**< Bits 183:180 - Reserved. */
        RT_GCC_EXTENSION uint64_t  u1InitPassthru : 1;            /**< Bits 184     - INIT Pass-through. */
        RT_GCC_EXTENSION uint64_t  u1ExtIntPassthru : 1;          /**< Bits 185     - External Interrupt Pass-through. */
        RT_GCC_EXTENSION uint64_t  u1NmiPassthru : 1;             /**< Bits 186     - NMI Pass-through. */
        RT_GCC_EXTENSION uint64_t  u1Rsvd2 : 1;                   /**< Bits 187     - Reserved. */
        RT_GCC_EXTENSION uint64_t  u2IntrCtrl : 2;                /**< Bits 189:188 - IntCtl: Interrupt Control. */
        RT_GCC_EXTENSION uint64_t  u1Lint0Passthru : 1;           /**< Bit  190     - Lint0Pass: LINT0 Pass-through. */
        RT_GCC_EXTENSION uint64_t  u1Lint1Passthru : 1;           /**< Bit  191     - Lint1Pass: LINT1 Pass-through. */
        RT_GCC_EXTENSION uint64_t  u32Rsvd0 : 32;                 /**< Bits 223:192 - Reserved. */
        RT_GCC_EXTENSION uint64_t  u22Rsvd0 : 22;                 /**< Bits 245:224 - Reserved. */
        RT_GCC_EXTENSION uint64_t  u1AttrOverride : 1;            /**< Bit  246     - AttrV: Attribute Override. */
        RT_GCC_EXTENSION uint64_t  u1Mode0FC : 1;                 /**< Bit  247     - Mode0FC. */
        RT_GCC_EXTENSION uint64_t  u8SnoopAttr : 8;               /**< Bits 255:248 - Snoop Attribute. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t        au32[8];
    /** The 64-bit unsigned integer view. */
    uint64_t        au64[4];
} DTE_T;
AssertCompileSize(DTE_T, 32);
/** Pointer to a device table entry. */
typedef DTE_T *PDTE_T;
/** Pointer to a const device table entry. */
typedef DTE_T const *PCDTE_T;

/** Mask of valid  bits for EPHSUP (Enhanced Peripheral Page Request Handling
 *  Support) feature (bits 52:53). */
#define IOMMU_DTE_QWORD_0_FEAT_EPHSUP_MASK              UINT64_C(0x0030000000000000)

/** Mask of valid bits for GTSup (Guest Translation Support) feature (bits 55:60,
 *  bits 80:95). */
#define IOMMU_DTE_QWORD_0_FEAT_GTSUP_MASK               UINT64_C(0x1f80000000000000)
#define IOMMU_DTE_QWORD_1_FEAT_GTSUP_MASK               UINT64_C(0x00000000ffff0000)

/** Mask of valid bits for GIoSup (Guest I/O Protection Support) feature (bit 54). */
#define IOMMU_DTE_QWORD_0_FEAT_GIOSUP_MASK              UINT64_C(0x0040000000000000)

/** Mask of valid DTE feature bits. */
#define IOMMU_DTE_QWORD_0_FEAT_MASK                     (  IOMMU_DTE_QWORD_0_FEAT_EPHSUP_MASK \
                                                         | IOMMU_DTE_QWORD_0_FEAT_GTSUP_MASK  \
                                                         | IOMMU_DTE_QWORD_0_FEAT_GIOSUP_MASK)
#define IOMMU_DTE_QWORD_1_FEAT_MASK                     IOMMU_DTE_QWORD_0_FEAT_GIOSUP_MASK

/** Mask of all valid DTE bits (including all feature bits). */
#define IOMMU_DTE_QWORD_0_VALID_MASK                    UINT64_C(0x7fffffffffffff83)
#define IOMMU_DTE_QWORD_1_VALID_MASK                    UINT64_C(0xfffffbffffffffff)
#define IOMMU_DTE_QWORD_2_VALID_MASK                    UINT64_C(0xff0fffffffffffff)
#define IOMMU_DTE_QWORD_3_VALID_MASK                    UINT64_C(0xffc0000000000000)

/** Mask of the interrupt table root pointer. */
#define IOMMU_DTE_IRTE_ROOT_PTR_MASK                    UINT64_C(0x000fffffffffffc0)
/** Number of bits to shift to get the interrupt root table pointer at
   qword 2 (qword 0 being the first one) - 128-byte aligned. */
#define IOMMU_DTE_IRTE_ROOT_PTR_SHIFT                   6

/** Maximum encoded IRTE length (exclusive). */
#define IOMMU_DTE_INTR_TAB_LEN_MAX                      12
/** Gets the interrupt table entries (in bytes) given the DTE pointer. */
#define IOMMU_DTE_GET_INTR_TAB_ENTRIES(a_pDte)          (UINT64_C(1) << (a_pDte)->n.u4IntrTableLength)
/** Gets the interrupt table length (in bytes) given the DTE pointer. */
#define IOMMU_DTE_GET_INTR_TAB_LEN(a_pDte)              (IOMMU_DTE_GET_INTR_TAB_ENTRIES(a_pDte) * sizeof(IRTE_T))
/** Mask of interrupt control bits. */
#define IOMMU_DTE_INTR_CTRL_MASK                        0x3
/** Gets the interrupt control bits from the DTE. */
#define IOMMU_DTE_GET_INTR_CTRL(a_pDte)                 (((a_pDte)->au64[2] >> 60) & IOMMU_DTE_INTR_CTRL_MASK)
/** Gets the ignore unmapped interrupt bit from DTE. */
#define IOMMU_DTE_GET_IG(a_pDte)                        (((a_pDte)->au64[2] >> 5) & 0x1)

/**
 * I/O Page Translation Entry.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t    u1Present : 1;             /**< Bit  0      - PR: Present. */
        RT_GCC_EXTENSION uint64_t    u4Ign0 : 4;                /**< Bits 4:1    - Ignored. */
        RT_GCC_EXTENSION uint64_t    u1Accessed : 1;            /**< Bit  5      - A: Accessed. */
        RT_GCC_EXTENSION uint64_t    u1Dirty : 1;               /**< Bit  6      - D: Dirty. */
        RT_GCC_EXTENSION uint64_t    u2Ign0 : 2;                /**< Bits 8:7    - Ignored. */
        RT_GCC_EXTENSION uint64_t    u3NextLevel : 3;           /**< Bits 11:9   - Next Level: Next page translation level. */
        RT_GCC_EXTENSION uint64_t    u40PageAddr : 40;          /**< Bits 51:12  - Page address. */
        RT_GCC_EXTENSION uint64_t    u7Rsvd0 : 7;               /**< Bits 58:52  - Reserved. */
        RT_GCC_EXTENSION uint64_t    u1UntranslatedAccess : 1;  /**< Bit 59      - U: Untranslated Access Only. */
        RT_GCC_EXTENSION uint64_t    u1ForceCoherent : 1;       /**< Bit 60      - FC: Force Coherent. */
        RT_GCC_EXTENSION uint64_t    u1IoRead : 1;              /**< Bit 61      - IR: I/O Read permission. */
        RT_GCC_EXTENSION uint64_t    u1IoWrite : 1;             /**< Bit 62      - IW: I/O Wead permission. */
        RT_GCC_EXTENSION uint64_t    u1Ign0 : 1;                /**< Bit 63      - Ignored. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t        u64;
} IOPTE_T;
AssertCompileSize(IOPTE_T, 8);

/**
 * I/O Page Directory Entry.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t    u1Present : 1;         /**< Bit  0      - PR: Present. */
        RT_GCC_EXTENSION uint64_t    u4Ign0 : 4;            /**< Bits 4:1    - Ignored. */
        RT_GCC_EXTENSION uint64_t    u1Accessed : 1;        /**< Bit  5      - A: Accessed. */
        RT_GCC_EXTENSION uint64_t    u3Ign0 : 3;            /**< Bits 8:6    - Ignored. */
        RT_GCC_EXTENSION uint64_t    u3NextLevel : 3;       /**< Bits 11:9   - Next Level: Next page translation level. */
        RT_GCC_EXTENSION uint64_t    u40PageAddr : 40;      /**< Bits 51:12  - Page address (Next Table Address). */
        RT_GCC_EXTENSION uint64_t    u9Rsvd0 : 9;           /**< Bits 60:52  - Reserved. */
        RT_GCC_EXTENSION uint64_t    u1IoRead : 1;          /**< Bit 61      - IR: I/O Read permission. */
        RT_GCC_EXTENSION uint64_t    u1IoWrite : 1;         /**< Bit 62      - IW: I/O Wead permission. */
        RT_GCC_EXTENSION uint64_t    u1Ign0 : 1;            /**< Bit 63      - Ignored. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t        u64;
} IOPDE_T;
AssertCompileSize(IOPDE_T, 8);

/**
 * I/O Page Table Entity.
 * In accordance with the AMD spec.
 *
 * This a common subset of an DTE.au64[0], PTE and PDE.
 * Named as an "entity" to avoid confusing it with PTE.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t    u1Present : 1;         /**< Bit  0      - PR: Present. */
        RT_GCC_EXTENSION uint64_t    u8Ign0 : 8;            /**< Bits 8:1    - Ignored. */
        RT_GCC_EXTENSION uint64_t    u3NextLevel : 3;       /**< Bits 11:9   - Mode / Next Level: Next page translation level. */
        RT_GCC_EXTENSION uint64_t    u40Addr : 40;          /**< Bits 51:12  - Page address. */
        RT_GCC_EXTENSION uint64_t    u9Ign0 : 9;            /**< Bits 60:52  - Ignored. */
        RT_GCC_EXTENSION uint64_t    u1IoRead : 1;          /**< Bit 61      - IR: I/O Read permission. */
        RT_GCC_EXTENSION uint64_t    u1IoWrite : 1;         /**< Bit 62      - IW: I/O Wead permission. */
        RT_GCC_EXTENSION uint64_t    u1Ign0 : 1;            /**< Bit 63      - Ignored. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t        u64;
} IOPTENTITY_T;
AssertCompileSize(IOPTENTITY_T, 8);
AssertCompile(sizeof(IOPTENTITY_T) == sizeof(IOPTE_T));
AssertCompile(sizeof(IOPTENTITY_T) == sizeof(IOPDE_T));
/** Pointer to an IOPT_ENTITY_T struct. */
typedef IOPTENTITY_T *PIOPTENTITY_T;
/** Pointer to a const IOPT_ENTITY_T struct. */
typedef IOPTENTITY_T const *PCIOPTENTITY_T;
/** Mask of the address field. */
#define IOMMU_PTENTITY_ADDR_MASK                UINT64_C(0x000ffffffffff000)
/** Reserved bits in the PDE (bits 60:52). */
#define IOMMU_PDE_RSVD_MASK                     UINT64_C(0x1ff0000000000000)
/** Reserved bits in the PTE (bits 58:52 - U, FC bits not reserved). */
#define IOMMU_PTE_RSVD_MASK                     UINT64_C(0x07f0000000000000)

/**
 * Interrupt Remapping Table Entry (IRTE) - Basic Format.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u1RemapEnable : 1;      /**< Bit  0     - RemapEn: Remap Enable. */
        uint32_t    u1SuppressIoPf : 1;     /**< Bit  1     - SupIOPF: Suppress I/O Page Fault. */
        uint32_t    u3IntrType : 3;         /**< Bits 4:2   - IntType: Interrupt Type. */
        uint32_t    u1ReqEoi : 1;           /**< Bit  5     - RqEoi: Request EOI. */
        uint32_t    u1DestMode : 1;         /**< Bit  6     - DM: Destination Mode. */
        uint32_t    u1GuestMode : 1;        /**< Bit  7     - GuestMode. */
        uint32_t    u8Dest : 8;             /**< Bits 15:8  - Destination. */
        uint32_t    u8Vector : 8;           /**< Bits 23:16 - Vector. */
        uint32_t    u8Rsvd0 : 8;            /**< Bits 31:24 - Reserved. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t        u32;
} IRTE_T;
AssertCompileSize(IRTE_T, 4);
/** Pointer to an IRTE_T struct. */
typedef IRTE_T *PIRTE_T;
/** Pointer to a const IRTE_T struct. */
typedef IRTE_T const *PCIRTE_T;

/** The IRTE offset corresponds directly to bits 10:0 of the originating MSI
 *  interrupt message. See AMD IOMMU spec. 2.2.5 "Interrupt Remapping Tables". */
#define IOMMU_MSI_DATA_IRTE_OFFSET_MASK     UINT32_C(0x000007ff)
/** Gets the IRTE offset from the originating MSI interrupt message. */
#define IOMMU_GET_IRTE_OFF(a_u32MsiData)    (((a_u32MsiData) & IOMMU_MSI_DATA_IRTE_OFFSET_MASK) * sizeof(IRTE_T))

/**
 * Interrupt Remapping Table Entry (IRTE) - Guest Virtual APIC Enabled.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u1RemapEnable : 1;          /**< Bit  0       - RemapEn: Remap Enable. */
        uint32_t    u1SuppressIoPf : 1;         /**< Bit  1       - SupIOPF: Suppress I/O Page Fault. */
        uint32_t    u1GALogIntr : 1;            /**< Bit  2       - GALogIntr: Guest APIC Log Interrupt. */
        uint32_t    u3Rsvd : 3;                 /**< Bits 5:3     - Reserved. */
        uint32_t    u1IsRunning : 1;            /**< Bit  6       - IsRun: Hint whether the guest is running. */
        uint32_t    u1GuestMode : 1;            /**< Bit  7       - GuestMode. */
        uint32_t    u8Dest : 8;                 /**< Bits 15:8    - Destination. */
        uint32_t    u8Rsvd0 : 8;                /**< Bits 31:16   - Reserved. */
        uint32_t    u32GATag : 32;              /**< Bits 63:31   - GATag: Tag used when writing to GA log. */
        uint32_t    u8Vector : 8;               /**< Bits 71:64   - Vector: Interrupt vector. */
        uint32_t    u4Reserved : 4;             /**< Bits 75:72   - Reserved or ignored depending on RemapEn. */
        uint32_t    u20GATableRootPtrLo : 20;   /**< Bits 95:76   - Bits [31:12] of Guest vAPIC Table Root Pointer. */
        uint32_t    u20GATableRootPtrHi : 20;   /**< Bits 115:76  - Bits [51:32] of Guest vAPIC Table Root Pointer. */
        uint32_t    u12Rsvd : 12;               /**< Bits 127:116 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t        u64[2];
} IRTE_GVA_T;
AssertCompileSize(IRTE_GVA_T, 16);
/** Pointer to an IRTE_GVA_T struct. */
typedef IRTE_GVA_T *PIRTE_GVA_T;
/** Pointer to a const IRTE_GVA_T struct. */
typedef IRTE_GVA_T const *PCIRTE_GVA_T;

/**
 * Command: Generic Command Buffer Entry.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u32Operand1Lo;          /**< Bits 31:0   - Operand 1 (Lo). */
        uint32_t    u28Operand1Hi : 28;     /**< Bits 59:32  - Operand 1 (Hi). */
        uint32_t    u4Opcode : 4;           /**< Bits 63:60  - Op Code. */
        uint64_t    u64Operand2;            /**< Bits 127:64 - Operand 2. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} CMD_GENERIC_T;
AssertCompileSize(CMD_GENERIC_T, 16);
/** Pointer to a generic command buffer entry. */
typedef CMD_GENERIC_T *PCMD_GENERIC_T;
/** Pointer to a const generic command buffer entry. */
typedef CMD_GENERIC_T const *PCCMD_GENERIC_T;

/** Number of bits to shift the byte offset of a command in the command buffer to
 *  get its index. */
#define IOMMU_CMD_GENERIC_SHIFT   4

/**
 * Command: COMPLETION_WAIT.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u1Store : 1;           /**< Bit  0      - S: Completion Store. */
        uint32_t    u1Interrupt : 1;       /**< Bit  1      - I: Completion Interrupt. */
        uint32_t    u1Flush : 1;           /**< Bit  2      - F: Flush Queue. */
        uint32_t    u29StoreAddrLo : 29;   /**< Bits 31:3   - Store Address (Lo). */
        uint32_t    u20StoreAddrHi : 20;   /**< Bits 51:32  - Store Address (Hi). */
        uint32_t    u8Rsvd0 : 8;           /**< Bits 59:52  - Reserved. */
        uint32_t    u4OpCode : 4;          /**< Bits 63:60  - OpCode (Command). */
        uint64_t    u64StoreData;          /**< Bits 127:64 - Store Data. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} CMD_COMWAIT_T;
AssertCompileSize(CMD_COMWAIT_T, 16);
/** Pointer to a completion wait command. */
typedef CMD_COMWAIT_T *PCMD_COMWAIT_T;
/** Pointer to a const completion wait command. */
typedef CMD_COMWAIT_T const *PCCMD_COMWAIT_T;
#define IOMMU_CMD_COM_WAIT_QWORD_0_VALID_MASK        UINT64_C(0xf00fffffffffffff)

/**
 * Command: INVALIDATE_DEVTAB_ENTRY.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint16_t    u16DevId;               /**< Bits 15:0   - Device ID. */
        uint16_t    u16Rsvd0;               /**< Bits 31:16  - Reserved. */
        uint32_t    u28Rsvd0 : 28;          /**< Bits 59:32  - Reserved. */
        uint32_t    u4OpCode : 4;           /**< Bits 63:60  - Op Code (Command). */
        uint64_t    u64Rsvd0;               /**< Bits 127:64 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} CMD_INV_DTE_T;
AssertCompileSize(CMD_INV_DTE_T, 16);
/** Pointer to a invalidate DTE command. */
typedef CMD_INV_DTE_T *PCMD_INV_DTE_T;
/** Pointer to a const invalidate DTE command. */
typedef CMD_INV_DTE_T const *PCCMD_INV_DTE_T;
#define IOMMU_CMD_INV_DTE_QWORD_0_VALID_MASK        UINT64_C(0xf00000000000ffff)
#define IOMMU_CMD_INV_DTE_QWORD_1_VALID_MASK        UINT64_C(0x0000000000000000)

/**
 * Command: INVALIDATE_IOMMU_PAGES.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u20Pasid : 20;          /**< Bits 19:0   - PASID: Process Address-Space ID. */
        uint32_t    u12Rsvd0 : 12;          /**< Bits 31:20  - Reserved. */
        uint32_t    u16DomainId : 16;       /**< Bits 47:32  - Domain ID. */
        uint32_t    u12Rsvd1 : 12;          /**< Bits 59:48  - Reserved. */
        uint32_t    u4OpCode : 4;           /**< Bits 63:60  - Op Code (Command). */
        uint32_t    u1Size : 1;             /**< Bit  64     - S: Size. */
        uint32_t    u1PageDirEntries : 1;   /**< Bit  65     - PDE: Page Directory Entries. */
        uint32_t    u1GuestOrNested : 1;    /**< Bit  66     - GN: Guest (GPA) or Nested (GVA). */
        uint32_t    u9Rsvd0 : 9;            /**< Bits 75:67  - Reserved. */
        uint32_t    u20AddrLo : 20;         /**< Bits 95:76  - Address (Lo). */
        uint32_t    u32AddrHi;              /**< Bits 127:96 - Address (Hi). */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} CMD_INV_IOMMU_PAGES_T;
AssertCompileSize(CMD_INV_IOMMU_PAGES_T, 16);
/** Pointer to a invalidate iommu pages command. */
typedef CMD_INV_IOMMU_PAGES_T *PCMD_INV_IOMMU_PAGES_T;
/** Pointer to a const invalidate iommu pages command. */
typedef CMD_INV_IOMMU_PAGES_T const *PCCMD_INV_IOMMU_PAGES_T;
#define IOMMU_CMD_INV_IOMMU_PAGES_QWORD_0_VALID_MASK        UINT64_C(0xf000ffff000fffff)
#define IOMMU_CMD_INV_IOMMU_PAGES_QWORD_1_VALID_MASK        UINT64_C(0xfffffffffffff007)

/**
 * Command: INVALIDATE_IOTLB_PAGES.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint16_t    u16DevId;               /**< Bits 15:0   - Device ID. */
        uint8_t     u8PasidLo;              /**< Bits 23:16  - PASID: Process Address-Space ID (Lo). */
        uint8_t     u8MaxPend;              /**< Bits 31:24  - Maxpend: Maximum simultaneous in-flight transactions. */
        uint32_t    u16QueueId : 16;        /**< Bits 47:32  - Queue ID. */
        uint32_t    u12PasidHi : 12;        /**< Bits 59:48  - PASID: Process Address-Space ID (Hi). */
        uint32_t    u4OpCode : 4;           /**< Bits 63:60  - Op Code (Command). */
        uint32_t    u1Size : 1;             /**< Bit  64     - S: Size. */
        uint32_t    u1Rsvd0: 1;             /**< Bit  65     - Reserved. */
        uint32_t    u1GuestOrNested : 1;    /**< Bit  66     - GN: Guest (GPA) or Nested (GVA). */
        uint32_t    u1Rsvd1 : 1;            /**< Bit  67     - Reserved. */
        uint32_t    u2Type : 2;             /**< Bit  69:68  - Type. */
        uint32_t    u6Rsvd0 : 6;            /**< Bits 75:70  - Reserved. */
        uint32_t    u20AddrLo : 20;         /**< Bits 95:76  - Address (Lo). */
        uint32_t    u32AddrHi;              /**< Bits 127:96 - Address (Hi). */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} CMD_INV_IOTLB_PAGES_T;
AssertCompileSize(CMD_INV_IOTLB_PAGES_T, 16);

/**
 * Command: INVALIDATE_INTR_TABLE.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint16_t    u16DevId;           /**< Bits 15:0   - Device ID. */
        uint16_t    u16Rsvd0;           /**< Bits 31:16  - Reserved. */
        uint32_t    u32Rsvd0 : 28;      /**< Bits 59:32  - Reserved. */
        uint32_t    u4OpCode : 4;       /**< Bits 63:60  - Op Code (Command). */
        uint64_t    u64Rsvd0;           /**< Bits 127:64 - Reserved. */
    } u;
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} CMD_INV_INTR_TABLE_T;
AssertCompileSize(CMD_INV_INTR_TABLE_T, 16);
/** Pointer to a invalidate interrupt table command. */
typedef CMD_INV_INTR_TABLE_T *PCMD_INV_INTR_TABLE_T;
/** Pointer to a const invalidate interrupt table command. */
typedef CMD_INV_INTR_TABLE_T const *PCCMD_INV_INTR_TABLE_T;
#define IOMMU_CMD_INV_INTR_TABLE_QWORD_0_VALID_MASK         UINT64_C(0xf00000000000ffff)
#define IOMMU_CMD_INV_INTR_TABLE_QWORD_1_VALID_MASK         UINT64_C(0x0000000000000000)

/**
 * Command: PREFETCH_IOMMU_PAGES.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint16_t    u16DevId;               /**< Bits 15:0   - Device ID. */
        uint8_t     u8Rsvd0;                /**< Bits 23:16  - Reserved. */
        uint8_t     u8PrefCount;            /**< Bits 31:24  - PFCount: Number of translations to prefetch. */
        uint32_t    u20Pasid : 20;          /**< Bits 51:32  - PASID: Process Address-Space ID. */
        uint32_t    u8Rsvd1 : 8;            /**< Bits 59:52  - Reserved. */
        uint32_t    u4OpCode : 4;           /**< Bits 63:60  - Op Code (Command). */
        uint32_t    u1Size : 1;             /**< Bit  64     - S: Size of the prefetched pages. */
        uint32_t    u1Rsvd0 : 1;            /**< Bit  65     - Reserved. */
        uint32_t    u1GuestOrNested : 1;    /**< Bit  66     - GN: Guest (GPA) or Nested (GVA). */
        uint32_t    u1Rsvd1 : 1;            /**< Bit  67     - Reserved. */
        uint32_t    u1Invalidate : 1;       /**< Bit  68     - Inval: Invalidate prior to prefetch. */
        uint32_t    u7Rsvd0 : 7;            /**< Bits 75:69  - Reserved */
        uint32_t    u20AddrLo : 7;          /**< Bits 95:76  - Address (Lo). */
        uint32_t    u32AddrHi;              /**< Bits 127:96 - Address (Hi). */
    } u;
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} CMD_PREF_IOMMU_PAGES_T;
AssertCompileSize(CMD_PREF_IOMMU_PAGES_T, 16);
/** Pointer to a invalidate iommu pages command. */
typedef CMD_PREF_IOMMU_PAGES_T *PCMD_PREF_IOMMU_PAGES_T;
/** Pointer to a const invalidate iommu pages command. */
typedef CMD_PREF_IOMMU_PAGES_T const *PCCMD_PREF_IOMMU_PAGES_T;
#define IOMMU_CMD_PREF_IOMMU_PAGES_QWORD_0_VALID_MASK       UINT64_C(0x780fffffff00ffff)
#define IOMMU_CMD_PREF_IOMMU_PAGES_QWORD_1_VALID_MASK       UINT64_C(0xfffffffffffff015)


/**
 * Command: COMPLETE_PPR_REQ.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint16_t    u16DevId;               /**< Bits 15:0    - Device ID. */
        uint16_t    u16Rsvd0;               /**< Bits 31:16   - Reserved. */
        uint32_t    u20Pasid : 20;          /**< Bits 51:32   - PASID: Process Address-Space ID. */
        uint32_t    u8Rsvd0 : 8;            /**< Bits 59:52   - Reserved. */
        uint32_t    u4OpCode : 4;           /**< Bits 63:60   - Op Code (Command). */
        uint32_t    u2Rsvd0 : 2;            /**< Bits 65:64   - Reserved. */
        uint32_t    u1GuestOrNested : 1;    /**< Bit  66      - GN: Guest (GPA) or Nested (GVA). */
        uint32_t    u29Rsvd0 : 29;          /**< Bits 95:67   - Reserved. */
        uint32_t    u16CompletionTag : 16;  /**< Bits 111:96  - Completion Tag. */
        uint32_t    u16Rsvd1 : 16;          /**< Bits 127:112 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} CMD_COMPLETE_PPR_REQ_T;
AssertCompileSize(CMD_COMPLETE_PPR_REQ_T, 16);

/**
 * Command: INV_IOMMU_ALL.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u32Rsvd0;           /**< Bits 31:0   - Reserved. */
        uint32_t    u28Rsvd0 : 28;      /**< Bits 59:32  - Reserved. */
        uint32_t    u4OpCode : 4;       /**< Bits 63:60  - Op Code (Command). */
        uint64_t    u64Rsvd0;           /**< Bits 127:64 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} CMD_INV_IOMMU_ALL_T;
AssertCompileSize(CMD_INV_IOMMU_ALL_T, 16);
/** Pointer to a invalidate IOMMU all command. */
typedef CMD_INV_IOMMU_ALL_T *PCMD_INV_IOMMU_ALL_T;
/** Pointer to a const invalidate IOMMU all command. */
typedef CMD_INV_IOMMU_ALL_T const *PCCMD_INV_IOMMU_ALL_T;
#define IOMMU_CMD_INV_IOMMU_ALL_QWORD_0_VALID_MASK          UINT64_C(0xf000000000000000)
#define IOMMU_CMD_INV_IOMMU_ALL_QWORD_1_VALID_MASK          UINT64_C(0x0000000000000000)

/**
 * Event Log Entry: Generic.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u32Operand1Lo;          /**< Bits 31:0   - Operand 1 (Lo). */
        uint32_t    u28Operand1Hi : 28;     /**< Bits 59:32  - Operand 1 (Hi). */
        uint32_t    u4EvtCode : 4;          /**< Bits 63:60  - Event code. */
        uint32_t    u32Operand2Lo;          /**< Bits 95:64  - Operand 2 (Lo). */
        uint32_t    u32Operand2Hi;          /**< Bits 127:96 - Operand 2 (Hi). */
    } n;
    /** The 32-bit unsigned integer view.  */
    uint32_t    au32[4];
} EVT_GENERIC_T;
AssertCompileSize(EVT_GENERIC_T, 16);
/** Number of bits to shift the byte offset of an event entry in the event log
 *  buffer to get its index. */
#define IOMMU_EVT_GENERIC_SHIFT   4
/** Pointer to a generic event log entry. */
typedef EVT_GENERIC_T *PEVT_GENERIC_T;
/** Pointer to a const generic event log entry. */
typedef const EVT_GENERIC_T *PCEVT_GENERIC_T;

/**
 * Hardware event types.
 * In accordance with the AMD spec.
 */
typedef enum HWEVTTYPE
{
    HWEVTTYPE_RSVD = 0,
    HWEVTTYPE_MASTER_ABORT,
    HWEVTTYPE_TARGET_ABORT,
    HWEVTTYPE_DATA_ERROR
} HWEVTTYPE;
AssertCompileSize(HWEVTTYPE, 4);

/**
 * Event Log Entry: ILLEGAL_DEV_TABLE_ENTRY.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
                         uint16_t  u16DevId;            /**< Bits 15:0   - Device ID. */
        RT_GCC_EXTENSION uint16_t  u4PasidHi : 4;       /**< Bits 19:16  - PASID: Process Address-Space ID (Hi). */
        RT_GCC_EXTENSION uint16_t  u12Rsvd0 : 12;       /**< Bits 31:20  - Reserved. */
                         uint16_t  u16PasidLo;          /**< Bits 47:32  - PASID: Process Address-Space ID (Lo). */
        RT_GCC_EXTENSION uint16_t  u1GuestOrNested : 1; /**< Bit  48     - GN: Guest (GPA) or Nested (GVA). */
        RT_GCC_EXTENSION uint16_t  u2Rsvd0 : 2;         /**< Bits 50:49  - Reserved. */
        RT_GCC_EXTENSION uint16_t  u1Interrupt : 1;     /**< Bit  51     - I: Interrupt. */
        RT_GCC_EXTENSION uint16_t  u1Rsvd0 : 1;         /**< Bit  52     - Reserved. */
        RT_GCC_EXTENSION uint16_t  u1ReadWrite : 1;     /**< Bit  53     - RW: Read/Write. */
        RT_GCC_EXTENSION uint16_t  u1Rsvd1 : 1;         /**< Bit  54     - Reserved. */
        RT_GCC_EXTENSION uint16_t  u1RsvdNotZero : 1;   /**< Bit  55     - RZ: Reserved bit not Zero (0=invalid level encoding). */
        RT_GCC_EXTENSION uint16_t  u1Translation : 1;   /**< Bit  56     - TN: Translation. */
        RT_GCC_EXTENSION uint16_t  u3Rsvd0 : 3;         /**< Bits 59:57  - Reserved. */
        RT_GCC_EXTENSION uint16_t  u4EvtCode : 4;       /**< Bits 63:60  - Event code. */
                         uint64_t  u64Addr;             /**< Bits 127:64 - Address: I/O Virtual Address (IOVA). */
    } n;
    /** The 32-bit unsigned integer view.  */
    uint32_t    au32[4];
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} EVT_ILLEGAL_DTE_T;
AssertCompileSize(EVT_ILLEGAL_DTE_T, 16);
/** Pointer to an illegal device table entry event. */
typedef EVT_ILLEGAL_DTE_T *PEVT_ILLEGAL_DTE_T;
/** Pointer to a const illegal device table entry event. */
typedef EVT_ILLEGAL_DTE_T const *PCEVT_ILLEGAL_DTE_T;

/**
 * Event Log Entry: IO_PAGE_FAULT_EVENT.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
                         uint16_t  u16DevId;               /**< Bits 15:0   - Device ID. */
        RT_GCC_EXTENSION uint16_t  u4PasidHi : 4;          /**< Bits 19:16  - PASID: Process Address-Space ID (Hi). */
        RT_GCC_EXTENSION uint16_t  u16DomainOrPasidLo;     /**< Bits 47:32  - D/P: Domain ID or Process Address-Space ID (Lo). */
        RT_GCC_EXTENSION uint16_t  u1GuestOrNested : 1;    /**< Bit  48     - GN: Guest (GPA) or Nested (GVA). */
        RT_GCC_EXTENSION uint16_t  u1NoExecute : 1;        /**< Bit  49     - NX: No Execute. */
        RT_GCC_EXTENSION uint16_t  u1User : 1;             /**< Bit  50     - US: User/Supervisor. */
        RT_GCC_EXTENSION uint16_t  u1Interrupt : 1;        /**< Bit  51     - I: Interrupt. */
        RT_GCC_EXTENSION uint16_t  u1Present : 1;          /**< Bit  52     - PR: Present. */
        RT_GCC_EXTENSION uint16_t  u1ReadWrite : 1;        /**< Bit  53     - RW: Read/Write. */
        RT_GCC_EXTENSION uint16_t  u1PermDenied : 1;       /**< Bit  54     - PE: Permission Indicator. */
        RT_GCC_EXTENSION uint16_t  u1RsvdNotZero : 1;      /**< Bit  55     - RZ: Reserved bit not Zero (0=invalid level encoding). */
        RT_GCC_EXTENSION uint16_t  u1Translation : 1;      /**< Bit  56     - TN: Translation. */
        RT_GCC_EXTENSION uint16_t  u3Rsvd0 : 3;            /**< Bit  59:57  - Reserved. */
        RT_GCC_EXTENSION uint16_t  u4EvtCode : 4;          /**< Bits 63:60  - Event code. */
                         uint64_t  u64Addr;                /**< Bits 127:64 - Address: I/O Virtual Address (IOVA). */
    } n;
    /** The 32-bit unsigned integer view.  */
    uint32_t    au32[4];
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} EVT_IO_PAGE_FAULT_T;
AssertCompileSize(EVT_IO_PAGE_FAULT_T, 16);
/** Pointer to an I/O page fault event. */
typedef EVT_IO_PAGE_FAULT_T *PEVT_IO_PAGE_FAULT_T;
/** Pointer to a const I/O page fault event. */
typedef EVT_IO_PAGE_FAULT_T const *PCEVT_IO_PAGE_FAULT_T;


/**
 * Event Log Entry: DEV_TAB_HARDWARE_ERROR.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint16_t    u16DevId;               /**< Bits 15:0   - Device ID. */
        uint16_t    u16Rsvd0;               /**< Bits 31:16  - Reserved. */
        uint32_t    u19Rsvd0 : 19;          /**< Bits 50:32  - Reserved. */
        uint32_t    u1Intr : 1;             /**< Bit  51     - I: Interrupt (1=interrupt request, 0=memory request). */
        uint32_t    u1Rsvd0 : 1;            /**< Bit  52     - Reserved. */
        uint32_t    u1ReadWrite : 1;        /**< Bit  53     - RW: Read/Write transaction (only meaninful when I=0 and TR=0). */
        uint32_t    u2Rsvd0 : 2;            /**< Bits 55:54  - Reserved. */
        uint32_t    u1Translation : 1;      /**< Bit  56     - TR: Translation (1=translation, 0=transaction). */
        uint32_t    u2Type : 2;             /**< Bits 58:57  - Type: The type of hardware error. */
        uint32_t    u1Rsvd1 : 1;            /**< Bit  59     - Reserved. */
        uint32_t    u4EvtCode : 4;          /**< Bits 63:60  - Event code. */
        uint64_t    u64Addr;                /**< Bits 127:64 - Address. */
    } n;
    /** The 32-bit unsigned integer view.  */
    uint32_t    au32[4];
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} EVT_DEV_TAB_HW_ERROR_T;
AssertCompileSize(EVT_DEV_TAB_HW_ERROR_T, 16);
/** Pointer to a device table hardware error event. */
typedef EVT_DEV_TAB_HW_ERROR_T *PEVT_DEV_TAB_HW_ERROR_T;
/** Pointer to a const device table hardware error event. */
typedef EVT_DEV_TAB_HW_ERROR_T const *PCEVT_DEV_TAB_HW_ERROR_T;

/**
 * Event Log Entry: EVT_PAGE_TAB_HARDWARE_ERROR.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint16_t    u16DevId;                   /**< Bits 15:0   - Device ID. */
        uint16_t    u16Rsvd0;                   /**< Bits 31:16  - Reserved. */
        uint32_t    u16DomainOrPasidLo : 16;    /**< Bits 47:32  - D/P: Domain ID or Process Address-Space ID (Lo). */
        uint32_t    u1GuestOrNested : 1;        /**< Bit  48     - GN: Guest (GPA) or Nested (GVA). */
        uint32_t    u2Rsvd0 : 2;                /**< Bits 50:49  - Reserved. */
        uint32_t    u1Interrupt : 1;            /**< Bit  51     - I: Interrupt. */
        uint32_t    u1Rsvd0 : 1;                /**< Bit  52     - Reserved. */
        uint32_t    u1ReadWrite : 1;            /**< Bit  53     - RW: Read/Write. */
        uint32_t    u2Rsvd1 : 2;                /**< Bit  55:54  - Reserved. */
        uint32_t    u1Translation : 1;          /**< Bit  56     - TR: Translation. */
        uint32_t    u2Type : 2;                 /**< Bits 58:57  - Type: The type of hardware error. */
        uint32_t    u1Rsvd1 : 1;                /**< Bit  59     - Reserved. */
        uint32_t    u4EvtCode : 4;              /**< Bit  63:60  - Event code. */
        /** @todo r=ramshankar: Figure 55: PAGE_TAB_HARDWARE_ERROR says Addr[31:3] but
         *        table 58 mentions Addr[31:4], we just use the full 64-bits. Looks like a
         *        typo in the figure.See AMD AMD IOMMU spec (3.05-PUB, Jan 2020). */
        uint64_t    u64Addr;                    /** Bits 127:64  - Address: SPA of the page table entry. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    au32[4];
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} EVT_PAGE_TAB_HW_ERR_T;
AssertCompileSize(EVT_PAGE_TAB_HW_ERR_T, 16);
/** Pointer to a page table hardware error event. */
typedef EVT_PAGE_TAB_HW_ERR_T *PEVT_PAGE_TAB_HW_ERR_T;
/** Pointer to a const page table hardware error event. */
typedef EVT_PAGE_TAB_HW_ERR_T const *PCEVT_PAGE_TAB_HW_ERR_T;

/**
 * Event Log Entry: ILLEGAL_COMMAND_ERROR.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u32Rsvd0;           /**< Bits 31:0   - Reserved. */
        uint32_t    u28Rsvd0 : 28;      /**< Bits 47:32  - Reserved. */
        uint32_t    u4EvtCode : 4;      /**< Bits 63:60  - Event code. */
        uint64_t    u64Addr;            /**< Bits 127:64 - Address: SPA of the invalid command. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    au32[4];
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} EVT_ILLEGAL_CMD_ERR_T;
AssertCompileSize(EVT_ILLEGAL_CMD_ERR_T, 16);
/** Pointer to an illegal command error event. */
typedef EVT_ILLEGAL_CMD_ERR_T *PEVT_ILLEGAL_CMD_ERR_T;
/** Pointer to a const illegal command error event. */
typedef EVT_ILLEGAL_CMD_ERR_T const *PCEVT_ILLEGAL_CMD_ERR_T;

/**
 * Event Log Entry: COMMAND_HARDWARE_ERROR.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u32Rsvd0;           /**< Bits 31:0   - Reserved. */
        uint32_t    u25Rsvd1 : 25;      /**< Bits 56:32  - Reserved. */
        uint32_t    u2Type : 2;         /**< Bits 58:57  - Type: The type of hardware error. */
        uint32_t    u1Rsvd1 : 1;        /**< Bit  59     - Reserved. */
        uint32_t    u4EvtCode : 4;      /**< Bits 63:60  - Event code. */
        uint64_t    u64Addr;            /**< Bits 128:64 - Address: SPA of the attempted access. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    au32[4];
    /** The 64-bit unsigned integer view. */
    uint64_t    au64[2];
} EVT_CMD_HW_ERR_T;
AssertCompileSize(EVT_CMD_HW_ERR_T, 16);
/** Pointer to a command hardware error event. */
typedef EVT_CMD_HW_ERR_T *PEVT_CMD_HW_ERR_T;
/** Pointer to a const command hardware error event. */
typedef EVT_CMD_HW_ERR_T const *PCEVT_CMD_HW_ERR_T;

/**
 * Event Log Entry: IOTLB_INV_TIMEOUT.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint16_t    u16DevId;           /**< Bits 15:0   - Device ID. */
        uint16_t    u16Rsvd0;           /**< Bits 31:16  - Reserved.*/
        uint32_t    u28Rsvd0 : 28;      /**< Bits 59:32  - Reserved. */
        uint32_t    u4EvtCode : 4;      /**< Bits 63:60  - Event code. */
        uint32_t    u4Rsvd0 : 4;        /**< Bits 67:64  - Reserved. */
        uint32_t    u28AddrLo : 28;     /**< Bits 95:68  - Address: SPA of the invalidation command that timedout (Lo). */
        uint32_t    u32AddrHi;          /**< Bits 127:96 - Address: SPA of the invalidation command that timedout (Hi). */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    au32[4];
} EVT_IOTLB_INV_TIMEOUT_T;
AssertCompileSize(EVT_IOTLB_INV_TIMEOUT_T, 16);

/**
 * Event Log Entry: INVALID_DEVICE_REQUEST.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u16DevId : 16;          /***< Bits 15:0   - Device ID. */
        uint32_t    u4PasidHi : 4;          /***< Bits 19:16  - PASID: Process Address-Space ID (Hi). */
        uint32_t    u12Rsvd0 : 12;          /***< Bits 31:20  - Reserved. */
        uint32_t    u16PasidLo : 16;        /***< Bits 47:32  - PASID: Process Address-Space ID (Lo). */
        uint32_t    u1GuestOrNested : 1;    /***< Bit  48     - GN: Guest (GPA) or Nested (GVA). */
        uint32_t    u1User : 1;             /***< Bit  49     - US: User/Supervisor. */
        uint32_t    u6Rsvd0 : 6;            /***< Bits 55:50  - Reserved. */
        uint32_t    u1Translation: 1;       /***< Bit  56     - TR: Translation. */
        uint32_t    u3Type: 3;              /***< Bits 59:57  - Type: The type of hardware error. */
        uint32_t    u4EvtCode : 4;          /***< Bits 63:60  - Event code. */
        uint64_t    u64Addr;                /***< Bits 127:64 - Address: Translation or access address. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    au32[4];
} EVT_INVALID_DEV_REQ_T;
AssertCompileSize(EVT_INVALID_DEV_REQ_T, 16);

/**
 * Event Log Entry: EVENT_COUNTER_ZERO.
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u32Rsvd0;               /**< Bits 31:0   - Reserved. */
        uint32_t    u28Rsvd0 : 28;          /**< Bits 59:32  - Reserved. */
        uint32_t    u4EvtCode : 4;          /**< Bits 63:60  - Event code. */
        uint32_t    u20CounterNoteHi : 20;  /**< Bits 83:64  - CounterNote: Counter value for the event counter register (Hi). */
        uint32_t    u12Rsvd0 : 12;          /**< Bits 95:84  - Reserved. */
        uint32_t    u32CounterNoteLo;       /**< Bits 127:96 - CounterNote: Counter value for the event cuonter register (Lo). */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    au32[4];
} EVT_EVENT_COUNTER_ZERO_T;
AssertCompileSize(EVT_EVENT_COUNTER_ZERO_T, 16);

/**
 * IOMMU Capability Header (PCI).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u8CapId : 8;        /**< Bits 7:0   - CapId: Capability ID. */
        uint32_t    u8CapPtr : 8;       /**< Bits 15:8  - CapPtr: Pointer (PCI config offset) to the next capability. */
        uint32_t    u3CapType : 3;      /**< Bits 18:16 - CapType: Capability Type. */
        uint32_t    u5CapRev : 5;       /**< Bits 23:19 - CapRev: Capability revision. */
        uint32_t    u1IoTlbSup : 1;     /**< Bit  24    - IotlbSup: IOTLB Support. */
        uint32_t    u1HtTunnel : 1;     /**< Bit  25    - HtTunnel: HyperTransport Tunnel translation support. */
        uint32_t    u1NpCache : 1;      /**< Bit  26    - NpCache: Not Present table entries are cached. */
        uint32_t    u1EfrSup : 1;       /**< Bit  27    - EFRSup: Extended Feature Register Support. */
        uint32_t    u1CapExt : 1;       /**< Bit  28    - CapExt: Misc. Information Register 1 Support. */
        uint32_t    u3Rsvd0 : 3;        /**< Bits 31:29 - Reserved. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    u32;
} IOMMU_CAP_HDR_T;
AssertCompileSize(IOMMU_CAP_HDR_T, 4);

/**
 * IOMMU Base Address (Lo and Hi) Register (PCI).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t   u1Enable : 1;       /**< Bit  1     - Enable: RW1S - Enable IOMMU MMIO region. */
        uint32_t   u12Rsvd0 : 12;      /**< Bits 13:1  - Reserved. */
        uint32_t   u18BaseAddrLo : 18; /**< Bits 31:14 - Base address (Lo) of the MMIO region. */
        uint32_t   u32BaseAddrHi;      /**< Bits 63:32 - Base address (Hi) of the MMIO region. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    au32[2];
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} IOMMU_BAR_T;
AssertCompileSize(IOMMU_BAR_T, 8);
#define IOMMU_BAR_VALID_MASK        UINT64_C(0xffffffffffffc001)

/**
 * IOMMU Range Register (PCI).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u5HtUnitId : 5;     /**< Bits 4:0   - UnitID: IOMMU HyperTransport Unit ID (not used). */
        uint32_t    u2Rsvd0 : 2;        /**< Bits 6:5   - Reserved. */
        uint32_t    u1RangeValid : 1;   /**< Bit  7     - RngValid: Range Valid. */
        uint32_t    u8Bus : 8;          /**< Bits 15:8  - BusNumber: Bus number of the first and last device. */
        uint32_t    u8FirstDevice : 8;  /**< Bits 23:16 - FirstDevice: Device and function number of the first device. */
        uint32_t    u8LastDevice: 8;    /**< Bits 31:24 - LastDevice: Device and function number of the last device. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    u32;
} IOMMU_RANGE_T;
AssertCompileSize(IOMMU_RANGE_T, 4);

/**
 * Device Table Base Address Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t   u9Size : 9;     /**< Bits 8:0   - Size: Size of the device table. */
        RT_GCC_EXTENSION uint64_t   u3Rsvd0 : 3;    /**< Bits 11:9  - Reserved. */
        RT_GCC_EXTENSION uint64_t   u40Base : 40;   /**< Bits 51:12 - DevTabBase: Device table base address. */
        RT_GCC_EXTENSION uint64_t   u12Rsvd0 : 12;  /**< Bits 63:52 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} DEV_TAB_BAR_T;
AssertCompileSize(DEV_TAB_BAR_T, 8);
#define IOMMU_DEV_TAB_BAR_VALID_MASK          UINT64_C(0x000ffffffffff1ff)
#define IOMMU_DEV_TAB_SEG_BAR_VALID_MASK      UINT64_C(0x000ffffffffff0ff)

/**
 * Command Buffer Base Address Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t   u12Rsvd0 : 12;      /**< Bits 11:0  - Reserved. */
        RT_GCC_EXTENSION uint64_t   u40Base : 40;       /**< Bits 51:12 - ComBase: Command buffer base address. */
        RT_GCC_EXTENSION uint64_t   u4Rsvd0 : 4;        /**< Bits 55:52 - Reserved. */
        RT_GCC_EXTENSION uint64_t   u4Len : 4;          /**< Bits 59:56 - ComLen: Command buffer length. */
        RT_GCC_EXTENSION uint64_t   u4Rsvd1 : 4;        /**< Bits 63:60 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} CMD_BUF_BAR_T;
AssertCompileSize(CMD_BUF_BAR_T, 8);
#define IOMMU_CMD_BUF_BAR_VALID_MASK      UINT64_C(0x0f0ffffffffff000)

/**
 * Event Log Base Address Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t   u12Rsvd0 : 12;      /**< Bits 11:0  - Reserved. */
        RT_GCC_EXTENSION uint64_t   u40Base : 40;       /**< Bits 51:12 - EventBase: Event log base address. */
        RT_GCC_EXTENSION uint64_t   u4Rsvd0 : 4;        /**< Bits 55:52 - Reserved. */
        RT_GCC_EXTENSION uint64_t   u4Len : 4;          /**< Bits 59:56 - EventLen: Event log length. */
        RT_GCC_EXTENSION uint64_t   u4Rsvd1 : 4;        /**< Bits 63:60 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} EVT_LOG_BAR_T;
AssertCompileSize(EVT_LOG_BAR_T, 8);
#define IOMMU_EVT_LOG_BAR_VALID_MASK      UINT64_C(0x0f0ffffffffff000)

/**
 * IOMMU Control Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u1IommuEn : 1;               /**< Bit  0     - IommuEn: IOMMU Enable. */
        uint32_t    u1HtTunEn : 1;               /**< Bit  1     - HtTunEn: HyperTransport Tunnel Enable. */
        uint32_t    u1EvtLogEn : 1;              /**< Bit  2     - EventLogEn: Event Log Enable. */
        uint32_t    u1EvtIntrEn : 1;             /**< Bit  3     - EventIntEn: Event Log Interrupt Enable. */
        uint32_t    u1CompWaitIntrEn : 1;        /**< Bit  4     - ComWaitIntEn: Completion Wait Interrupt Enable. */
        uint32_t    u3InvTimeOut : 3;            /**< Bits 7:5   - InvTimeOut: Invalidation Timeout. */
        uint32_t    u1PassPW : 1;                /**< Bit  8     - PassPW: Pass Posted Write. */
        uint32_t    u1ResPassPW : 1;             /**< Bit  9     - ResPassPW: Response Pass Posted Write. */
        uint32_t    u1Coherent : 1;              /**< Bit  10    - Coherent: HT read request packet Coherent bit. */
        uint32_t    u1Isoc : 1;                  /**< Bit  11    - Isoc: HT read request packet Isochronous bit. */
        uint32_t    u1CmdBufEn : 1;              /**< Bit  12    - CmdBufEn: Command Buffer Enable. */
        uint32_t    u1PprLogEn : 1;              /**< Bit  13    - PprLogEn: Peripheral Page Request (PPR) Log Enable. */
        uint32_t    u1PprIntrEn : 1;             /**< Bit  14    - PprIntrEn: Peripheral Page Request Interrupt Enable. */
        uint32_t    u1PprEn : 1;                 /**< Bit  15    - PprEn: Peripheral Page Request processing Enable. */
        uint32_t    u1GstTranslateEn : 1;        /**< Bit  16    - GTEn: Guest Translate Enable. */
        uint32_t    u1GstVirtApicEn : 1;         /**< Bit  17    - GAEn: Guest Virtual-APIC Enable. */
        uint32_t    u4Crw : 1;                   /**< Bits 21:18 - CRW: Intended for future use (not documented). */
        uint32_t    u1SmiFilterEn : 1;           /**< Bit  22    - SmiFEn: SMI Filter Enable. */
        uint32_t    u1SelfWriteBackDis : 1;      /**< Bit  23    - SlfWBDis: Self Write-Back Disable. */
        uint32_t    u1SmiFilterLogEn : 1;        /**< Bit  24    - SmiFLogEn: SMI Filter Log Enable. */
        uint32_t    u3GstVirtApicModeEn : 3;     /**< Bits 27:25 - GAMEn: Guest Virtual-APIC Mode Enable. */
        uint32_t    u1GstLogEn : 1;              /**< Bit  28    - GALogEn: Guest Virtual-APIC GA Log Enable. */
        uint32_t    u1GstIntrEn : 1;             /**< Bit  29    - GAIntEn: Guest Virtual-APIC Interrupt Enable. */
        uint32_t    u2DualPprLogEn : 2;          /**< Bits 31:30 - DualPprLogEn: Dual Peripheral Page Request Log Enable. */
        uint32_t    u2DualEvtLogEn : 2;          /**< Bits 33:32 - DualEventLogEn: Dual Event Log Enable. */
        uint32_t    u3DevTabSegEn : 3;           /**< Bits 36:34 - DevTblSegEn: Device Table Segment Enable. */
        uint32_t    u2PrivAbortEn : 2;           /**< Bits 38:37 - PrivAbrtEn: Privilege Abort Enable. */
        uint32_t    u1PprAutoRespEn : 1;         /**< Bit  39    - PprAutoRspEn: Peripheral Page Request Auto Response Enable. */
        uint32_t    u1MarcEn : 1;                /**< Bit  40    - MarcEn: Memory Address Routing and Control Enable. */
        uint32_t    u1BlockStopMarkEn : 1;       /**< Bit  41    - BlkStopMarkEn: Block StopMark messages Enable. */
        uint32_t    u1PprAutoRespAlwaysOnEn : 1; /**< Bit  42    - PprAutoRspAon:: PPR Auto Response - Always On Enable. */
        uint32_t    u1DomainIDPNE : 1;           /**< Bit  43    - DomainIDPE: Reserved (not documented). */
        uint32_t    u1Rsvd0 : 1;                 /**< Bit  44    - Reserved. */
        uint32_t    u1EnhancedPpr : 1;           /**< Bit  45    - EPHEn: Enhanced Peripheral Page Request Handling Enable. */
        uint32_t    u2HstAccDirtyBitUpdate : 2;  /**< Bits 47:46 - HADUpdate: Access and Dirty Bit updated in host page table. */
        uint32_t    u1GstDirtyUpdateDis : 1;     /**< Bit  48    - GDUpdateDis: Disable hardare update of Dirty bit in GPT. */
        uint32_t    u1Rsvd1 : 1;                 /**< Bit  49    - Reserved. */
        uint32_t    u1X2ApicEn : 1;              /**< Bit  50    - XTEn: Enable X2APIC. */
        uint32_t    u1X2ApicIntrGenEn : 1;       /**< Bit  51    - IntCapXTEn: Enable IOMMU X2APIC Interrupt generation. */
        uint32_t    u2Rsvd0 : 2;                 /**< Bits 53:52 - Reserved. */
        uint32_t    u1GstAccessUpdateDis : 1;    /**< Bit  54    - GAUpdateDis: Disable hardare update of Access bit in GPT. */
        uint32_t    u8Rsvd0 : 8;                 /**< Bits 63:55 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} IOMMU_CTRL_T;
AssertCompileSize(IOMMU_CTRL_T, 8);
#define IOMMU_CTRL_VALID_MASK           UINT64_C(0x004defffffffffff)
#define IOMMU_CTRL_CMD_BUF_EN_MASK      UINT64_C(0x0000000000001001)

/**
 * IOMMU Exclusion Base Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t   u1ExclEnable : 1;       /**< Bit 0      - ExEn: Exclusion Range Enable. */
        RT_GCC_EXTENSION uint64_t   u1AllowAll : 1;         /**< Bit 1      - Allow: Allow All Devices. */
        RT_GCC_EXTENSION uint64_t   u10Rsvd0 : 10;          /**< Bits 11:2  - Reserved. */
        RT_GCC_EXTENSION uint64_t   u40ExclRangeBase : 40;  /**< Bits 51:12 - Exclusion Range Base Address. */
        RT_GCC_EXTENSION uint64_t   u12Rsvd0 : 12;          /**< Bits 63:52 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} IOMMU_EXCL_RANGE_BAR_T;
AssertCompileSize(IOMMU_EXCL_RANGE_BAR_T, 8);
#define IOMMU_EXCL_RANGE_BAR_VALID_MASK     UINT64_C(0x000ffffffffff003)

/**
 * IOMMU Exclusion Range Limit Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t   u12Rsvd0 : 12;           /**< Bits 63:52 - Reserved. */
        RT_GCC_EXTENSION uint64_t   u40ExclRangeLimit : 40;  /**< Bits 51:12 - Exclusion Range Limit Address. */
        RT_GCC_EXTENSION uint64_t   u12Rsvd1 : 12;           /**< Bits 63:52 - Reserved (treated as 1s). */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} IOMMU_EXCL_RANGE_LIMIT_T;
AssertCompileSize(IOMMU_EXCL_RANGE_LIMIT_T, 8);
#define IOMMU_EXCL_RANGE_LIMIT_VALID_MASK   UINT64_C(0x000fffffffffffff)

/**
 * IOMMU Extended Feature Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u1PrefetchSup : 1;            /**< Bit  0     - PreFSup: Prefetch Support. */
        uint32_t    u1PprSup : 1;                 /**< Bit  1     - PPRSup: Peripheral Page Request Support. */
        uint32_t    u1X2ApicSup : 1;              /**< Bit  2     - XTSup: x2Apic Support. */
        uint32_t    u1NoExecuteSup : 1;           /**< Bit  3     - NXSup: No-Execute and Privilege Level Support. */
        uint32_t    u1GstTranslateSup : 1;        /**< Bit  4     - GTSup: Guest Translations (for GVAs) Support. */
        uint32_t    u1Rsvd0 : 1;                  /**< Bit  5     - Reserved. */
        uint32_t    u1InvAllSup : 1;              /**< Bit  6     - IASup: Invalidate-All Support. */
        uint32_t    u1GstVirtApicSup : 1;         /**< Bit  7     - GASup: Guest Virtual-APIC Support. */
        uint32_t    u1HwErrorSup : 1;             /**< Bit  8     - HESup: Hardware Error registers Support. */
        uint32_t    u1PerfCounterSup : 1;         /**< Bit  9     - PCSup: Performance Counter Support. */
        uint32_t    u2HostAddrTranslateSize : 2;  /**< Bits 11:10 - HATS: Host Address Translation Size. */
        uint32_t    u2GstAddrTranslateSize : 2;   /**< Bits 13:12 - GATS: Guest Address Translation Size. */
        uint32_t    u2GstCr3RootTblLevel : 2;     /**< Bits 15:14 - GLXSup: Guest CR3 Root Table Level (Max) Size Support. */
        uint32_t    u2SmiFilterSup : 2;           /**< Bits 17:16 - SmiFSup: SMI Filter Register Support. */
        uint32_t    u3SmiFilterCount : 3;         /**< Bits 20:18 - SmiFRC: SMI Filter Register Count. */
        uint32_t    u3GstVirtApicModeSup : 3;     /**< Bits 23:21 - GAMSup: Guest Virtual-APIC Modes Supported. */
        uint32_t    u2DualPprLogSup : 2;          /**< Bits 25:24 - DualPprLogSup: Dual Peripheral Page Request Log Support. */
        uint32_t    u2Rsvd0 : 2;                  /**< Bits 27:26 - Reserved. */
        uint32_t    u2DualEvtLogSup : 2;          /**< Bits 29:28 - DualEventLogSup: Dual Event Log Support. */
        uint32_t    u2Rsvd1 : 2;                  /**< Bits 31:30 - Reserved. */
        uint32_t    u5MaxPasidSup : 5;            /**< Bits 36:32 - PASMax: Maximum PASID Supported. */
        uint32_t    u1UserSupervisorSup : 1;      /**< Bit  37    - USSup: User/Supervisor Page Protection Support. */
        uint32_t    u2DevTabSegSup : 2;           /**< Bits 39:38 - DevTlbSegSup: Segmented Device Table Support. */
        uint32_t    u1PprLogOverflowWarn : 1;     /**< Bit  40    - PprOvrflwEarlySup: PPR Log Overflow Early Warning Support. */
        uint32_t    u1PprAutoRespSup : 1;         /**< Bit  41    - PprAutoRspSup: PPR Automatic Response Support. */
        uint32_t    u2MarcSup : 2;                /**< Bit  43:42 - MarcSup: Memory Access Routing and Control Support. */
        uint32_t    u1BlockStopMarkSup : 1;       /**< Bit  44    - BlkStopMarkSup: Block StopMark messages Support. */
        uint32_t    u1PerfOptSup : 1;             /**< Bit  45    - PerfOptSup: IOMMU Performance Optimization Support. */
        uint32_t    u1MsiCapMmioSup : 1;          /**< Bit  46    - MsiCapMmioSup: MSI Capability Register MMIO Access Support. */
        uint32_t    u1Rsvd1 : 1;                  /**< Bit  47    - Reserved. */
        uint32_t    u1GstIoSup : 1;               /**< Bit  48    - GIoSup: Guest I/O Protection Support. */
        uint32_t    u1HostAccessSup : 1;          /**< Bit  49    - HASup: Host Access Support. */
        uint32_t    u1EnhancedPprSup : 1;         /**< Bit  50    - EPHSup: Enhanced Peripheral Page Request Handling Support. */
        uint32_t    u1AttrForwardSup : 1;         /**< Bit  51    - AttrFWSup: Attribute Forward Support. */
        uint32_t    u1HostDirtySup : 1;           /**< Bit  52    - HDSup: Host Dirty Support. */
        uint32_t    u1Rsvd2 : 1;                  /**< Bit  53    - Reserved. */
        uint32_t    u1InvIoTlbTypeSup : 1;        /**< Bit  54    - InvIotlbTypeSup: Invalidate IOTLB Type Support. */
        uint32_t    u6Rsvd0 : 6;                  /**< Bit  60:55 - Reserved. */
        uint32_t    u1GstUpdateDisSup : 1;        /**< Bit  61    - GAUpdateDisSup: Disable hardware update on GPT Support. */
        uint32_t    u1ForcePhysDstSup : 1;        /**< Bit  62    - ForcePhyDestSup: Force Phys. Dst. Mode for Remapped Intr. */
        uint32_t    u1Rsvd3 : 1;                  /**< Bit  63    - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} IOMMU_EXT_FEAT_T;
AssertCompileSize(IOMMU_EXT_FEAT_T, 8);

/**
 * Peripheral Page Request Log Base Address Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t   u12Rsvd0 : 12;      /**< Bit 11:0   - Reserved. */
        RT_GCC_EXTENSION uint64_t   u40Base : 40;       /**< Bits 51:12 - PPRLogBase: Peripheral Page Request Log Base Address. */
        RT_GCC_EXTENSION uint64_t   u4Rsvd0 : 4;        /**< Bits 55:52 - Reserved. */
        RT_GCC_EXTENSION uint64_t   u4Len : 4;          /**< Bits 59:56 - PPRLogLen: Peripheral Page Request Log Length. */
        RT_GCC_EXTENSION uint64_t   u4Rsvd1 : 4;        /**< Bits 63:60 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} PPR_LOG_BAR_T;
AssertCompileSize(PPR_LOG_BAR_T, 8);
#define IOMMU_PPR_LOG_BAR_VALID_MASK    UINT64_C(0x0f0ffffffffff000)

/**
 * IOMMU Hardware Event Upper Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t   u60FirstOperand : 60;   /**< Bits 59:0  - First event code dependent operand. */
        RT_GCC_EXTENSION uint64_t   u4EvtCode : 4;          /**< Bits 63:60 - Event Code. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} IOMMU_HW_EVT_HI_T;
AssertCompileSize(IOMMU_HW_EVT_HI_T, 8);

/**
 * IOMMU Hardware Event Lower Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef uint64_t IOMMU_HW_EVT_LO_T;

/**
 * IOMMU Hardware Event Status (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t   u1Valid : 1;     /**< Bit 0      - HEV: Hardware Event Valid. */
        uint32_t   u1Overflow : 1;  /**< Bit 1      - HEO: Hardware Event Overflow. */
        uint32_t   u30Rsvd0 : 30;   /**< Bits 31:2  - Reserved. */
        uint32_t   u32Rsvd0;        /**< Bits 63:32 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} IOMMU_HW_EVT_STATUS_T;
AssertCompileSize(IOMMU_HW_EVT_STATUS_T, 8);
#define IOMMU_HW_EVT_STATUS_VALID_MASK      UINT64_C(0x0000000000000003)

/**
 * Guest Virtual-APIC Log Base Address Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t   u12Rsvd0 : 12;  /**< Bit 11:0   - Reserved. */
        RT_GCC_EXTENSION uint64_t   u40Base : 40;   /**< Bits 51:12 - GALogBase: Guest Virtual-APIC Log Base Address. */
        RT_GCC_EXTENSION uint64_t   u4Rsvd0 : 4;    /**< Bits 55:52 - Reserved. */
        RT_GCC_EXTENSION uint64_t   u4Len : 4;      /**< Bits 59:56 - GALogLen: Guest Virtual-APIC Log Length. */
        RT_GCC_EXTENSION uint64_t   u4Rsvd1 : 4;    /**< Bits 63:60 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} GALOG_BAR_T;
AssertCompileSize(GALOG_BAR_T, 8);

/**
 * Guest Virtual-APIC Log Tail Address Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t   u3Rsvd0 : 3;            /**< Bits 2:0   - Reserved. */
        RT_GCC_EXTENSION uint64_t   u40GALogTailAddr : 48;  /**< Bits 51:3  - GATAddr: Guest Virtual-APIC Tail Log Address. */
        RT_GCC_EXTENSION uint64_t   u11Rsvd1 : 11;          /**< Bits 63:52 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} GALOG_TAIL_ADDR_T;
AssertCompileSize(GALOG_TAIL_ADDR_T, 8);

/**
 * PPR Log B Base Address Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to PPR_LOG_BAR_T.
 */
typedef PPR_LOG_BAR_T       PPR_LOG_B_BAR_T;

/**
 * Event Log B Base Address Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to EVT_LOG_BAR_T.
 */
typedef EVT_LOG_BAR_T       EVT_LOG_B_BAR_T;

/**
 * Device-specific Feature Extension (DSFX) Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u24DevSpecFeat : 24;     /**< Bits 23:0  - DevSpecificFeatSupp: Implementation specific features. */
        uint32_t    u4RevMinor : 4;          /**< Bits 27:24 - RevMinor: Minor revision identifier. */
        uint32_t    u4RevMajor : 4;          /**< Bits 31:28 - RevMajor: Major revision identifier. */
        uint32_t    u32Rsvd0;                /**< Bits 63:32 - Reserved.*/
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} DEV_SPECIFIC_FEAT_T;
AssertCompileSize(DEV_SPECIFIC_FEAT_T, 8);

/**
 * Device-specific Control Extension (DSCX) Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u24DevSpecCtrl : 24;     /**< Bits 23:0  - DevSpecificFeatCntrl: Implementation specific control. */
        uint32_t    u4RevMinor : 4;          /**< Bits 27:24 - RevMinor: Minor revision identifier. */
        uint32_t    u4RevMajor : 4;          /**< Bits 31:28 - RevMajor: Major revision identifier. */
        uint32_t    u32Rsvd0;                /**< Bits 63:32 - Reserved.*/
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} DEV_SPECIFIC_CTRL_T;
AssertCompileSize(DEV_SPECIFIC_CTRL_T, 8);

/**
 * Device-specific Status Extension (DSSX) Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u24DevSpecStatus : 24;      /**< Bits 23:0  - DevSpecificFeatStatus: Implementation specific status. */
        uint32_t    u4RevMinor : 4;             /**< Bits 27:24 - RevMinor: Minor revision identifier. */
        uint32_t    u4RevMajor : 4;             /**< Bits 31:28 - RevMajor: Major revision identifier. */
        uint32_t    u32Rsvd0;                   /**< Bits 63:32 - Reserved.*/
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} DEV_SPECIFIC_STATUS_T;
AssertCompileSize(DEV_SPECIFIC_STATUS_T, 8);

/**
 * MSI Information Register 0 and 1 (PCI) / MSI Vector Register 0 and 1 (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u5MsiNumEvtLog : 5;     /**< Bits 4:0   - MsiNum: Event Log MSI message number. */
        uint32_t    u3GstVirtAddrSize: 3;   /**< Bits 7:5   - GVAsize: Guest Virtual Address Size. */
        uint32_t    u7PhysAddrSize : 7;     /**< Bits 14:8  - PAsize: Physical Address Size. */
        uint32_t    u7VirtAddrSize : 7;     /**< Bits 21:15 - VAsize: Virtual Address Size. */
        uint32_t    u1HtAtsResv: 1;         /**< Bit  22    - HtAtsResv: HyperTransport ATS Response Address range Reserved. */
        uint32_t    u4Rsvd0 : 4;            /**< Bits 26:23 - Reserved. */
        uint32_t    u5MsiNumPpr : 5;        /**< Bits 31:27 - MsiNumPPR: Peripheral Page Request MSI message number. */
        uint32_t    u5MsiNumGa : 5;         /**< Bits 36:32 - MsiNumGa: MSI message number for guest virtual-APIC log. */
        uint32_t    u27Rsvd0: 27;           /**< Bits 63:37 - Reserved. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    au32[2];
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} MSI_MISC_INFO_T;
AssertCompileSize(MSI_MISC_INFO_T, 8);
/** MSI Vector Register 0 and 1 (MMIO). */
typedef MSI_MISC_INFO_T       MSI_VECTOR_T;
/** Mask of valid bits in MSI Vector Register 1 (or high dword of MSI Misc.
 *  info). */
#define IOMMU_MSI_VECTOR_1_VALID_MASK       UINT32_C(0x1f)

/**
 * MSI Capability Header Register (PCI + MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u8MsiCapId : 8;         /**< Bits 7:0   - MsiCapId: Capability ID. */
        uint32_t    u8MsiCapPtr : 8;        /**< Bits 15:8  - MsiCapPtr: Pointer (PCI config offset) to the next capability. */
        uint32_t    u1MsiEnable : 1;        /**< Bit  16    - MsiEn: Message Signal Interrupt Enable. */
        uint32_t    u3MsiMultiMessCap : 3;  /**< Bits 19:17 - MsiMultMessCap: MSI Multi-Message Capability. */
        uint32_t    u3MsiMultiMessEn : 3;   /**< Bits 22:20 - MsiMultMessEn: MSI Multi-Message Enable. */
        uint32_t    u1Msi64BitEn : 1;       /**< Bit  23    - Msi64BitEn: MSI 64-bit Enable. */
        uint32_t    u8Rsvd0 : 8;            /**< Bits 31:24 - Reserved. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    u32;
} MSI_CAP_HDR_T;
AssertCompileSize(MSI_CAP_HDR_T, 4);
#define IOMMU_MSI_CAP_HDR_MSI_EN_MASK       RT_BIT(16)

/**
 * MSI Mapping Capability Header Register (PCI + MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u8MsiMapCapId : 8;  /**< Bits 7:0   - MsiMapCapId: MSI Map capability ID. */
        uint32_t    u8Rsvd0 : 8;        /**< Bits 15:8  - Reserved. */
        uint32_t    u1MsiMapEn : 1;     /**< Bit  16    - MsiMapEn: MSI Map enable. */
        uint32_t    u1MsiMapFixed : 1;  /**< Bit  17    - MsiMapFixd: MSI Map fixed. */
        uint32_t    u9Rsvd0 : 9;        /**< Bits 26:18 - Reserved. */
        uint32_t    u5MapCapType : 5;   /**< Bits 31:27 - MsiMapCapType: MSI Mapping capability type. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    u32;
} MSI_MAP_CAP_HDR_T;
AssertCompileSize(MSI_MAP_CAP_HDR_T, 4);

/**
 * Performance Optimization Control Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u13Rsvd0 : 13;      /**< Bits 12:0  - Reserved. */
        uint32_t    u1PerfOptEn : 1;    /**< Bit  13    - PerfOptEn: Performance Optimization Enable. */
        uint32_t    u17Rsvd0 : 18;      /**< Bits 31:14 - Reserved. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    u32;
} IOMMU_PERF_OPT_CTRL_T;
AssertCompileSize(IOMMU_PERF_OPT_CTRL_T, 4);

/**
 * XT (x2APIC) IOMMU General Interrupt Control Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u2Rsvd0 : 2;                    /**< Bits 1:0   - Reserved.*/
        uint32_t    u1X2ApicIntrDstMode : 1;        /**< Bit  2     - Destination Mode for general interrupt.*/
        uint32_t    u4Rsvd0 : 4;                    /**< Bits 7:3   - Reserved.*/
        uint32_t    u24X2ApicIntrDstLo : 24;        /**< Bits 31:8  - Destination for general interrupt (Lo).*/
        uint32_t    u8X2ApicIntrVector : 8;         /**< Bits 39:32 - Vector for general interrupt.*/
        uint32_t    u1X2ApicIntrDeliveryMode : 1;   /**< Bit  40    - Delivery Mode for general interrupt.*/
        uint32_t    u15Rsvd0 : 15;                  /**< Bits 55:41 - Reserved.*/
        uint32_t    u7X2ApicIntrDstHi : 7;          /**< Bits 63:56 - Destination for general interrupt (Hi) .*/
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} IOMMU_XT_GEN_INTR_CTRL_T;
AssertCompileSize(IOMMU_XT_GEN_INTR_CTRL_T, 8);

/**
 * XT (x2APIC) IOMMU General Interrupt Control Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u2Rsvd0 : 2;                    /**< Bits 1:0   - Reserved.*/
        uint32_t    u1X2ApicIntrDstMode : 1;        /**< Bit  2     - Destination Mode for the interrupt.*/
        uint32_t    u4Rsvd0 : 4;                    /**< Bits 7:3   - Reserved.*/
        uint32_t    u24X2ApicIntrDstLo : 24;        /**< Bits 31:8  - Destination for the interrupt (Lo).*/
        uint32_t    u8X2ApicIntrVector : 8;         /**< Bits 39:32 - Vector for the interrupt.*/
        uint32_t    u1X2ApicIntrDeliveryMode : 1;   /**< Bit  40    - Delivery Mode for the interrupt.*/
        uint32_t    u15Rsvd0 : 15;                  /**< Bits 55:41 - Reserved.*/
        uint32_t    u7X2ApicIntrDstHi : 7;          /**< Bits 63:56 - Destination for the interrupt (Hi) .*/
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} IOMMU_XT_INTR_CTRL_T;
AssertCompileSize(IOMMU_XT_INTR_CTRL_T, 8);

/**
 * XT (x2APIC) IOMMU PPR Interrupt Control Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to IOMMU_XT_INTR_CTRL_T.
 */
typedef IOMMU_XT_INTR_CTRL_T    IOMMU_XT_PPR_INTR_CTRL_T;

/**
 * XT (x2APIC) IOMMU GA (Guest Address) Log Control Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to IOMMU_XT_INTR_CTRL_T.
 */
typedef IOMMU_XT_INTR_CTRL_T    IOMMU_XT_GALOG_INTR_CTRL_T;

/**
 * Memory Access and Routing Control (MARC) Aperture Base Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t   u12Rsvd0 : 12;          /**< Bits 11:0  - Reserved. */
        RT_GCC_EXTENSION uint64_t   u40MarcBaseAddr : 40;   /**< Bits 51:12 - MarcBaseAddr: MARC Aperture Base Address. */
        RT_GCC_EXTENSION uint64_t   u12Rsvd1 : 12;          /**< Bits 63:52 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} MARC_APER_BAR_T;
AssertCompileSize(MARC_APER_BAR_T, 8);

/**
 * Memory Access and Routing Control (MARC) Relocation Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t   u1RelocEn : 1;          /**< Bit  0     - RelocEn: Relocation Enabled. */
        RT_GCC_EXTENSION uint64_t   u1ReadOnly : 1;         /**< Bit  1     - ReadOnly: Whether only read-only acceses allowed. */
        RT_GCC_EXTENSION uint64_t   u10Rsvd0 : 10;          /**< Bits 11:2  - Reserved. */
        RT_GCC_EXTENSION uint64_t   u40MarcRelocAddr : 40;  /**< Bits 51:12 - MarcRelocAddr: MARC Aperture Relocation Address. */
        RT_GCC_EXTENSION uint64_t   u12Rsvd1 : 12;          /**< Bits 63:52 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} MARC_APER_RELOC_T;
AssertCompileSize(MARC_APER_RELOC_T, 8);

/**
 * Memory Access and Routing Control (MARC) Length Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        RT_GCC_EXTENSION uint64_t   u12Rsvd0 : 12;          /**< Bits 11:0  - Reserved. */
        RT_GCC_EXTENSION uint64_t   u40MarcLength : 40;     /**< Bits 51:12 - MarcLength: MARC Aperture Length. */
        RT_GCC_EXTENSION uint64_t   u12Rsvd1 : 12;          /**< Bits 63:52 - Reserved. */
    } n;
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} MARC_APER_LEN_T;

/**
 * Memory Access and Routing Control (MARC) Aperture Register.
 * This combines other registers to match the MMIO layout for convenient access.
 */
typedef struct
{
    MARC_APER_BAR_T     Base;
    MARC_APER_RELOC_T   Reloc;
    MARC_APER_LEN_T     Length;
} MARC_APER_T;
AssertCompileSize(MARC_APER_T, 24);

/**
 * IOMMU Reserved Register (MMIO).
 * In accordance with the AMD spec.
 * This register is reserved for hardware use (although RW?).
 */
typedef uint64_t    IOMMU_RSVD_REG_T;

/**
 * Command Buffer Head Pointer Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    off;            /**< Bits 31:0  - Buffer pointer (offset; 16 byte aligned, 512 KB max). */
        uint32_t    u32Rsvd0;       /**< Bits 63:32 - Reserved. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    au32[2];
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} CMD_BUF_HEAD_PTR_T;
AssertCompileSize(CMD_BUF_HEAD_PTR_T, 8);
#define IOMMU_CMD_BUF_HEAD_PTR_VALID_MASK       UINT64_C(0x000000000007fff0)

/**
 * Command Buffer Tail Pointer Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to CMD_BUF_HEAD_PTR_T.
 */
typedef CMD_BUF_HEAD_PTR_T    CMD_BUF_TAIL_PTR_T;
#define IOMMU_CMD_BUF_TAIL_PTR_VALID_MASK       IOMMU_CMD_BUF_HEAD_PTR_VALID_MASK

/**
 * Event Log Head Pointer Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to CMD_BUF_HEAD_PTR_T.
 */
typedef CMD_BUF_HEAD_PTR_T    EVT_LOG_HEAD_PTR_T;
#define IOMMU_EVT_LOG_HEAD_PTR_VALID_MASK       IOMMU_CMD_BUF_HEAD_PTR_VALID_MASK

/**
 * Event Log Tail Pointer Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to CMD_BUF_HEAD_PTR_T.
 */
typedef CMD_BUF_HEAD_PTR_T    EVT_LOG_TAIL_PTR_T;
#define IOMMU_EVT_LOG_TAIL_PTR_VALID_MASK       IOMMU_CMD_BUF_HEAD_PTR_VALID_MASK


/**
 * IOMMU Status Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u1EvtOverflow : 1;          /**< Bit  0     - EventOverflow: Event log overflow. */
        uint32_t    u1EvtLogIntr : 1;           /**< Bit  1     - EventLogInt: Event log interrupt. */
        uint32_t    u1CompWaitIntr : 1;         /**< Bit  2     - ComWaitInt: Completion wait interrupt . */
        uint32_t    u1EvtLogRunning : 1;        /**< Bit  3     - EventLogRun: Event logging is running. */
        uint32_t    u1CmdBufRunning : 1;        /**< Bit  4     - CmdBufRun: Command buffer is running. */
        uint32_t    u1PprOverflow : 1;          /**< Bit  5     - PprOverflow: Peripheral Page Request Log (PPR) overflow. */
        uint32_t    u1PprIntr : 1;              /**< Bit  6     - PprInt: PPR interrupt. */
        uint32_t    u1PprLogRunning : 1;        /**< Bit  7     - PprLogRun: PPR logging is running. */
        uint32_t    u1GstLogRunning : 1;        /**< Bit  8     - GALogRun: Guest virtual-APIC logging is running. */
        uint32_t    u1GstLogOverflow : 1;       /**< Bit  9     - GALOverflow: Guest virtual-APIC log overflow. */
        uint32_t    u1GstLogIntr : 1;           /**< Bit  10    - GAInt: Guest virtual-APIC log interrupt. */
        uint32_t    u1PprOverflowB : 1;         /**< Bit  11    - PprOverflowB: PPR log B overflow. */
        uint32_t    u1PprLogActive : 1;         /**< Bit  12    - PprLogActive: PPR log A is active. */
        uint32_t    u2Rsvd0 : 2;                /**< Bits 14:13 - Reserved. */
        uint32_t    u1EvtOverflowB : 1;         /**< Bit  15    - EvtOverflowB: Event log B overflow. */
        uint32_t    u1EvtLogActive : 1;         /**< Bit  16    - EvtLogActive: Event log A active. */
        uint32_t    u1PprOverflowEarlyB : 1;    /**< Bit  17    - PprOverflowEarlyB: PPR log B overflow early warning. */
        uint32_t    u1PprOverflowEarly : 1;     /**< Bit  18    - PprOverflowEarly: PPR log overflow early warning. */
        uint32_t    u13Rsvd0 : 13;              /**< Bits 31:19 - Reserved. */
        uint32_t    u32Rsvd0;                   /**< Bits 63:32 - Reserved . */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    au32[2];
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} IOMMU_STATUS_T;
AssertCompileSize(IOMMU_STATUS_T, 8);
#define IOMMU_STATUS_VALID_MASK     UINT64_C(0x0000000000079fff)
#define IOMMU_STATUS_RW1C_MASK      UINT64_C(0x0000000000068e67)

/**
 * PPR Log Head Pointer Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to CMD_BUF_HEAD_PTR_T.
 */
typedef CMD_BUF_HEAD_PTR_T      PPR_LOG_HEAD_PTR_T;

/**
 * PPR Log Tail Pointer Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to CMD_BUF_HEAD_PTR_T.
 */
typedef CMD_BUF_HEAD_PTR_T      PPR_LOG_TAIL_PTR_T;

/**
 * Guest Virtual-APIC Log Head Pointer Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u2Rsvd0 : 2;            /**< Bits 2:0   - Reserved. */
        uint32_t    u12GALogPtr : 12;       /**< Bits 15:3  - Guest Virtual-APIC Log Head or Tail Pointer. */
        uint32_t    u16Rsvd0 : 16;          /**< Bits 31:16 - Reserved. */
        uint32_t    u32Rsvd0;               /**< Bits 63:32 - Reserved. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    au32[2];
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} GALOG_HEAD_PTR_T;
AssertCompileSize(GALOG_HEAD_PTR_T, 8);

/**
 * Guest Virtual-APIC Log Tail Pointer Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to GALOG_HEAD_PTR_T.
 */
typedef GALOG_HEAD_PTR_T        GALOG_TAIL_PTR_T;

/**
 * PPR Log B Head Pointer Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to CMD_BUF_HEAD_PTR_T.
 */
typedef CMD_BUF_HEAD_PTR_T      PPR_LOG_B_HEAD_PTR_T;

/**
 * PPR Log B Tail Pointer Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to CMD_BUF_HEAD_PTR_T.
 */
typedef CMD_BUF_HEAD_PTR_T      PPR_LOG_B_TAIL_PTR_T;

/**
 * Event Log B Head Pointer Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to CMD_BUF_HEAD_PTR_T.
 */
typedef CMD_BUF_HEAD_PTR_T      EVT_LOG_B_HEAD_PTR_T;

/**
 * Event Log B Tail Pointer Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to CMD_BUF_HEAD_PTR_T.
 */
typedef CMD_BUF_HEAD_PTR_T      EVT_LOG_B_TAIL_PTR_T;

/**
 * PPR Log Auto Response Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u4AutoRespCode : 4;     /**< Bits 3:0   - PprAutoRespCode: PPR log Auto Response Code. */
        uint32_t    u1AutoRespMaskGen : 1;  /**< Bit  4     - PprAutoRespMaskGn: PPR log Auto Response Mask Gen. */
        uint32_t    u27Rsvd0 : 27;          /**< Bits 31:5  - Reserved. */
        uint32_t    u32Rsvd0;               /**< Bits 63:32 - Reserved.*/
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    au32[2];
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} PPR_LOG_AUTO_RESP_T;
AssertCompileSize(PPR_LOG_AUTO_RESP_T, 8);

/**
 * PPR Log Overflow Early Indicator Register (MMIO).
 * In accordance with the AMD spec.
 */
typedef union
{
    struct
    {
        uint32_t    u15Threshold : 15;  /**< Bits 14:0  - PprOvrflwEarlyThreshold: Overflow early indicator threshold. */
        uint32_t    u15Rsvd0 : 15;      /**< Bits 29:15 - Reserved. */
        uint32_t    u1IntrEn : 1;       /**< Bit  30    - PprOvrflwEarlyIntEn: Overflow early indicator interrupt enable. */
        uint32_t    u1Enable : 1;       /**< Bit  31    - PprOvrflwEarlyEn: Overflow early indicator enable. */
        uint32_t    u32Rsvd0;           /**< Bits 63:32 - Reserved. */
    } n;
    /** The 32-bit unsigned integer view. */
    uint32_t    au32[2];
    /** The 64-bit unsigned integer view. */
    uint64_t    u64;
} PPR_LOG_OVERFLOW_EARLY_T;
AssertCompileSize(PPR_LOG_OVERFLOW_EARLY_T, 8);

/**
 * PPR Log B Overflow Early Indicator Register (MMIO).
 * In accordance with the AMD spec.
 * Currently identical to PPR_LOG_OVERFLOW_EARLY_T.
 */
typedef PPR_LOG_OVERFLOW_EARLY_T        PPR_LOG_B_OVERFLOW_EARLY_T;

/**
 * ILLEGAL_DEV_TABLE_ENTRY Event Types.
 * In accordance with the AMD spec.
 */
typedef enum EVT_ILLEGAL_DTE_TYPE_T
{
    kIllegalDteType_RsvdNotZero = 0,
    kIllegalDteType_RsvdIntTabLen,
    kIllegalDteType_RsvdIoCtl,
    kIllegalDteType_RsvdIntCtl
} EVT_ILLEGAL_DTE_TYPE_T;

/**
 * ILLEGAL_DEV_TABLE_ENTRY Event Types.
 * In accordance with the AMD spec.
 */
typedef enum EVT_IO_PAGE_FAULT_TYPE_T
{
    /* Memory transaction. */
    kIoPageFaultType_DteRsvdPagingMode = 0,
    kIoPageFaultType_PteInvalidPageSize,
    kIoPageFaultType_PteInvalidLvlEncoding,
    kIoPageFaultType_SkippedLevelIovaNotZero,
    kIoPageFaultType_PteRsvdNotZero,
    kIoPageFaultType_PteValidNotSet,
    kIoPageFaultType_DteTranslationDisabled,
    kIoPageFaultType_PasidInvalidRange,
    kIoPageFaultType_PermDenied,
    kIoPageFaultType_UserSupervisor,
    /* Interrupt remapping */
    kIoPageFaultType_IrteAddrInvalid,
    kIoPageFaultType_IrteRsvdNotZero,
    kIoPageFaultType_IrteRemapEn,
    kIoPageFaultType_IrteRsvdIntType,
    kIoPageFaultType_IntrReqAborted,
    kIoPageFaultType_IntrWithPasid,
    kIoPageFaultType_SmiFilterMismatch,
    /* Memory transaction or interrupt remapping. */
    kIoPageFaultType_DevId_Invalid
} EVT_IO_PAGE_FAULT_TYPE_T;

/**
 * IOTLB_INV_TIMEOUT Event Types.
 * In accordance with the AMD spec.
 */
typedef enum EVT_IOTLB_INV_TIMEOUT_TYPE_T
{
    InvTimeoutType_NoResponse = 0
} EVT_IOTLB_INV_TIMEOUT_TYPE_T;

/**
 * INVALID_DEVICE_REQUEST Event Types.
 * In accordance with the AMD spec.
 */
typedef enum EVT_INVALID_DEV_REQ_TYPE_T
{
    /* Access. */
    kInvalidDevReqType_ReadOrNonPostedWrite = 0,
    kInvalidDevReqType_PretranslatedTransaction,
    kInvalidDevReqType_PortIo,
    kInvalidDevReqType_SysMgt,
    kInvalidDevReqType_IntrRange,
    kInvalidDevReqType_RsvdIntrRange,
    kInvalidDevReqType_SysMgtAddr,
    /* Translation Request. */
    kInvalidDevReqType_TrAccessInvalid,
    kInvalidDevReqType_TrDisabled,
    kInvalidDevReqType_DevIdInvalid
} EVT_INVALID_DEV_REQ_TYPE_T;

/**
 * INVALID_PPR_REQUEST Event Types.
 * In accordance with the AMD spec.
 */
typedef enum EVT_INVALID_PPR_REQ_TYPE_T
{
    kInvalidPprReqType_PriNotSupported,
    kInvalidPprReqType_GstTranslateDisabled
} EVT_INVALID_PPR_REQ_TYPE_T;


/** @name IVRS format revision field.
 * In accordance with the AMD spec.
 * @{ */
/** Fixed: Supports only pre-assigned device IDs and type 10h and 11h IVHD
 *  blocks. */
#define ACPI_IVRS_FMT_REV_FIXED                         0x1
/** Mixed: Supports pre-assigned and ACPI HID device naming and all IVHD blocks. */
#define ACPI_IVRS_FMT_REV_MIXED                         0x2
/** @} */

/** @name IVHD special device entry variety field.
 * In accordance with the AMD spec.
 * @{ */
/** I/O APIC. */
#define ACPI_IVHD_VARIETY_IOAPIC                        0x1
/** HPET. */
#define ACPI_IVHD_VARIETY_HPET                          0x2
/** @} */

/** @name IVHD device entry type codes.
 * In accordance with the AMD spec.
 * @{ */
/** Reserved. */
#define ACPI_IVHD_DEVENTRY_TYPE_RSVD                    0x0
/** All: DTE setting applies to all Device IDs. */
#define ACPI_IVHD_DEVENTRY_TYPE_ALL                     0x1
/** Select: DTE setting applies to the device specified in DevId field. */
#define ACPI_IVHD_DEVENTRY_TYPE_SELECT                  0x2
/** Start of range: DTE setting applies to all devices from start of range specified
 *  by the DevId field. */
#define ACPI_IVHD_DEVENTRY_TYPE_START_RANGE             0x3
/** End of range: DTE setting from previous type 3 entry applies to all devices
 *  incl. DevId specified by this entry. */
#define ACPI_IVHD_DEVENTRY_TYPE_END_RANGE               0x4
/** @} */

/** @name IVHD DTE (Device Table Entry) Settings.
 * In accordance with the AMD spec.
 * @{ */
/** INITPass: Identifies a device able to assert INIT interrupts. */
#define ACPI_IVHD_DTE_INIT_PASS_SHIFT                   0
#define ACPI_IVHD_DTE_INIT_PASS_MASK                    UINT8_C(0x01)
/** EIntPass: Identifies a device able to assert ExtInt interrupts. */
#define ACPI_IVHD_DTE_EXTINT_PASS_SHIFT                 1
#define ACPI_IVHD_DTE_EXTINT_PASS_MASK                  UINT8_C(0x02)
/** NMIPass: Identifies a device able to assert NMI interrupts. */
#define ACPI_IVHD_DTE_NMI_PASS_SHIFT                    2
#define ACPI_IVHD_DTE_NMI_PASS_MASK                     UINT8_C(0x04)
/** Bit 3 reserved. */
#define ACPI_IVHD_DTE_RSVD_3_SHIFT                      3
#define ACPI_IVHD_DTE_RSVD_3_MASK                       UINT8_C(0x08)
/** SysMgt: Identifies a device able to assert system management messages. */
#define ACPI_IVHD_DTE_SYS_MGT_SHIFT                     4
#define ACPI_IVHD_DTE_SYS_MGT_MASK                      UINT8_C(0x30)
/** Lint0Pass: Identifies a device able to assert LINT0 interrupts. */
#define ACPI_IVHD_DTE_LINT0_PASS_SHIFT                  6
#define ACPI_IVHD_DTE_LINT0_PASS_MASK                   UINT8_C(0x40)
/** Lint0Pass: Identifies a device able to assert LINT1 interrupts. */
#define ACPI_IVHD_DTE_LINT1_PASS_SHIFT                  7
#define ACPI_IVHD_DTE_LINT1_PASS_MASK                   UINT8_C(0x80)
RT_BF_ASSERT_COMPILE_CHECKS(ACPI_IVHD_DTE_, UINT8_C(0), UINT8_MAX,
                            (INIT_PASS, EXTINT_PASS, NMI_PASS, RSVD_3, SYS_MGT, LINT0_PASS, LINT1_PASS));
/** @} */

/**
 * AMD IOMMU: IVHD (I/O Virtualization Hardware Definition) Device Entry (4-byte).
 * In accordance with the AMD spec.
 */
#pragma pack(1)
typedef struct ACPIIVHDDEVENTRY4
{
    uint8_t         u8DevEntryType;     /**< Device entry type. */
    uint16_t        u16DevId;           /**< Device ID. */
    uint8_t         u8DteSetting;       /**< DTE (Device Table Entry) setting. */
} ACPIIVHDDEVENTRY4;
#pragma pack()
AssertCompileSize(ACPIIVHDDEVENTRY4, 4);

/**
 * AMD IOMMU: IVHD (I/O Virtualization Hardware Definition) Device Entry (8-byte).
 * In accordance with the AMD spec.
 */
#pragma pack(1)
typedef struct ACPIIVHDDEVENTRY8
{
    uint8_t         u8DevEntryType;     /**< Device entry type. */
    union
    {
        /** Reserved: When u8DevEntryType is 0x40, 0x41, 0x44 or 0x45 (or 0x49-0x7F). */
        struct
        {
            uint8_t     au8Rsvd0[7];        /**< Reserved (MBZ). */
        } rsvd;
        /** Alias Select: When u8DevEntryType is 0x42 or 0x43. */
        struct
        {
            uint16_t    u16DevIdA;          /**< Device ID A. */
            uint8_t     u8DteSetting;       /**< DTE (Device Table Entry) setting. */
            uint8_t     u8Rsvd0;            /**< Reserved (MBZ). */
            uint16_t    u16DevIdB;          /**< Device ID B. */
            uint8_t     u8Rsvd1;            /**< Reserved (MBZ). */
        } alias;
        /** Extended Select: When u8DevEntryType is 0x46 or 0x47. */
        struct
        {
            uint16_t    u16DevId;           /**< Device ID. */
            uint8_t     u8DteSetting;       /**< DTE (Device Table Entry) setting. */
            uint32_t    u32ExtDteSetting;   /**< Extended DTE setting. */
        } ext;
        /** Special Device: When u8DevEntryType is 0x48. */
        struct
        {
            uint16_t    u16Rsvd0;           /**< Reserved (MBZ). */
            uint8_t     u8DteSetting;       /**< DTE (Device Table Entry) setting. */
            uint8_t     u8Handle;           /**< Handle contains I/O APIC ID or HPET number. */
            uint16_t    u16DevIdB;          /**< Device ID B (I/O APIC or HPET). */
            uint8_t     u8Variety;          /**< Whether this is the HPET or I/O APIC. */
        } special;
    } u;
} ACPIIVHDDEVENTRY8;
#pragma pack()
AssertCompileSize(ACPIIVHDDEVENTRY8, 8);

/** @name IVHD Type 10h Flags.
 * In accordance with the AMD spec.
 * @{ */
/** Peripheral page request support. */
#define ACPI_IVHD_10H_F_PPR_SUP                         RT_BIT(7)
/** Prefetch IOMMU pages command support. */
#define ACPI_IVHD_10H_F_PREF_SUP                        RT_BIT(6)
/** Coherent control. */
#define ACPI_IVHD_10H_F_COHERENT                        RT_BIT(5)
/** Remote IOTLB support. */
#define ACPI_IVHD_10H_F_IOTLB_SUP                       RT_BIT(4)
/** Isochronous control. */
#define ACPI_IVHD_10H_F_ISOC                            RT_BIT(3)
/** Response Pass Posted Write. */
#define ACPI_IVHD_10H_F_RES_PASS_PW                     RT_BIT(2)
/** Pass Posted Write. */
#define ACPI_IVHD_10H_F_PASS_PW                         RT_BIT(1)
/** HyperTransport Tunnel. */
#define ACPI_IVHD_10H_F_HT_TUNNEL                       RT_BIT(0)
/** @} */

/** @name IVRS IVinfo field.
 * In accordance with the AMD spec.
 * @{ */
/** EFRSup: Extended Feature Support. */
#define ACPI_IVINFO_BF_EFR_SUP_SHIFT                    0
#define ACPI_IVINFO_BF_EFR_SUP_MASK                     UINT32_C(0x00000001)
/** DMA Remap Sup: DMA remapping support (pre-boot DMA protection with
 *  mandatory remapping of device accessed memory). */
#define ACPI_IVINFO_BF_DMA_REMAP_SUP_SHIFT              1
#define ACPI_IVINFO_BF_DMA_REMAP_SUP_MASK               UINT32_C(0x00000002)
/** Bits 4:2 reserved. */
#define ACPI_IVINFO_BF_RSVD_2_4_SHIFT                   2
#define ACPI_IVINFO_BF_RSVD_2_4_MASK                    UINT32_C(0x0000001c)
/** GVASize: Guest virtual-address size. */
#define ACPI_IVINFO_BF_GVA_SIZE_SHIFT                   5
#define ACPI_IVINFO_BF_GVA_SIZE_MASK                    UINT32_C(0x000000e0)
/** PASize: System physical address size. */
#define ACPI_IVINFO_BF_PA_SIZE_SHIFT                    8
#define ACPI_IVINFO_BF_PA_SIZE_MASK                     UINT32_C(0x00007f00)
/** VASize: Virtual address size. */
#define ACPI_IVINFO_BF_VA_SIZE_SHIFT                    15
#define ACPI_IVINFO_BF_VA_SIZE_MASK                     UINT32_C(0x003f8000)
/** HTAtsResv: HyperTransport ATS-response address translation range reserved. */
#define ACPI_IVINFO_BF_HT_ATS_RESV_SHIFT                22
#define ACPI_IVINFO_BF_HT_ATS_RESV_MASK                 UINT32_C(0x00400000)
/** Bits 31:23 reserved. */
#define ACPI_IVINFO_BF_RSVD_23_31_SHIFT                 23
#define ACPI_IVINFO_BF_RSVD_23_31_MASK                  UINT32_C(0xff800000)
RT_BF_ASSERT_COMPILE_CHECKS(ACPI_IVINFO_BF_, UINT32_C(0), UINT32_MAX,
                            (EFR_SUP, DMA_REMAP_SUP, RSVD_2_4, GVA_SIZE, PA_SIZE, VA_SIZE, HT_ATS_RESV, RSVD_23_31));
/** @} */

/** @name IVHD IOMMU info flags.
 * In accordance with the AMD spec.
 * @{ */
/** MSI message number for the event log. */
#define ACPI_IOMMU_INFO_BF_MSI_NUM_SHIFT                0
#define ACPI_IOMMU_INFO_BF_MSI_NUM_MASK                 UINT16_C(0x001f)
/** Bits 7:5 reserved. */
#define ACPI_IOMMU_INFO_BF_RSVD_5_7_SHIFT               5
#define ACPI_IOMMU_INFO_BF_RSVD_5_7_MASK                UINT16_C(0x00e0)
/** IOMMU HyperTransport Unit ID number. */
#define ACPI_IOMMU_INFO_BF_UNIT_ID_SHIFT                8
#define ACPI_IOMMU_INFO_BF_UNIT_ID_MASK                 UINT16_C(0x1f00)
/** Bits 15:13 reserved. */
#define ACPI_IOMMU_INFO_BF_RSVD_13_15_SHIFT             13
#define ACPI_IOMMU_INFO_BF_RSVD_13_15_MASK              UINT16_C(0xe000)
RT_BF_ASSERT_COMPILE_CHECKS(ACPI_IOMMU_INFO_BF_, UINT16_C(0), UINT16_MAX,
                            (MSI_NUM, RSVD_5_7, UNIT_ID, RSVD_13_15));
/** @} */

/** @name IVHD IOMMU feature reporting field.
 * In accordance with the AMD spec.
 * @{ */
/** x2APIC supported for peripherals. */
#define ACPI_IOMMU_FEAT_BF_XT_SUP_SHIFT                 0
#define ACPI_IOMMU_FEAT_BF_XT_SUP_MASK                  UINT32_C(0x00000001)
/** NX supported for I/O. */
#define ACPI_IOMMU_FEAT_BF_NX_SUP_SHIFT                 1
#define ACPI_IOMMU_FEAT_BF_NX_SUP_MASK                  UINT32_C(0x00000002)
/** GT (Guest Translation) supported. */
#define ACPI_IOMMU_FEAT_BF_GT_SUP_SHIFT                 2
#define ACPI_IOMMU_FEAT_BF_GT_SUP_MASK                  UINT32_C(0x00000004)
/** GLX (Number of guest CR3 tables) supported. */
#define ACPI_IOMMU_FEAT_BF_GLX_SUP_SHIFT                3
#define ACPI_IOMMU_FEAT_BF_GLX_SUP_MASK                 UINT32_C(0x00000018)
/** IA (INVALIDATE_IOMMU_ALL) command supported. */
#define ACPI_IOMMU_FEAT_BF_IA_SUP_SHIFT                 5
#define ACPI_IOMMU_FEAT_BF_IA_SUP_MASK                  UINT32_C(0x00000020)
/** GA (Guest virtual APIC) supported. */
#define ACPI_IOMMU_FEAT_BF_GA_SUP_SHIFT                 6
#define ACPI_IOMMU_FEAT_BF_GA_SUP_MASK                  UINT32_C(0x00000040)
/** HE (Hardware error) registers supported. */
#define ACPI_IOMMU_FEAT_BF_HE_SUP_SHIFT                 7
#define ACPI_IOMMU_FEAT_BF_HE_SUP_MASK                  UINT32_C(0x00000080)
/** PASMax (maximum PASID) supported. Ignored if PPRSup=0. */
#define ACPI_IOMMU_FEAT_BF_PAS_MAX_SHIFT                8
#define ACPI_IOMMU_FEAT_BF_PAS_MAX_MASK                 UINT32_C(0x00001f00)
/** PNCounters (Number of performance counters per counter bank) supported. */
#define ACPI_IOMMU_FEAT_BF_PN_COUNTERS_SHIFT            13
#define ACPI_IOMMU_FEAT_BF_PN_COUNTERS_MASK             UINT32_C(0x0001e000)
/** PNBanks (Number of performance counter banks) supported. */
#define ACPI_IOMMU_FEAT_BF_PN_BANKS_SHIFT               17
#define ACPI_IOMMU_FEAT_BF_PN_BANKS_MASK                UINT32_C(0x007e0000)
/** MSINumPPR (MSI number for peripheral page requests). */
#define ACPI_IOMMU_FEAT_BF_MSI_NUM_PPR_SHIFT            23
#define ACPI_IOMMU_FEAT_BF_MSI_NUM_PPR_MASK             UINT32_C(0x0f800000)
/** GATS (Guest address translation size). MBZ when GTSup=0. */
#define ACPI_IOMMU_FEAT_BF_GATS_SHIFT                   28
#define ACPI_IOMMU_FEAT_BF_GATS_MASK                    UINT32_C(0x30000000)
/** HATS (Host address translation size). */
#define ACPI_IOMMU_FEAT_BF_HATS_SHIFT                   30
#define ACPI_IOMMU_FEAT_BF_HATS_MASK                    UINT32_C(0xc0000000)
RT_BF_ASSERT_COMPILE_CHECKS(ACPI_IOMMU_FEAT_BF_, UINT32_C(0), UINT32_MAX,
                            (XT_SUP, NX_SUP, GT_SUP, GLX_SUP, IA_SUP, GA_SUP, HE_SUP, PAS_MAX, PN_COUNTERS, PN_BANKS,
                             MSI_NUM_PPR, GATS, HATS));
/** @} */

/** @name IOMMU Extended Feature Register (PCI/MMIO/ACPI).
 * In accordance with the AMD spec.
 * @{ */
/** PreFSup: Prefetch support (RO).   */
#define IOMMU_EXT_FEAT_BF_PREF_SUP_SHIFT                0
#define IOMMU_EXT_FEAT_BF_PREF_SUP_MASK                 UINT64_C(0x0000000000000001)
/** PPRSup: Peripheral Page Request (PPR) support (RO). */
#define IOMMU_EXT_FEAT_BF_PPR_SUP_SHIFT                 1
#define IOMMU_EXT_FEAT_BF_PPR_SUP_MASK                  UINT64_C(0x0000000000000002)
/** XTSup: x2APIC support (RO). */
#define IOMMU_EXT_FEAT_BF_X2APIC_SUP_SHIFT              2
#define IOMMU_EXT_FEAT_BF_X2APIC_SUP_MASK               UINT64_C(0x0000000000000004)
/** NXSup: No Execute (PMR and PRIV) support (RO). */
#define IOMMU_EXT_FEAT_BF_NO_EXEC_SUP_SHIFT             3
#define IOMMU_EXT_FEAT_BF_NO_EXEC_SUP_MASK              UINT64_C(0x0000000000000008)
/** GTSup: Guest Translation support (RO). */
#define IOMMU_EXT_FEAT_BF_GT_SUP_SHIFT                  4
#define IOMMU_EXT_FEAT_BF_GT_SUP_MASK                   UINT64_C(0x0000000000000010)
/** Bit 5 reserved. */
#define IOMMU_EXT_FEAT_BF_RSVD_5_SHIFT                  5
#define IOMMU_EXT_FEAT_BF_RSVD_5_MASK                   UINT64_C(0x0000000000000020)
/** IASup: INVALIDATE_IOMMU_ALL command support (RO). */
#define IOMMU_EXT_FEAT_BF_IA_SUP_SHIFT                  6
#define IOMMU_EXT_FEAT_BF_IA_SUP_MASK                   UINT64_C(0x0000000000000040)
/** GASup: Guest virtual-APIC support (RO). */
#define IOMMU_EXT_FEAT_BF_GA_SUP_SHIFT                  7
#define IOMMU_EXT_FEAT_BF_GA_SUP_MASK                   UINT64_C(0x0000000000000080)
/** HESup: Hardware error registers support (RO). */
#define IOMMU_EXT_FEAT_BF_HE_SUP_SHIFT                  8
#define IOMMU_EXT_FEAT_BF_HE_SUP_MASK                   UINT64_C(0x0000000000000100)
/** PCSup: Performance counters support (RO). */
#define IOMMU_EXT_FEAT_BF_PC_SUP_SHIFT                  9
#define IOMMU_EXT_FEAT_BF_PC_SUP_MASK                   UINT64_C(0x0000000000000200)
/** HATS: Host Address Translation Size (RO). */
#define IOMMU_EXT_FEAT_BF_HATS_SHIFT                    10
#define IOMMU_EXT_FEAT_BF_HATS_MASK                     UINT64_C(0x0000000000000c00)
/** GATS: Guest Address Translation Size (RO). */
#define IOMMU_EXT_FEAT_BF_GATS_SHIFT                    12
#define IOMMU_EXT_FEAT_BF_GATS_MASK                     UINT64_C(0x0000000000003000)
/** GLXSup: Guest CR3 root table level support  (RO). */
#define IOMMU_EXT_FEAT_BF_GLX_SUP_SHIFT                 14
#define IOMMU_EXT_FEAT_BF_GLX_SUP_MASK                  UINT64_C(0x000000000000c000)
/** SmiFSup: SMI filter register support  (RO). */
#define IOMMU_EXT_FEAT_BF_SMI_FLT_SUP_SHIFT             16
#define IOMMU_EXT_FEAT_BF_SMI_FLT_SUP_MASK              UINT64_C(0x0000000000030000)
/** SmiFRC: SMI filter register count  (RO). */
#define IOMMU_EXT_FEAT_BF_SMI_FLT_REG_CNT_SHIFT         18
#define IOMMU_EXT_FEAT_BF_SMI_FLT_REG_CNT_MASK          UINT64_C(0x00000000001c0000)
/** GAMSup: Guest virtual-APIC modes support (RO). */
#define IOMMU_EXT_FEAT_BF_GAM_SUP_SHIFT                 21
#define IOMMU_EXT_FEAT_BF_GAM_SUP_MASK                  UINT64_C(0x0000000000e00000)
/** DualPprLogSup: Dual PPR Log support (RO). */
#define IOMMU_EXT_FEAT_BF_DUAL_PPR_LOG_SUP_SHIFT        24
#define IOMMU_EXT_FEAT_BF_DUAL_PPR_LOG_SUP_MASK         UINT64_C(0x0000000003000000)
/** Bits 27:26 reserved. */
#define IOMMU_EXT_FEAT_BF_RSVD_26_27_SHIFT              26
#define IOMMU_EXT_FEAT_BF_RSVD_26_27_MASK               UINT64_C(0x000000000c000000)
/** DualEventLogSup: Dual Event Log support (RO). */
#define IOMMU_EXT_FEAT_BF_DUAL_EVT_LOG_SUP_SHIFT        28
#define IOMMU_EXT_FEAT_BF_DUAL_EVT_LOG_SUP_MASK         UINT64_C(0x0000000030000000)
/** Bits 31:30 reserved. */
#define IOMMU_EXT_FEAT_BF_RSVD_30_31_SHIFT              30
#define IOMMU_EXT_FEAT_BF_RSVD_30_31_MASK               UINT64_C(0x00000000c0000000)
/** PASMax: Maximum PASID support (RO). */
#define IOMMU_EXT_FEAT_BF_PASID_MAX_SHIFT               32
#define IOMMU_EXT_FEAT_BF_PASID_MAX_MASK                UINT64_C(0x0000001f00000000)
/** USSup: User/Supervisor support (RO). */
#define IOMMU_EXT_FEAT_BF_US_SUP_SHIFT                  37
#define IOMMU_EXT_FEAT_BF_US_SUP_MASK                   UINT64_C(0x0000002000000000)
/** DevTblSegSup: Segmented Device Table support (RO). */
#define IOMMU_EXT_FEAT_BF_DEV_TBL_SEG_SUP_SHIFT         38
#define IOMMU_EXT_FEAT_BF_DEV_TBL_SEG_SUP_MASK          UINT64_C(0x000000c000000000)
/** PprOverflwEarlySup: PPR Log Overflow Early warning support (RO). */
#define IOMMU_EXT_FEAT_BF_PPR_OVERFLOW_EARLY_SHIFT      40
#define IOMMU_EXT_FEAT_BF_PPR_OVERFLOW_EARLY_MASK       UINT64_C(0x0000010000000000)
/** PprAutoRspSup: PPR Automatic Response support (RO). */
#define IOMMU_EXT_FEAT_BF_PPR_AUTO_RES_SUP_SHIFT        41
#define IOMMU_EXT_FEAT_BF_PPR_AUTO_RES_SUP_MASK         UINT64_C(0x0000020000000000)
/** MarcSup: Memory Access and Routing (MARC) support (RO). */
#define IOMMU_EXT_FEAT_BF_MARC_SUP_SHIFT                42
#define IOMMU_EXT_FEAT_BF_MARC_SUP_MASK                 UINT64_C(0x00000c0000000000)
/** BlkStopMrkSup: Block StopMark message support (RO). */
#define IOMMU_EXT_FEAT_BF_BLKSTOP_MARK_SUP_SHIFT        44
#define IOMMU_EXT_FEAT_BF_BLKSTOP_MARK_SUP_MASK         UINT64_C(0x0000100000000000)
/** PerfOptSup: IOMMU Performance Optimization support (RO). */
#define IOMMU_EXT_FEAT_BF_PERF_OPT_SUP_SHIFT            45
#define IOMMU_EXT_FEAT_BF_PERF_OPT_SUP_MASK             UINT64_C(0x0000200000000000)
/** MsiCapMmioSup: MSI-Capability Register MMIO access support (RO). */
#define IOMMU_EXT_FEAT_BF_MSI_CAP_MMIO_SUP_SHIFT        46
#define IOMMU_EXT_FEAT_BF_MSI_CAP_MMIO_SUP_MASK         UINT64_C(0x0000400000000000)
/** Bit 47 reserved. */
#define IOMMU_EXT_FEAT_BF_RSVD_47_SHIFT                 47
#define IOMMU_EXT_FEAT_BF_RSVD_47_MASK                  UINT64_C(0x0000800000000000)
/** GIoSup: Guest I/O Protection support (RO). */
#define IOMMU_EXT_FEAT_BF_GST_IO_PROT_SUP_SHIFT         48
#define IOMMU_EXT_FEAT_BF_GST_IO_PROT_SUP_MASK          UINT64_C(0x0001000000000000)
/** HASup: Host Access support (RO). */
#define IOMMU_EXT_FEAT_BF_HST_ACCESS_SUP_SHIFT          49
#define IOMMU_EXT_FEAT_BF_HST_ACCESS_SUP_MASK           UINT64_C(0x0002000000000000)
/** EPHSup: Enhandled PPR Handling support (RO). */
#define IOMMU_EXT_FEAT_BF_ENHANCED_PPR_SUP_SHIFT        50
#define IOMMU_EXT_FEAT_BF_ENHANCED_PPR_SUP_MASK         UINT64_C(0x0004000000000000)
/** AttrFWSup: Attribute Forward support (RO). */
#define IOMMU_EXT_FEAT_BF_ATTR_FW_SUP_SHIFT             51
#define IOMMU_EXT_FEAT_BF_ATTR_FW_SUP_MASK              UINT64_C(0x0008000000000000)
/** HDSup: Host Dirty Support (RO). */
#define IOMMU_EXT_FEAT_BF_HST_DIRTY_SUP_SHIFT           52
#define IOMMU_EXT_FEAT_BF_HST_DIRTY_SUP_MASK            UINT64_C(0x0010000000000000)
/** Bit 53 reserved. */
#define IOMMU_EXT_FEAT_BF_RSVD_53_SHIFT                 53
#define IOMMU_EXT_FEAT_BF_RSVD_53_MASK                  UINT64_C(0x0020000000000000)
/** InvIotlbTypeSup: Invalidate IOTLB type support (RO). */
#define IOMMU_EXT_FEAT_BF_INV_IOTLB_TYPE_SUP_SHIFT      54
#define IOMMU_EXT_FEAT_BF_INV_IOTLB_TYPE_SUP_MASK       UINT64_C(0x0040000000000000)
/** Bits 60:55 reserved. */
#define IOMMU_EXT_FEAT_BF_RSVD_55_60_SHIFT              55
#define IOMMU_EXT_FEAT_BF_RSVD_55_60_MASK               UINT64_C(0x1f80000000000000)
/** GAUpdateDisSup: Support disabling hardware update on guest page table access
 *  (RO). */
#define IOMMU_EXT_FEAT_BF_GA_UPDATE_DIS_SUP_SHIFT       61
#define IOMMU_EXT_FEAT_BF_GA_UPDATE_DIS_SUP_MASK        UINT64_C(0x2000000000000000)
/** ForcePhysDestSup: Force Physical Destination Mode for Remapped Interrupt
 *  support (RO). */
#define IOMMU_EXT_FEAT_BF_FORCE_PHYS_DST_SUP_SHIFT      62
#define IOMMU_EXT_FEAT_BF_FORCE_PHYS_DST_SUP_MASK       UINT64_C(0x4000000000000000)
/** Bit 63 reserved. */
#define IOMMU_EXT_FEAT_BF_RSVD_63_SHIFT                 63
#define IOMMU_EXT_FEAT_BF_RSVD_63_MASK                  UINT64_C(0x8000000000000000)
RT_BF_ASSERT_COMPILE_CHECKS(IOMMU_EXT_FEAT_BF_, UINT64_C(0), UINT64_MAX,
                            (PREF_SUP, PPR_SUP, X2APIC_SUP, NO_EXEC_SUP, GT_SUP, RSVD_5, IA_SUP, GA_SUP, HE_SUP, PC_SUP,
                             HATS, GATS, GLX_SUP, SMI_FLT_SUP, SMI_FLT_REG_CNT, GAM_SUP, DUAL_PPR_LOG_SUP, RSVD_26_27,
                             DUAL_EVT_LOG_SUP, RSVD_30_31, PASID_MAX, US_SUP, DEV_TBL_SEG_SUP, PPR_OVERFLOW_EARLY,
                             PPR_AUTO_RES_SUP, MARC_SUP, BLKSTOP_MARK_SUP, PERF_OPT_SUP, MSI_CAP_MMIO_SUP, RSVD_47,
                             GST_IO_PROT_SUP, HST_ACCESS_SUP, ENHANCED_PPR_SUP, ATTR_FW_SUP, HST_DIRTY_SUP, RSVD_53,
                             INV_IOTLB_TYPE_SUP, RSVD_55_60, GA_UPDATE_DIS_SUP, FORCE_PHYS_DST_SUP, RSVD_63));
/** @} */

/**
 * IVHD (I/O Virtualization Hardware Definition) Type 10h.
 * In accordance with the AMD spec.
 */
#pragma pack(1)
typedef struct ACPIIVHDTYPE10
{
    uint8_t         u8Type;                 /**< Type: Must be 0x10. */
    uint8_t         u8Flags;                /**< Flags (see ACPI_IVHD_10H_F_XXX). */
    uint16_t        u16Length;              /**< Length of IVHD including IVHD device entries. */
    uint16_t        u16DeviceId;            /**< Device ID of the IOMMU. */
    uint16_t        u16CapOffset;           /**< Offset in Capability space for control fields of IOMMU. */
    uint64_t        u64BaseAddress;         /**< Base address of IOMMU control registers in MMIO space. */
    uint16_t        u16PciSegmentGroup;     /**< PCI segment group number. */
    uint16_t        u16IommuInfo;           /**< Interrupt number and Unit ID. */
    uint32_t        u32Features;            /**< IOMMU feature reporting. */
    /* IVHD device entry block follows. */
} ACPIIVHDTYPE10;
#pragma pack()
AssertCompileSize(ACPIIVHDTYPE10, 24);
AssertCompileMemberOffset(ACPIIVHDTYPE10, u8Type,               0);
AssertCompileMemberOffset(ACPIIVHDTYPE10, u8Flags,              1);
AssertCompileMemberOffset(ACPIIVHDTYPE10, u16Length,            2);
AssertCompileMemberOffset(ACPIIVHDTYPE10, u16DeviceId,          4);
AssertCompileMemberOffset(ACPIIVHDTYPE10, u16CapOffset,         6);
AssertCompileMemberOffset(ACPIIVHDTYPE10, u64BaseAddress,       8);
AssertCompileMemberOffset(ACPIIVHDTYPE10, u16PciSegmentGroup,   16);
AssertCompileMemberOffset(ACPIIVHDTYPE10, u16IommuInfo,         18);
AssertCompileMemberOffset(ACPIIVHDTYPE10, u32Features,          20);

/** @name IVHD Type 11h Flags.
 * In accordance with the AMD spec.
 * @{ */
/** Coherent control. */
#define ACPI_IVHD_11H_F_COHERENT                        RT_BIT(5)
/** Remote IOTLB support. */
#define ACPI_IVHD_11H_F_IOTLB_SUP                       RT_BIT(4)
/** Isochronous control. */
#define ACPI_IVHD_11H_F_ISOC                            RT_BIT(3)
/** Response Pass Posted Write. */
#define ACPI_IVHD_11H_F_RES_PASS_PW                     RT_BIT(2)
/** Pass Posted Write. */
#define ACPI_IVHD_11H_F_PASS_PW                         RT_BIT(1)
/** HyperTransport Tunnel. */
#define ACPI_IVHD_11H_F_HT_TUNNEL                       RT_BIT(0)
/** @} */

/** @name IVHD IOMMU Type 11 Attributes field.
 * In accordance with the AMD spec.
 * @{ */
/** Bits 12:0 reserved. */
#define ACPI_IOMMU_ATTR_BF_RSVD_0_12_SHIFT              0
#define ACPI_IOMMU_ATTR_BF_RSVD_0_12_MASK               UINT32_C(0x00001fff)
/** PNCounters: Number of performance counters per counter bank. */
#define ACPI_IOMMU_ATTR_BF_PN_COUNTERS_SHIFT            13
#define ACPI_IOMMU_ATTR_BF_PN_COUNTERS_MASK             UINT32_C(0x0001e000)
/** PNBanks: Number of performance counter banks. */
#define ACPI_IOMMU_ATTR_BF_PN_BANKS_SHIFT               17
#define ACPI_IOMMU_ATTR_BF_PN_BANKS_MASK                UINT32_C(0x007e0000)
/** MSINumPPR: MSI number for peripheral page requests (PPR). */
#define ACPI_IOMMU_ATTR_BF_MSI_NUM_PPR_SHIFT            23
#define ACPI_IOMMU_ATTR_BF_MSI_NUM_PPR_MASK             UINT32_C(0x0f800000)
/** Bits 31:28 reserved. */
#define ACPI_IOMMU_ATTR_BF_RSVD_28_31_SHIFT             28
#define ACPI_IOMMU_ATTR_BF_RSVD_28_31_MASK              UINT32_C(0xf0000000)
RT_BF_ASSERT_COMPILE_CHECKS(ACPI_IOMMU_ATTR_BF_, UINT32_C(0), UINT32_MAX,
                            (RSVD_0_12, PN_COUNTERS, PN_BANKS, MSI_NUM_PPR, RSVD_28_31));
/** @} */

/**
 * AMD IOMMU: IVHD (I/O Virtualization Hardware Definition) Type 11h.
 * In accordance with the AMD spec.
 */
#pragma pack(1)
typedef struct ACPIIVHDTYPE11
{
    uint8_t         u8Type;                 /**< Type: Must be 0x11. */
    uint8_t         u8Flags;                /**< Flags. */
    uint16_t        u16Length;              /**< Length: Size starting from Type fields incl. IVHD device entries. */
    uint16_t        u16DeviceId;            /**< Device ID of the IOMMU. */
    uint16_t        u16CapOffset;           /**< Offset in Capability space for control fields of IOMMU. */
    uint64_t        u64BaseAddress;         /**< Base address of IOMMU control registers in MMIO space. */
    uint16_t        u16PciSegmentGroup;     /**< PCI segment group number. */
    uint16_t        u16IommuInfo;           /**< Interrupt number and unit ID. */
    uint32_t        u32IommuAttr;           /**< IOMMU info. not reported in EFR. */
    uint64_t        u64EfrRegister;         /**< Extended Feature Register (must be identical to its MMIO shadow). */
    uint64_t        u64Rsvd0;               /**< Reserved for future. */
    /* IVHD device entry block follows. */
} ACPIIVHDTYPE11;
#pragma pack()
AssertCompileSize(ACPIIVHDTYPE11, 40);
AssertCompileMemberOffset(ACPIIVHDTYPE11, u8Type,               0);
AssertCompileMemberOffset(ACPIIVHDTYPE11, u8Flags,              1);
AssertCompileMemberOffset(ACPIIVHDTYPE11, u16Length,            2);
AssertCompileMemberOffset(ACPIIVHDTYPE11, u16DeviceId,          4);
AssertCompileMemberOffset(ACPIIVHDTYPE11, u16CapOffset,         6);
AssertCompileMemberOffset(ACPIIVHDTYPE11, u64BaseAddress,       8);
AssertCompileMemberOffset(ACPIIVHDTYPE11, u16PciSegmentGroup,   16);
AssertCompileMemberOffset(ACPIIVHDTYPE11, u16IommuInfo,         18);
AssertCompileMemberOffset(ACPIIVHDTYPE11, u32IommuAttr,         20);
AssertCompileMemberOffset(ACPIIVHDTYPE11, u64EfrRegister,       24);
AssertCompileMemberOffset(ACPIIVHDTYPE11, u64Rsvd0,             32);

/**
 * AMD IOMMU: IVHD (I/O Virtualization Hardware Definition) Type 40h.
 * In accordance with the AMD spec.
 */
typedef struct ACPIIVHDTYPE11 ACPIIVHDTYPE40;

#endif /* !VBOX_INCLUDED_iommu_amd_h */
