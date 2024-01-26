/** @file

  Implements
  - the SNM.WaitForPacket EVT_NOTIFY_WAIT event,
  - the EVT_SIGNAL_EXIT_BOOT_SERVICES event
  for the e1000 driver.

  Copyright (c) 2021, Oracle and/or its affiliates.
  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "E1kNet.h"

/**
  Invoke a notification event

  @param  Event                 Event whose notification function is being
                                invoked.
  @param  Context               The pointer to the notification function's
                                context, which is implementation-dependent.

**/

VOID
EFIAPI
E1kNetIsPacketAvailable (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  )
{
  //
  // This callback has been enqueued by an external application and is
  // running at TPL_CALLBACK already.
  //
  // The WaitForPacket logic is similar to that of WaitForKey. The former has
  // almost no documentation in either the UEFI-2.3.1+errC spec or the
  // DWG-2.3.1, but WaitForKey does have some.
  //
  E1K_NET_DEV *Dev;
  UINT32      RdhCur;

  Dev = Context;
  if (Dev->Snm.State != EfiSimpleNetworkInitialized) {
    return;
  }

  E1kNetRegRead32(Dev, E1K_REG_RDH, &RdhCur);

  if (Dev->RdhLastSeen != RdhCur) {
    gBS->SignalEvent (Dev->Snp.WaitForPacket);
  }
}

VOID
EFIAPI
E1kNetExitBoot (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  )
{
  //
  // This callback has been enqueued by ExitBootServices() and is running at
  // TPL_CALLBACK already.
  //
  // Shut down pending transfers according to DWG-2.3.1, "25.5.1 Exit Boot
  // Services Event".
  //
  E1K_NET_DEV *Dev;

  DEBUG ((DEBUG_VERBOSE, "%a: Context=0x%p\n", __FUNCTION__, Context));
  Dev = Context;
  if (Dev->Snm.State == EfiSimpleNetworkInitialized) {
    E1kNetDevReset (Dev);
  }
}
