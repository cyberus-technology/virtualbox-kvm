/** @file

  E1000 hardware interface definitions.

  Copyright (c) 2021, Oracle and/or its affiliates.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _E1K_NET_HW_H_
#define _E1K_NET_HW_H_

#define INTEL_PCI_VENDOR_ID         0x8086
#define INTEL_82540EM_PCI_DEVICE_ID 0x100e
#define INTEL_82543GC_PCI_DEVICE_ID 0x1004
#define INTEL_82545EM_PCI_DEVICE_ID 0x100f

//
// Receive descriptor.
//
typedef struct {
  UINT32                          AddrBufferLow;
  UINT32                          AddrBufferHigh;
  UINT16                          BufferLength;
  UINT16                          Checksum;
  UINT8                           Status;
  UINT8                           Errors;
  UINT16                          Special;
} E1K_RX_DESC;

#define E1K_RX_STATUS_DONE        BIT0
#define E1K_RX_STATUS_EOP         BIT1

#define E1K_RX_ERROR_CE           BIT0
#define E1K_RX_ERROR_SEQ          BIT2
#define E1K_RX_ERROR_CXE          BIT4
#define E1K_RX_ERROR_RXE          BIT7

//
// Transmit descriptor.
//
typedef struct {
  UINT32                          AddrBufferLow;
  UINT32                          AddrBufferHigh;
  UINT16                          BufferLength;
  UINT8                           ChecksumOffset;
  UINT8                           Command;
  UINT8                           Status;
  UINT8                           ChecksumStart;
  UINT16                          Special;
} E1K_TX_DESC;

#define E1K_TX_CMD_EOP            BIT0
#define E1K_TX_CMD_FCS            BIT1
#define E1K_TX_CMD_RS             BIT3

#define E1K_REG_CTRL              0x00000000
# define E1K_REG_CTRL_ASDE        BIT5
# define E1K_REG_CTRL_SLU         BIT6
# define E1K_REG_CTRL_RST         BIT26
# define E1K_REG_CTRL_PHY_RST     BIT31
#define E1K_REG_STATUS            0x00000008
# define E1K_REG_STATUS_LU        BIT1
#define E1K_REG_EECD              0x00000010
#define E1K_REG_EERD              0x00000014
# define E1K_REG_EERD_START       BIT0
# define E1K_REG_EERD_DONE        BIT4
# define E1K_REG_EERD_DATA_GET(x) (((x) >> 16) & 0xffff)
#define E1K_REG_ICR               0x000000c0
#define E1K_REG_ITR               0x000000c4
#define E1K_REG_ICS               0x000000c8
#define E1K_REG_IMS               0x000000d0
#define E1K_REG_IMC               0x000000d8
#define E1K_REG_RCTL              0x00000100
# define E1K_REG_RCTL_EN          BIT1
# define E1K_REG_RCTL_MPE         BIT4
# define E1K_REG_RCTL_BSIZE_MASK  0x00030000
#define E1K_REG_RDBAL             0x00002800
#define E1K_REG_RDBAH             0x00002804
#define E1K_REG_RDLEN             0x00002808
#define E1K_REG_RDH               0x00002810
#define E1K_REG_RDT               0x00002818
#define E1K_REG_RDTR              0x00002820
#define E1K_REG_TCTL              0x00000400
# define E1K_REG_TCTL_EN          BIT1
# define E1K_REG_TCTL_PSP         BIT3
#define E1K_REG_TIPG              0x00000410
#define E1K_REG_TDBAL             0x00003800
#define E1K_REG_TDBAH             0x00003804
#define E1K_REG_TDLEN             0x00003808
#define E1K_REG_TDH               0x00003810
#define E1K_REG_TDT               0x00003818
#define E1K_REG_RAL               0x00005400
#define E1K_REG_RAH               0x00005404
# define E1K_REG_RAH_AV           BIT31

//
// MAC address.
//
typedef struct
{
  UINT8                           Mac[6];
} E1K_NET_MAC;

#endif // _E1K_NET_HW_H_
