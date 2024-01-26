/** @file

  This file implements the hardware register access functions of the e1000 driver.

  Copyright (c) 2021, Oracle and/or its affiliates.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Pci.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "E1kNet.h"

EFI_STATUS
EFIAPI
E1kNetRegWrite32 (
  IN E1K_NET_DEV        *Dev,
  IN UINT32             Addr,
  IN UINT32             Data
  )
{
  EFI_STATUS Status;

  Status = Dev->PciIo->Io.Write (
                          Dev->PciIo,
                          EfiPciIoWidthUint32,
                          PCI_BAR_IDX2,
                          0, // IOADDR
                          1,
                          &Addr
                          );
  if (!EFI_ERROR (Status))
  {
    Status = Dev->PciIo->Io.Write (
                            Dev->PciIo,
                            EfiPciIoWidthUint32,
                            PCI_BAR_IDX2,
                            4, // IODATA
                            1,
                            &Data
                            );
  }

  return Status;
}

EFI_STATUS
EFIAPI
E1kNetRegRead32 (
  IN  E1K_NET_DEV        *Dev,
  IN  UINT32             Addr,
  OUT UINT32             *Data
  )
{
  EFI_STATUS Status;

  Status = Dev->PciIo->Io.Write (
                          Dev->PciIo,
                          EfiPciIoWidthUint32,
                          PCI_BAR_IDX2,
                          0, // IOADDR
                          1,
                          &Addr
                          );
  if (!EFI_ERROR (Status))
  {
    return Dev->PciIo->Io.Read (
                            Dev->PciIo,
                            EfiPciIoWidthUint32,
                            PCI_BAR_IDX2,
                            4, // IODATA
                            1,
                            Data
                            );
  }

  return Status;
}

EFI_STATUS
EFIAPI
E1kNetRegSet32 (
  IN E1K_NET_DEV        *Dev,
  IN UINT32             Addr,
  IN UINT32             Set)
{
  UINT32 Reg;
  EFI_STATUS Status;

  Status = E1kNetRegRead32 (Dev, Addr, &Reg);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Reg |= Set;
  Status = E1kNetRegWrite32 (Dev, Addr, Reg);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
E1kNetRegClear32 (
  IN E1K_NET_DEV        *Dev,
  IN UINT32             Addr,
  IN UINT32             Clear)
{
  UINT32 Reg;
  EFI_STATUS Status;

  Status = E1kNetRegRead32 (Dev, Addr, &Reg);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Reg &= ~Clear;
  Status = E1kNetRegWrite32 (Dev, Addr, Reg);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
E1kNetDevReset (
  IN E1K_NET_DEV        *Dev
  )
{
  EFI_STATUS Status;

  //
  // Reset hardware
  //
  Status = E1kNetRegSet32 (Dev, E1K_REG_CTRL, E1K_REG_CTRL_RST);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Wait for the reset to complete
  //
  for (;;)
  {
    UINT32 Ctrl;

    Status = E1kNetRegRead32 (Dev, E1K_REG_CTRL, &Ctrl);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    /// @todo Timeout?
    if (!(Ctrl & E1K_REG_CTRL_RST))
      break;
  }

  //
  // Reset the PHY.
  //
  Status = E1kNetRegSet32 (Dev, E1K_REG_CTRL, E1K_REG_CTRL_PHY_RST);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Wait for the specified amount of 3us and de-assert the PHY reset signal.
  //
  gBS->Stall(3);
  Status = E1kNetRegClear32 (Dev, E1K_REG_CTRL, E1K_REG_CTRL_PHY_RST);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
