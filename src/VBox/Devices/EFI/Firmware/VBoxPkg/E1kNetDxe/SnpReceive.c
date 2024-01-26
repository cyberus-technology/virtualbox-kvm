/** @file

  Implementation of the SNP.Receive() function and its private helpers if any.

  Copyright (c) 2021, Oracle and/or its affiliates.
  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "E1kNet.h"

/**
  Receives a packet from a network interface.

  @param  This       The protocol instance pointer.
  @param  HeaderSize The size, in bytes, of the media header received on the
                     network interface. If this parameter is NULL, then the
                     media header size will not be returned.
  @param  BufferSize On entry, the size, in bytes, of Buffer. On exit, the
                     size, in bytes, of the packet that was received on the
                     network interface.
  @param  Buffer     A pointer to the data buffer to receive both the media
                     header and the data.
  @param  SrcAddr    The source HW MAC address. If this parameter is NULL, the
                     HW MAC source address will not be extracted from the media
                     header.
  @param  DestAddr   The destination HW MAC address. If this parameter is NULL,
                     the HW MAC destination address will not be extracted from
                     the media header.
  @param  Protocol   The media header type. If this parameter is NULL, then the
                     protocol will not be extracted from the media header. See
                     RFC 1700 section "Ether Types" for examples.

  @retval  EFI_SUCCESS           The received data was stored in Buffer, and
                                 BufferSize has been updated to the number of
                                 bytes received.
  @retval  EFI_NOT_STARTED       The network interface has not been started.
  @retval  EFI_NOT_READY         The network interface is too busy to accept
                                 this transmit request.
  @retval  EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval  EFI_INVALID_PARAMETER One or more of the parameters has an
                                 unsupported value.
  @retval  EFI_DEVICE_ERROR      The command could not be sent to the network
                                 interface.
  @retval  EFI_UNSUPPORTED       This function is not supported by the network
                                 interface.

**/

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
  )
{
  E1K_NET_DEV *Dev;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;
  UINT32      RdhCur;
  UINT32      RxLen;
  UINTN       OrigBufferSize;
  EFI_PHYSICAL_ADDRESS BufferAddress;
  UINT8       *RxPtr;
  UINTN       RxBufOffset;

  DEBUG((DEBUG_INFO, "E1kNetReceive: HeaderSize=%p BufferSize=%u Buffer=%p\n",
         HeaderSize, *BufferSize, Buffer));

  if (This == NULL || BufferSize == NULL || Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = E1K_NET_FROM_SNP (This);
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  switch (Dev->Snm.State) {
  case EfiSimpleNetworkStopped:
    Status = EFI_NOT_STARTED;
    goto Exit;
  case EfiSimpleNetworkStarted:
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  default:
    break;
  }

  E1kNetRegRead32(Dev, E1K_REG_RDH, &RdhCur);

  if (Dev->RdhLastSeen == RdhCur) {
    Status = EFI_NOT_READY;
    goto Exit;
  }

  RxLen = Dev->RxRing[Dev->RdhLastSeen].BufferLength;
  //
  // the host must not have filled in more data than requested
  //
  ASSERT (RxLen <= 2048);

  OrigBufferSize = *BufferSize;
  *BufferSize = RxLen;

  if (OrigBufferSize < RxLen) {
    Status = EFI_BUFFER_TOO_SMALL;
    goto Exit; // keep the packet
  }

  if (RxLen < Dev->Snm.MediaHeaderSize) {
    Status = EFI_DEVICE_ERROR;
    goto RecycleDesc; // drop useless short packet
  }

  if (HeaderSize != NULL) {
    *HeaderSize = Dev->Snm.MediaHeaderSize;
  }

  BufferAddress = Dev->RxRing[Dev->RdhLastSeen].AddrBufferLow;
  BufferAddress |= LShiftU64(Dev->RxRing[Dev->RdhLastSeen].AddrBufferHigh, 32);
  RxBufOffset = (UINTN)(BufferAddress - Dev->RxBufDeviceBase);
  RxPtr = Dev->RxBuf + RxBufOffset;
  CopyMem (Buffer, RxPtr, RxLen);

  if (DestAddr != NULL) {
    CopyMem (DestAddr, RxPtr, sizeof (E1K_NET_MAC));
  }
  RxPtr += sizeof (E1K_NET_MAC);

  if (SrcAddr != NULL) {
    CopyMem (SrcAddr, RxPtr, sizeof (E1K_NET_MAC));
  }
  RxPtr += sizeof (E1K_NET_MAC);

  if (Protocol != NULL) {
    *Protocol = (UINT16) ((RxPtr[0] << 8) | RxPtr[1]);
  }
  RxPtr += sizeof (UINT16);

  Status = EFI_SUCCESS;

RecycleDesc:
  Dev->RdhLastSeen = (Dev->RdhLastSeen + 1) % E1K_NET_MAX_PENDING;
  E1kNetRegWrite32(Dev, E1K_REG_RDT, Dev->RdhLastSeen);

Exit:
  gBS->RestoreTPL (OldTpl);
  return Status;
}
