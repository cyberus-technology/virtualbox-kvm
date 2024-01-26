/** @file

  Implementation of the SNP.Initialize() function and its private helpers if
  any.

  Copyright (c) 2021, Oracle and/or its affiliates.
  Copyright (c) 2017, AMD Inc, All rights reserved.
  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "E1kNet.h"

/**
  Set up static scaffolding for the E1kNetTransmit() and
  E1kNetGetStatus() SNP methods.

  This function may only be called by E1kNetInitialize().

  @param[in,out] Dev       The E1K_NET_DEV driver instance about to enter the
                           EfiSimpleNetworkInitialized state.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the stack to track the heads
                                of free descriptor chains or failed to init
                                TxBufCollection.
  @return                       Status codes from VIRTIO_DEVICE_PROTOCOL.
                                AllocateSharedPages() or
                                VirtioMapAllBytesInSharedBuffer()
  @retval EFI_SUCCESS           TX setup successful.
*/

STATIC
EFI_STATUS
EFIAPI
E1kNetInitTx (
  IN OUT E1K_NET_DEV *Dev
  )
{
  UINTN                 TxRingSize;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  DeviceAddress;
  VOID                  *TxRingBuffer;

  Dev->TxMaxPending = E1K_NET_MAX_PENDING;
  Dev->TxCurPending = 0;
  Dev->TxBufCollection = OrderedCollectionInit (
                           E1kNetTxBufMapInfoCompare,
                           E1kNetTxBufDeviceAddressCompare
                           );
  if (Dev->TxBufCollection == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Allocate TxRing header and map with BusMasterCommonBuffer so that it
  // can be accessed equally by both processor and device.
  //
  TxRingSize = Dev->TxMaxPending * sizeof (*Dev->TxRing);
  Status = Dev->PciIo->AllocateBuffer (
                          Dev->PciIo,
                          AllocateAnyPages,
                          EfiBootServicesData,
                          EFI_SIZE_TO_PAGES (TxRingSize),
                          &TxRingBuffer,
                          EFI_PCI_ATTRIBUTE_MEMORY_CACHED
                          );
  if (EFI_ERROR (Status)) {
    goto UninitTxBufCollection;
  }

  ZeroMem (TxRingBuffer, TxRingSize);

  Status = Dev->PciIo->Map (
                         Dev->PciIo,
                         EfiPciIoOperationBusMasterCommonBuffer,
                         TxRingBuffer,
                         &TxRingSize,
                         &DeviceAddress,
                         &Dev->TxRingMap
                         );
  if (EFI_ERROR (Status)) {
    goto FreeTxRingBuffer;
  }

  Dev->TxRing      = TxRingBuffer;
  Dev->TdhLastSeen = 0;
  Dev->TxLastUsed  = 0;

  // Program the transmit engine.
  MemoryFence ();
  E1kNetRegWrite32(Dev, E1K_REG_TDBAL, (UINT32)DeviceAddress);
  E1kNetRegWrite32(Dev, E1K_REG_TDBAH, (UINT32)(RShiftU64 (DeviceAddress, 32)));
  E1kNetRegWrite32(Dev, E1K_REG_TDLEN, (UINT32)TxRingSize);
  E1kNetRegWrite32(Dev, E1K_REG_TDH, 0);
  E1kNetRegWrite32(Dev, E1K_REG_TDT, 0);
  E1kNetRegWrite32(Dev, E1K_REG_TCTL, E1K_REG_TCTL_EN | E1K_REG_TCTL_PSP);

  return EFI_SUCCESS;

FreeTxRingBuffer:
  Dev->PciIo->FreeBuffer (
                 Dev->PciIo,
                 EFI_SIZE_TO_PAGES (TxRingSize),
                 TxRingBuffer
                 );

UninitTxBufCollection:
  OrderedCollectionUninit (Dev->TxBufCollection);

Exit:
  return Status;
}


/**
  Set up static scaffolding for the E1kNetReceive() SNP method and enable
  live device operation.

  This function may only be called as E1kNetInitialize()'s final step.

  @param[in,out] Dev       The E1K_NET_DEV driver instance about to enter the
                           EfiSimpleNetworkInitialized state.

  @return                       Status codes from VIRTIO_CFG_WRITE() or
                                VIRTIO_DEVICE_PROTOCOL.AllocateSharedPages or
                                VirtioMapAllBytesInSharedBuffer().
  @retval EFI_SUCCESS           RX setup successful. The device is live and may
                                already be writing to the receive area.
*/

STATIC
EFI_STATUS
EFIAPI
E1kNetInitRx (
  IN OUT E1K_NET_DEV *Dev
  )
{
  EFI_STATUS            Status;
  UINTN                 RxBufSize;
  UINTN                 PktIdx;
  UINTN                 NumBytes;
  EFI_PHYSICAL_ADDRESS  RxBufDeviceAddress;
  VOID                  *RxBuffer;

  //
  // For each incoming packet we must supply two buffers:
  // - the recipient for the RX descriptor, plus
  // - the recipient for the network data (which consists of Ethernet header
  //   and Ethernet payload) which is a 2KB buffer.
  //
  RxBufSize = sizeof(*Dev->RxRing) + 2048;

  //
  // The RxBuf is shared between guest and hypervisor, use
  // AllocateSharedPages() to allocate this memory region and map it with
  // BusMasterCommonBuffer so that it can be accessed by both guest and
  // hypervisor.
  //
  NumBytes = E1K_NET_MAX_PENDING * RxBufSize;
  Dev->RxBufNrPages = EFI_SIZE_TO_PAGES (NumBytes);
  Status = Dev->PciIo->AllocateBuffer (
                          Dev->PciIo,
                          AllocateAnyPages,
                          EfiBootServicesData,
                          Dev->RxBufNrPages,
                          &RxBuffer,
                          EFI_PCI_ATTRIBUTE_MEMORY_CACHED
                          );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (RxBuffer, NumBytes);

  Status = Dev->PciIo->Map (
                         Dev->PciIo,
                         EfiPciIoOperationBusMasterCommonBuffer,
                         RxBuffer,
                         &NumBytes,
                         &Dev->RxDeviceBase,
                         &Dev->RxMap
                         );
  if (EFI_ERROR (Status)) {
    goto FreeSharedBuffer;
  }

  Dev->RxRing = RxBuffer;
  Dev->RxBuf  = (UINT8 *)RxBuffer + sizeof(*Dev->RxRing) * E1K_NET_MAX_PENDING;
  Dev->RdhLastSeen = 0;

  // Set up the RX descriptors.
  Dev->RxBufDeviceBase = Dev->RxDeviceBase + sizeof(*Dev->RxRing) * E1K_NET_MAX_PENDING;
  RxBufDeviceAddress = Dev->RxBufDeviceBase;
  for (PktIdx = 0; PktIdx < E1K_NET_MAX_PENDING; ++PktIdx) {
    Dev->RxRing[PktIdx].AddrBufferLow  = (UINT32)RxBufDeviceAddress;
    Dev->RxRing[PktIdx].AddrBufferHigh = (UINT32)RShiftU64(RxBufDeviceAddress, 32);
    Dev->RxRing[PktIdx].BufferLength   = 2048;

    RxBufDeviceAddress += Dev->RxRing[PktIdx].BufferLength;
  }

  // Program the receive engine.
  MemoryFence ();
  E1kNetRegWrite32(Dev, E1K_REG_RDBAL, (UINT32)Dev->RxDeviceBase);
  E1kNetRegWrite32(Dev, E1K_REG_RDBAH, (UINT32)(RShiftU64 (Dev->RxDeviceBase, 32)));
  E1kNetRegWrite32(Dev, E1K_REG_RDLEN, sizeof(*Dev->RxRing) * E1K_NET_MAX_PENDING);
  E1kNetRegWrite32(Dev, E1K_REG_RDH, 0);
  E1kNetRegWrite32(Dev, E1K_REG_RDT, E1K_NET_MAX_PENDING - 1);
  E1kNetRegClear32(Dev, E1K_REG_RCTL, E1K_REG_RCTL_BSIZE_MASK);
  E1kNetRegSet32(Dev, E1K_REG_RCTL, E1K_REG_RCTL_EN | E1K_REG_RCTL_MPE);

  return EFI_SUCCESS;

FreeSharedBuffer:
  Dev->PciIo->FreeBuffer (
                 Dev->PciIo,
                 Dev->RxBufNrPages,
                 RxBuffer
                 );
  return Status;
}

/**
  Resets a network adapter and allocates the transmit and receive buffers
  required by the network interface; optionally, also requests allocation  of
  additional transmit and receive buffers.

  @param  This              The protocol instance pointer.
  @param  ExtraRxBufferSize The size, in bytes, of the extra receive buffer
                            space that the driver should allocate for the
                            network interface. Some network interfaces will not
                            be able to use the extra buffer, and the caller
                            will not know if it is actually being used.
  @param  ExtraTxBufferSize The size, in bytes, of the extra transmit buffer
                            space that the driver should allocate for the
                            network interface. Some network interfaces will not
                            be able to use the extra buffer, and the caller
                            will not know if it is actually being used.

  @retval EFI_SUCCESS           The network interface was initialized.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_OUT_OF_RESOURCES  There was not enough memory for the transmit
                                and receive buffers.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an
                                unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network
                                interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network
                                interface.

**/

EFI_STATUS
EFIAPI
E1kNetInitialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINTN                       ExtraRxBufferSize  OPTIONAL,
  IN UINTN                       ExtraTxBufferSize  OPTIONAL
  )
{
  E1K_NET_DEV *Dev;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  DEBUG((DEBUG_INFO, "E1kNetInitialize:\n"));

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (ExtraRxBufferSize > 0 || ExtraTxBufferSize > 0) {
    return EFI_UNSUPPORTED;
  }

  Dev = E1K_NET_FROM_SNP (This);
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  if (Dev->Snm.State != EfiSimpleNetworkStarted) {
    Status = EFI_NOT_STARTED;
    goto InitFailed;
  }

  // Program the first Receive Address Low/High register.
  E1kNetRegSet32(Dev, E1K_REG_CTRL, E1K_REG_CTRL_ASDE | E1K_REG_CTRL_SLU);
  E1kNetRegWrite32(Dev, E1K_REG_RAL, *(UINT32 *)&Dev->Snm.CurrentAddress.Addr[0]);
  E1kNetRegWrite32(Dev, E1K_REG_RAH, (*(UINT32 *)&Dev->Snm.CurrentAddress.Addr[4]) | E1K_REG_RAH_AV);

  Status = E1kNetInitTx (Dev);
  if (EFI_ERROR (Status)) {
    goto AbortDevice;
  }

  //
  // start receiving
  //
  Status = E1kNetInitRx (Dev);
  if (EFI_ERROR (Status)) {
    goto ReleaseTxAux;
  }

  Dev->Snm.State = EfiSimpleNetworkInitialized;
  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;

ReleaseTxAux:
  E1kNetShutdownTx (Dev);

AbortDevice:
  E1kNetDevReset(Dev);

InitFailed:
  gBS->RestoreTPL (OldTpl);
  return Status;
}
