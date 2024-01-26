/** @file

  Implementation of the SNP.GetStatus() function and its private helpers if
  any.

  Copyright (c) 2021, Oracle and/or its affiliates.
  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "E1kNet.h"

/**
  Reads the current interrupt status and recycled transmit buffer status from
  a network interface.

  @param  This            The protocol instance pointer.
  @param  InterruptStatus A pointer to the bit mask of the currently active
                          interrupts If this is NULL, the interrupt status will
                          not be read from the device. If this is not NULL, the
                          interrupt status will be read from the device. When
                          the  interrupt status is read, it will also be
                          cleared. Clearing the transmit  interrupt does not
                          empty the recycled transmit buffer array.
  @param  TxBuf           Recycled transmit buffer address. The network
                          interface will not transmit if its internal recycled
                          transmit buffer array is full. Reading the transmit
                          buffer does not clear the transmit interrupt. If this
                          is NULL, then the transmit buffer status will not be
                          read. If there are no transmit buffers to recycle and
                          TxBuf is not NULL, * TxBuf will be set to NULL.

  @retval EFI_SUCCESS           The status of the network interface was
                                retrieved.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an
                                unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network
                                interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network
                                interface.

**/

EFI_STATUS
EFIAPI
E1kNetGetStatus (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  OUT UINT32                     *InterruptStatus OPTIONAL,
  OUT VOID                       **TxBuf OPTIONAL
  )
{
  E1K_NET_DEV          *Dev;
  EFI_TPL              OldTpl;
  EFI_STATUS           Status;
  UINT32               TdhCur;
  UINT32               RdhCur;
  EFI_PHYSICAL_ADDRESS DeviceAddress;

  if (This == NULL) {
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

  //
  // update link status
  //
  if (Dev->Snm.MediaPresentSupported) {
    UINT32 RegSts;

    Status = E1kNetRegRead32(Dev, E1K_REG_STATUS, &RegSts);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Dev->Snm.MediaPresent = (BOOLEAN)((RegSts & E1K_REG_STATUS_LU) != 0);
  }

  E1kNetRegRead32(Dev, E1K_REG_TDH, &TdhCur);
  E1kNetRegRead32(Dev, E1K_REG_RDH, &RdhCur);


  if (InterruptStatus != NULL) {
    //
    // report the receive interrupt if there is data available for reception,
    // report the transmit interrupt if we have transmitted at least one buffer
    //
    *InterruptStatus = 0;
    if (Dev->RdhLastSeen != RdhCur) {
      *InterruptStatus |= EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT;
    }
    if (Dev->TdhLastSeen != TdhCur) {
      ASSERT (Dev->TxCurPending > 0);
      *InterruptStatus |= EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT;
    }
  }

  if (TxBuf != NULL) {
    if (Dev->TdhLastSeen == TdhCur) {
      *TxBuf = NULL;
    }
    else {
      ASSERT (Dev->TxCurPending > 0);
      ASSERT (Dev->TxCurPending <= Dev->TxMaxPending);

      //
      // get the device address that has been enqueued for the caller's
      // transmit buffer
      //
      DeviceAddress = Dev->TxRing[Dev->TdhLastSeen].AddrBufferLow;
      DeviceAddress |= LShiftU64(Dev->TxRing[Dev->TdhLastSeen].AddrBufferHigh, 32);

      Dev->TdhLastSeen = (Dev->TdhLastSeen + 1) % E1K_NET_MAX_PENDING;
      Dev->TxCurPending--;

      //
      // Unmap the device address and perform the reverse mapping to find the
      // caller buffer address.
      //
      Status = E1kNetUnmapTxBuf (
                 Dev,
                 TxBuf,
                 DeviceAddress
                 );
      if (EFI_ERROR (Status)) {
        //
        // E1kNetUnmapTxBuf should never fail, if we have reached here
        // that means our internal state has been corrupted
        //
        ASSERT (FALSE);
        Status = EFI_DEVICE_ERROR;
        goto Exit;
      }
    }
  }

  Status = EFI_SUCCESS;

Exit:
  gBS->RestoreTPL (OldTpl);
  return Status;
}
