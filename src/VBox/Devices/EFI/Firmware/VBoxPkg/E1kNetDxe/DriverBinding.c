/** @file

  Driver Binding code and its private helpers for the virtio-net driver.

  Copyright (c) 2021, Oracle and/or its affiliates.
  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Pci.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "E1kNet.h"

#define RECEIVE_FILTERS_NO_MCAST ((UINT32) (       \
          EFI_SIMPLE_NETWORK_RECEIVE_UNICAST     | \
          EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST   | \
          EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS   \
          ))

STATIC
EFI_STATUS
E1kNetEepromRead (
  IN  E1K_NET_DEV       *Dev,
  IN  UINT8             Offset,
  OUT UINT16            *Data
  )
{
  EFI_STATUS Status;
  UINT32     RegEerd = 0;

  Status = E1kNetRegWrite32(Dev, E1K_REG_EERD, ((UINT32)Offset << 8) | E1K_REG_EERD_START);
  if (EFI_ERROR (Status))
    return Status;

  // Wait for the read to complete
  while (   !EFI_ERROR (Status)
         && !(RegEerd & E1K_REG_EERD_DONE)) {
    gBS->Stall(1);
    Status = E1kNetRegRead32(Dev, E1K_REG_EERD, &RegEerd);
  }

  if (!EFI_ERROR(Status))
    *Data = E1K_REG_EERD_DATA_GET(RegEerd);

  return Status;
}

STATIC
EFI_STATUS
E1kNetMacAddrRead (
  IN E1K_NET_DEV        *Dev
  )
{
  EFI_STATUS Status;
  UINT8      i;

  for (i = 0; i < 3; i++)
  {
    UINT16 MacAddr;
    Status = E1kNetEepromRead (Dev, i, &MacAddr);
    if (EFI_ERROR (Status))
      return Status;

    Dev->Snm.CurrentAddress.Addr[i * 2]     = MacAddr & 0xff;
    Dev->Snm.CurrentAddress.Addr[i * 2 + 1] = (MacAddr >> 8) & 0xff;
  }

  return Status;
}

/**
  Set up the Simple Network Protocol fields, the Simple Network Mode fields,
  and the Exit Boot Services Event of the virtio-net driver instance.

  This function may only be called by E1kNetDriverBindingStart().

  @param[in,out] Dev  The E1K_NET_DEV driver instance being created for the
                      e1000 device.

  @return              Status codes from the CreateEvent().
  @retval EFI_SUCCESS  Configuration successful.
*/
STATIC
EFI_STATUS
EFIAPI
E1kNetSnpPopulate (
  IN OUT E1K_NET_DEV *Dev
  )
{
  UINT32 RegSts;
  EFI_STATUS Status;

  //
  // We set up a function here that is asynchronously callable by an
  // external application to check if there are any packets available for
  // reception. The least urgent task priority level we can specify for such a
  // "software interrupt" is TPL_CALLBACK.
  //
  // TPL_CALLBACK is also the maximum TPL an SNP implementation is allowed to
  // run at (see 6.1 Event, Timer, and Task Priority Services in the UEFI
  // Specification 2.3.1+errC).
  //
  // Since we raise our TPL to TPL_CALLBACK in every single function that
  // accesses the device, and the external application also queues its interest
  // for received packets at the same TPL_CALLBACK, in effect the
  // E1kNetIsPacketAvailable() function will never interrupt any
  // device-accessing driver function, it will be scheduled in isolation.
  //
  // TPL_CALLBACK (which basically this entire driver runs at) is allowed
  // for "[l]ong term operations (such as file system operations and disk
  // I/O)". Because none of our functions block, we'd satisfy an even stronger
  // requirement.
  //
  Status = gBS->CreateEvent (EVT_NOTIFY_WAIT, TPL_CALLBACK,
                  &E1kNetIsPacketAvailable, Dev, &Dev->Snp.WaitForPacket);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Dev->Snp.Revision       = EFI_SIMPLE_NETWORK_PROTOCOL_REVISION;
  Dev->Snp.Start          = &E1kNetStart;
  Dev->Snp.Stop           = &E1kNetStop;
  Dev->Snp.Initialize     = &E1kNetInitialize;
  Dev->Snp.Reset          = &E1kNetReset;
  Dev->Snp.Shutdown       = &E1kNetShutdown;
  Dev->Snp.ReceiveFilters = &E1kNetReceiveFilters;
  Dev->Snp.StationAddress = &E1kNetStationAddress;
  Dev->Snp.Statistics     = &E1kNetStatistics;
  Dev->Snp.MCastIpToMac   = &E1kNetMcastIpToMac;
  Dev->Snp.NvData         = &E1kNetNvData;
  Dev->Snp.GetStatus      = &E1kNetGetStatus;
  Dev->Snp.Transmit       = &E1kNetTransmit;
  Dev->Snp.Receive        = &E1kNetReceive;
  Dev->Snp.Mode           = &Dev->Snm;

  Dev->Snm.State                 = EfiSimpleNetworkStopped;
  Dev->Snm.HwAddressSize         = sizeof (E1K_NET_MAC);
  Dev->Snm.MediaHeaderSize       = sizeof (E1K_NET_MAC) + // dst MAC
                                   sizeof (E1K_NET_MAC) + // src MAC
                                   2;                     // Ethertype
  Dev->Snm.MaxPacketSize         = 1500;
  Dev->Snm.NvRamSize             = 0;
  Dev->Snm.NvRamAccessSize       = 0;
  Dev->Snm.ReceiveFilterMask     = RECEIVE_FILTERS_NO_MCAST;
  Dev->Snm.ReceiveFilterSetting  = RECEIVE_FILTERS_NO_MCAST;
  Dev->Snm.MaxMCastFilterCount   = 0;
  Dev->Snm.MCastFilterCount      = 0;
  Dev->Snm.IfType                = 1; // ethernet
  Dev->Snm.MacAddressChangeable  = FALSE;
  Dev->Snm.MultipleTxSupported   = TRUE;

  ASSERT (sizeof (E1K_NET_MAC) <= sizeof (EFI_MAC_ADDRESS));

  Dev->Snm.MediaPresentSupported = TRUE;
  Status = E1kNetRegRead32(Dev, E1K_REG_STATUS, &RegSts);
  if (EFI_ERROR (Status)) {
    goto CloseWaitForPacket;
  }

  Dev->Snm.MediaPresent = (BOOLEAN)((RegSts & E1K_REG_STATUS_LU) != 0);

  Status = E1kNetMacAddrRead(Dev);
  CopyMem (&Dev->Snm.PermanentAddress, &Dev->Snm.CurrentAddress,
    sizeof (E1K_NET_MAC));
  SetMem (&Dev->Snm.BroadcastAddress, sizeof (E1K_NET_MAC), 0xFF);

  //
  // E1kNetExitBoot() is queued by ExitBootServices(); its purpose is to
  // cancel any pending requests. The TPL_CALLBACK reasoning is
  // identical to the one above. There's one difference: this kind of
  // event is "globally visible", which means it can be signalled as soon as
  // we create it. We haven't raised our TPL here, hence E1kNetExitBoot()
  // could be entered immediately. E1kNetExitBoot() checks Dev->Snm.State,
  // so we're safe.
  //
  Status = gBS->CreateEvent (EVT_SIGNAL_EXIT_BOOT_SERVICES, TPL_CALLBACK,
                             &E1kNetExitBoot, Dev, &Dev->ExitBoot);
  if (EFI_ERROR (Status)) {
    goto CloseWaitForPacket;
  }

  return EFI_SUCCESS;

CloseWaitForPacket:
  gBS->CloseEvent (Dev->Snp.WaitForPacket);
  return Status;
}


/**
  Release any resources allocated by E1kNetSnpPopulate().

  This function may only be called by E1kNetDriverBindingStart(), when
  rolling back a partial, failed driver instance creation, and by
  E1kNetDriverBindingStop(), when disconnecting a virtio-net device from the
  driver.

  @param[in,out] Dev  The E1K_NET_DEV driver instance being destroyed.
*/
STATIC
VOID
EFIAPI
E1kNetSnpEvacuate (
  IN OUT E1K_NET_DEV *Dev
  )
{
  //
  // This function runs either at TPL_CALLBACK already (from
  // E1kNetDriverBindingStop()), or it is part of a teardown following
  // a partial, failed construction in E1kNetDriverBindingStart(), when
  // WaitForPacket was never accessible to the world.
  //
  gBS->CloseEvent (Dev->ExitBoot);
  gBS->CloseEvent (Dev->Snp.WaitForPacket);
}


/**
  Tests to see if this driver supports a given controller. If a child device is
  provided, it further tests to see if this driver supports creating a handle
  for the specified child device.

  This function checks to see if the driver specified by This supports the
  device specified by ControllerHandle. Drivers will typically use the device
  path attached to ControllerHandle and/or the services from the bus I/O
  abstraction attached to ControllerHandle to determine if the driver supports
  ControllerHandle. This function may be called many times during platform
  initialization. In order to reduce boot times, the tests performed by this
  function must be very small, and take as little time as possible to execute.
  This function must not change the state of any hardware devices, and this
  function must be aware that the device specified by ControllerHandle may
  already be managed by the same driver or a different driver. This function
  must match its calls to AllocatePages() with FreePages(), AllocatePool() with
  FreePool(), and OpenProtocol() with CloseProtocol(). Because ControllerHandle
  may have been previously started by the same driver, if a protocol is already
  in the opened state, then it must not be closed with CloseProtocol(). This is
  required to guarantee the state of ControllerHandle is not modified by this
  function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                   instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This
                                   handle must support a protocol interface
                                   that supplies an I/O abstraction to the
                                   driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a
                                   device path.  This parameter is ignored by
                                   device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter
                                   is not NULL, then the bus driver must
                                   determine if the bus controller specified by
                                   ControllerHandle and the child controller
                                   specified by RemainingDevicePath are both
                                   supported by this bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the
                                   driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed
                                   by the driver specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed
                                   by a different driver or an application that
                                   requires exclusive access. Currently not
                                   implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the
                                   driver specified by This.
**/

STATIC
EFI_STATUS
EFIAPI
E1kNetDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL            *This,
  IN EFI_HANDLE                             ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL               *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE00          Pci;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (Pci) / sizeof (UINT32),
                        &Pci
                        );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  if (Pci.Hdr.VendorId == INTEL_PCI_VENDOR_ID &&
      (Pci.Hdr.DeviceId == INTEL_82540EM_PCI_DEVICE_ID ||
       Pci.Hdr.DeviceId == INTEL_82543GC_PCI_DEVICE_ID ||
       Pci.Hdr.DeviceId == INTEL_82545EM_PCI_DEVICE_ID)) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_UNSUPPORTED;
  }

Done:
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );
  return Status;
}


/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service
  ConnectController(). As a result, much of the error checking on the
  parameters to Start() has been moved into this  common boot service. It is
  legal to call Start() from other locations,  but the following calling
  restrictions must be followed, or the system behavior will not be
  deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a
     naturally aligned EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver
     specified by This must have been called with the same calling parameters,
     and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                   instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This
                                   handle  must support a protocol interface
                                   that supplies  an I/O abstraction to the
                                   driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a
                                   device path.  This  parameter is ignored by
                                   device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter
                                   is NULL, then handles  for all the children
                                   of Controller are created by this driver.
                                   If this parameter is not NULL and the first
                                   Device Path Node is  not the End of Device
                                   Path Node, then only the handle for the
                                   child device specified by the first Device
                                   Path Node of  RemainingDevicePath is created
                                   by this driver. If the first Device Path
                                   Node of RemainingDevicePath is  the End of
                                   Device Path Node, no child handle is created
                                   by this driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a
                                   device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a
                                   lack of resources.
  @retval Others                   The driver failed to start the device.

**/
STATIC
EFI_STATUS
EFIAPI
E1kNetDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL            *This,
  IN EFI_HANDLE                             ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL               *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS               Status;
  E1K_NET_DEV              *Dev;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  MAC_ADDR_DEVICE_PATH     MacNode;

  DEBUG((DEBUG_INFO, "E1kNetControllerStart:\n"));

  Dev = AllocateZeroPool (sizeof (*Dev));
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Dev->Signature = E1K_NET_DEV_SIGNATURE;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&Dev->PciIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FreePool;
  }

  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationGet,
                         0,
                         &Dev->OriginalPciAttributes
                         );
  if (EFI_ERROR (Status)) {
    goto CloseProtocol;
  }

  //
  // Enable I/O Space & Bus-Mastering
  //
  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationEnable,
                         (EFI_PCI_IO_ATTRIBUTE_IO |
                          EFI_PCI_IO_ATTRIBUTE_BUS_MASTER),
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    goto CloseProtocol;
  }

  //
  // Signal device supports 64-bit DMA addresses
  //
  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationEnable,
                         EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE,
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    //
    // Warn user that device will only be using 32-bit DMA addresses.
    //
    // Note that this does not prevent the device/driver from working
    // and therefore we only warn and continue as usual.
    //
    DEBUG ((
      DEBUG_WARN,
      "%a: failed to enable 64-bit DMA addresses\n",
      __FUNCTION__
      ));
  }

  DEBUG((DEBUG_INFO, "E1kNetControllerStart: Resetting NIC\n"));
  Status = E1kNetDevReset (Dev);
  if (EFI_ERROR (Status)) {
    goto RestoreAttributes;
  }

  //
  // now we can run a basic one-shot e1000 initialization required to
  // retrieve the MAC address
  //
  DEBUG((DEBUG_INFO, "E1kNetControllerStart: Populating SNP interface\n"));
  Status = E1kNetSnpPopulate (Dev);
  if (EFI_ERROR (Status)) {
    goto UninitDev;
  }

  //
  // get the device path of the e1000 device -- one-shot open
  //
  Status = gBS->OpenProtocol (ControllerHandle, &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath, This->DriverBindingHandle,
                  ControllerHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (EFI_ERROR (Status)) {
    goto Evacuate;
  }

  //
  // create another device path that has the MAC address appended
  //
  MacNode.Header.Type    = MESSAGING_DEVICE_PATH;
  MacNode.Header.SubType = MSG_MAC_ADDR_DP;
  SetDevicePathNodeLength (&MacNode, sizeof MacNode);
  CopyMem (&MacNode.MacAddress, &Dev->Snm.CurrentAddress,
    sizeof (EFI_MAC_ADDRESS));
  MacNode.IfType         = Dev->Snm.IfType;

  Dev->MacDevicePath = AppendDevicePathNode (DevicePath, &MacNode.Header);
  if (Dev->MacDevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Evacuate;
  }

  //
  // create a child handle with the Simple Network Protocol and the new
  // device path installed on it
  //
  Status = gBS->InstallMultipleProtocolInterfaces (&Dev->MacHandle,
                  &gEfiSimpleNetworkProtocolGuid, &Dev->Snp,
                  &gEfiDevicePathProtocolGuid,    Dev->MacDevicePath,
                  NULL);
  if (EFI_ERROR (Status)) {
    goto FreeMacDevicePath;
  }

  DEBUG((DEBUG_INFO, "E1kNetControllerStart: returns EFI_SUCCESS\n"));
  return EFI_SUCCESS;

FreeMacDevicePath:
  FreePool (Dev->MacDevicePath);

Evacuate:
  E1kNetSnpEvacuate (Dev);

UninitDev:
  E1kNetDevReset (Dev);

RestoreAttributes:
  Dev->PciIo->Attributes (
                Dev->PciIo,
                EfiPciIoAttributeOperationSet,
                Dev->OriginalPciAttributes,
                NULL
                );

CloseProtocol:
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

FreePool:
  FreePool (Dev);

  DEBUG((DEBUG_INFO, "E1kNetControllerStart: returns %u\n", Status));
  return Status;
}

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service
  DisconnectController().  As a result, much of the error checking on the
  parameters to Stop() has been moved  into this common boot service. It is
  legal to call Stop() from other locations,  but the following calling
  restrictions must be followed, or the system behavior will not be
  deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous
     call to this same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a
     valid EFI_HANDLE. In addition, all of these handles must have been created
     in this driver's Start() function, and the Start() function must have
     called OpenProtocol() on ControllerHandle with an Attribute of
     EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The
                                handle must  support a bus specific I/O
                                protocol for the driver  to use to stop the
                                device.
  @param[in]  NumberOfChildren  The number of child device handles in
                                ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be
                                NULL  if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device
                                error.

**/
STATIC
EFI_STATUS
EFIAPI
E1kNetDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  )
{
  if (NumberOfChildren > 0) {
    //
    // free all resources for whose access we need the child handle, because
    // the child handle is going away
    //
    EFI_STATUS                  Status;
    EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
    E1K_NET_DEV                 *Dev;
    EFI_TPL                     OldTpl;

    ASSERT (NumberOfChildren == 1);

    Status = gBS->OpenProtocol (ChildHandleBuffer[0],
                    &gEfiSimpleNetworkProtocolGuid, (VOID **)&Snp,
                    This->DriverBindingHandle, ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    ASSERT_EFI_ERROR (Status);
    Dev = E1K_NET_FROM_SNP (Snp);

    //
    // prevent any interference with WaitForPacket
    //
    OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

    ASSERT (Dev->MacHandle == ChildHandleBuffer[0]);
    if (Dev->Snm.State != EfiSimpleNetworkStopped) {
      //
      // device in use, cannot stop driver instance
      //
      Status = EFI_DEVICE_ERROR;
    }
    else {
      gBS->UninstallMultipleProtocolInterfaces (Dev->MacHandle,
             &gEfiDevicePathProtocolGuid,    Dev->MacDevicePath,
             &gEfiSimpleNetworkProtocolGuid, &Dev->Snp,
             NULL);
      FreePool (Dev->MacDevicePath);
      E1kNetSnpEvacuate (Dev);

      Dev->PciIo->Attributes (
                    Dev->PciIo,
                    EfiPciIoAttributeOperationSet,
                    Dev->OriginalPciAttributes,
                    NULL
                    );

      gBS->CloseProtocol (
             ControllerHandle,
             &gEfiPciIoProtocolGuid,
             This->DriverBindingHandle,
             ControllerHandle
             );

      FreePool (Dev);
    }

    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  return EFI_SUCCESS;
}


EFI_DRIVER_BINDING_PROTOCOL gE1kNetDriverBinding = {
  &E1kNetDriverBindingSupported,
  &E1kNetDriverBindingStart,
  &E1kNetDriverBindingStop,
  0x10,
  NULL,
  NULL
};
