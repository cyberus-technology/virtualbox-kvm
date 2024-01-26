/** @file

  Implementation of the SNP.Shutdown() function and its private helpers if any.

  Copyright (c) 2021, Oracle and/or its affiliates.
  Copyright (c) 2017, AMD Inc, All rights reserved.
  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiBootServicesTableLib.h>

#include "E1kNet.h"

/**
  Resets a network adapter and leaves it in a state that is safe for  another
  driver to initialize.

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           The network interface was shutdown.
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
E1kNetShutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  )
{
  E1K_NET_DEV *Dev;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  DEBUG((DEBUG_INFO, "E1kNetShutdown:\n"));

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

  E1kNetDevReset(Dev);
  E1kNetShutdownRx (Dev);
  E1kNetShutdownTx (Dev);

  Dev->Snm.State = EfiSimpleNetworkStarted;
  Status = EFI_SUCCESS;

Exit:
  gBS->RestoreTPL (OldTpl);
  return Status;
}
