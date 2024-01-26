/** @file

  Internal definitions for the virtio-net driver, which produces Simple Network
  Protocol instances for virtio-net devices.

  Copyright (c) 2021, Oracle and/or its affiliates.
  Copyright (c) 2017, AMD Inc, All rights reserved.
  Copyright (C) 2013, Red Hat, Inc.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _E1K_NET_DXE_H_
#define _E1K_NET_DXE_H_

#include <Library/DebugLib.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/SimpleNetwork.h>
#include <Library/OrderedCollectionLib.h>

#include "E1kNetHw.h"

#define E1K_NET_DEV_SIGNATURE SIGNATURE_32 ('E','1','K','N')

//
// maximum number of pending packets, separately for each direction
//
#define E1K_NET_MAX_PENDING 64

//
// State diagram:
//
//                  |     ^
//                  |     |
//        BindingStart  BindingStop
//        +SnpPopulate    |
//        ++GetFeatures   |
//                  |     |
//                  v     |
//                +---------+    virtio-net device is reset, no resources are
//                | stopped |    allocated for traffic, but MAC address has
//                +---------+    been retrieved
//                  |     ^
//                  |     |
//            SNP.Start SNP.Stop
//                  |     |
//                  v     |
//                +---------+
//                | started |    functionally identical to stopped
//                +---------+
//                  |     ^
//                  |     |
//       SNP.Initialize SNP.Shutdown
//                  |     |
//                  v     |
//              +-------------+  Virtio-net setup complete, including DRIVER_OK
//              | initialized |  bit. The receive queue is populated with
//              +-------------+  requests; McastIpToMac, GetStatus, Transmit,
//                               Receive are callable.
//

typedef struct {
  //
  // Parts of this structure are initialized / torn down in various functions
  // at various call depths. The table to the right should make it easier to
  // track them.
  //
  //                          field              init function
  //                          ------------------ ------------------------------
  UINT32                      Signature;         // VirtioNetDriverBindingStart
  EFI_PCI_IO_PROTOCOL         *PciIo;            // VirtioNetDriverBindingStart
  UINT64                      OriginalPciAttributes; // VirtioNetDriverBindingStart
  EFI_SIMPLE_NETWORK_PROTOCOL Snp;               // VirtioNetSnpPopulate
  EFI_SIMPLE_NETWORK_MODE     Snm;               // VirtioNetSnpPopulate
  EFI_EVENT                   ExitBoot;          // VirtioNetSnpPopulate
  EFI_DEVICE_PATH_PROTOCOL    *MacDevicePath;    // VirtioNetDriverBindingStart
  EFI_HANDLE                  MacHandle;         // VirtioNetDriverBindingStart

  E1K_RX_DESC                 *RxRing;           // VirtioNetInitRing
  UINT8                       *RxBuf;            // E1kNetInitRx
  UINT32                      RdhLastSeen;       // E1kNetInitRx
  UINTN                       RxBufNrPages;      // E1kNetInitRx
  EFI_PHYSICAL_ADDRESS        RxBufDeviceBase;   // E1kNetInitRx
  EFI_PHYSICAL_ADDRESS        RxDeviceBase;      // E1kNetInitRx
  VOID                        *RxMap;            // E1kNetInitRx

  UINT16                      TxMaxPending;      // E1kNetInitTx
  UINT16                      TxCurPending;      // E1kNetInitTx
  E1K_TX_DESC                 *TxRing;           // E1kNetInitTx
  VOID                        *TxRingMap;        // E1kNetInitTx
  UINT16                      TxLastUsed;        // E1kNetInitTx
  UINT32                      TdhLastSeen;       // E1kNetInitTx
  ORDERED_COLLECTION          *TxBufCollection;  // E1kNetInitTx
} E1K_NET_DEV;


//
// In order to avoid duplication of interface documentation, please find all
// leading comments near the respective function / variable definitions (not
// the declarations here), which is where your code editor of choice takes you
// anyway when jumping to a function.
//

//
// utility macros
//
#define E1K_NET_FROM_SNP(SnpPointer) \
        CR (SnpPointer, E1K_NET_DEV, Snp, E1K_NET_DEV_SIGNATURE)

//
// component naming
//
extern EFI_COMPONENT_NAME_PROTOCOL gE1kNetComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gE1kNetComponentName2;

//
// driver binding
//
extern EFI_DRIVER_BINDING_PROTOCOL gE1kNetDriverBinding;

//
// member functions implementing the Simple Network Protocol
//
EFI_STATUS
EFIAPI
E1kNetStart (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  );

EFI_STATUS
EFIAPI
E1kNetStop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  );

EFI_STATUS
EFIAPI
E1kNetInitialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINTN                       ExtraRxBufferSize  OPTIONAL,
  IN UINTN                       ExtraTxBufferSize  OPTIONAL
  );

EFI_STATUS
EFIAPI
E1kNetReset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     ExtendedVerification
  );

EFI_STATUS
EFIAPI
E1kNetShutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  );

EFI_STATUS
EFIAPI
E1kNetReceiveFilters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINT32                      Enable,
  IN UINT32                      Disable,
  IN BOOLEAN                     ResetMCastFilter,
  IN UINTN                       MCastFilterCnt    OPTIONAL,
  IN EFI_MAC_ADDRESS             *MCastFilter      OPTIONAL
  );

EFI_STATUS
EFIAPI
E1kNetStationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     Reset,
  IN EFI_MAC_ADDRESS             *New OPTIONAL
  );

EFI_STATUS
EFIAPI
E1kNetStatistics (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     Reset,
  IN OUT UINTN                   *StatisticsSize   OPTIONAL,
  OUT EFI_NETWORK_STATISTICS     *StatisticsTable  OPTIONAL
  );

EFI_STATUS
EFIAPI
E1kNetMcastIpToMac (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     IPv6,
  IN EFI_IP_ADDRESS              *Ip,
  OUT EFI_MAC_ADDRESS            *Mac
  );

EFI_STATUS
EFIAPI
E1kNetNvData (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     ReadWrite,
  IN UINTN                       Offset,
  IN UINTN                       BufferSize,
  IN OUT VOID                    *Buffer
  );

EFI_STATUS
EFIAPI
E1kNetGetStatus (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  OUT UINT32                     *InterruptStatus OPTIONAL,
  OUT VOID                       **TxBuf OPTIONAL
  );

EFI_STATUS
EFIAPI
E1kNetTransmit (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINTN                       HeaderSize,
  IN UINTN                       BufferSize,
  IN /* +OUT! */ VOID            *Buffer,
  IN EFI_MAC_ADDRESS             *SrcAddr  OPTIONAL,
  IN EFI_MAC_ADDRESS             *DestAddr OPTIONAL,
  IN UINT16                      *Protocol OPTIONAL
  );

EFI_STATUS
EFIAPI
E1kNetReceive (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  OUT UINTN                      *HeaderSize OPTIONAL,
  IN OUT UINTN                   *BufferSize,
  OUT VOID                       *Buffer,
  OUT EFI_MAC_ADDRESS            *SrcAddr    OPTIONAL,
  OUT EFI_MAC_ADDRESS            *DestAddr   OPTIONAL,
  OUT UINT16                     *Protocol   OPTIONAL
  );

//
// utility functions shared by various SNP member functions
//
VOID
EFIAPI
E1kNetShutdownRx (
  IN OUT E1K_NET_DEV *Dev
  );

VOID
EFIAPI
E1kNetShutdownTx (
  IN OUT E1K_NET_DEV *Dev
  );

//
// utility functions to map caller-supplied Tx buffer system physical address
// to a device address and vice versa
//
EFI_STATUS
EFIAPI
E1kNetMapTxBuf (
  IN  E1K_NET_DEV           *Dev,
  IN  VOID                  *Buffer,
  IN  UINTN                 NumberOfBytes,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress
  );

EFI_STATUS
EFIAPI
E1kNetUnmapTxBuf (
  IN  E1K_NET_DEV              *Dev,
  OUT VOID                  **Buffer,
  IN  EFI_PHYSICAL_ADDRESS  DeviceAddress
  );

INTN
EFIAPI
E1kNetTxBufMapInfoCompare (
  IN CONST VOID *UserStruct1,
  IN CONST VOID *UserStruct2
  );

INTN
EFIAPI
E1kNetTxBufDeviceAddressCompare (
  IN CONST VOID *StandaloneKey,
  IN CONST VOID *UserStruct
  );


//
// event callbacks
//
VOID
EFIAPI
E1kNetIsPacketAvailable (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  );

VOID
EFIAPI
E1kNetExitBoot (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  );

//
// Hardware I/O functions.
//
EFI_STATUS
EFIAPI
E1kNetRegWrite32 (
  IN E1K_NET_DEV        *Dev,
  IN UINT32             Addr,
  IN UINT32             Data
  );

EFI_STATUS
EFIAPI
E1kNetRegRead32 (
  IN E1K_NET_DEV        *Dev,
  IN  UINT32            Addr,
  OUT UINT32            *Data
  );

EFI_STATUS
EFIAPI
E1kNetRegSet32 (
  IN E1K_NET_DEV        *Dev,
  IN UINT32             Addr,
  IN UINT32             Set
  );

EFI_STATUS
EFIAPI
E1kNetRegClear32 (
  IN E1K_NET_DEV        *Dev,
  IN UINT32             Addr,
  IN UINT32             Clear
  );

EFI_STATUS
EFIAPI
E1kNetDevReset (
  IN E1K_NET_DEV        *Dev
  );

#endif // _E1K_NET_DXE_H_
