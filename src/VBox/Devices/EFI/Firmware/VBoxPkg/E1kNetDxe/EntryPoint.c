/** @file

  This file implements the entry point of the e1000 driver.

  Copyright (c) 2021, Oracle and/or its affiliates.
  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiLib.h>

#include "E1kNet.h"

/**
  This is the declaration of an EFI image entry point. This entry point is the
  same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including both
  device drivers and bus drivers.

  @param  ImageHandle           The firmware allocated handle for the UEFI
                                image.
  @param  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                An unexpected error occurred.
**/

EFI_STATUS
EFIAPI
E1kNetEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gE1kNetDriverBinding,
           ImageHandle,
           &gE1kNetComponentName,
           &gE1kNetComponentName2
           );
}
