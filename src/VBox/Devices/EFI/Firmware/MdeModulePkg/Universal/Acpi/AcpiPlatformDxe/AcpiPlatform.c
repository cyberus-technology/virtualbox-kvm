/** @file
  Sample ACPI Platform Driver

  Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/FirmwareVolume2.h>

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/Acpi.h>

/**
  Locate the first instance of a protocol.  If the protocol requested is an
  FV protocol, then it will return the first FV that contains the ACPI table
  storage file.

  @param  Instance      Return pointer to the first instance of the protocol

  @return EFI_SUCCESS           The function completed successfully.
  @return EFI_NOT_FOUND         The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateFvInstanceWithTables (
  OUT EFI_FIRMWARE_VOLUME2_PROTOCOL **Instance
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         NumberOfHandles;
  EFI_FV_FILETYPE               FileType;
  UINT32                        FvStatus;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINTN                         Size;
  UINTN                         Index;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *FvInstance;

  FvStatus = 0;

  //
  // Locate protocol.
  //
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiFirmwareVolume2ProtocolGuid,
                   NULL,
                   &NumberOfHandles,
                   &HandleBuffer
                   );
  if (EFI_ERROR (Status)) {
    //
    // Defined errors at this time are not found and out of resources.
    //
    return Status;
  }



  //
  // Looking for FV with ACPI storage file
  //

  for (Index = 0; Index < NumberOfHandles; Index++) {
    //
    // Get the protocol on this handle
    // This should not fail because of LocateHandleBuffer
    //
    Status = gBS->HandleProtocol (
                     HandleBuffer[Index],
                     &gEfiFirmwareVolume2ProtocolGuid,
                     (VOID**) &FvInstance
                     );
    ASSERT_EFI_ERROR (Status);

    //
    // See if it has the ACPI storage file
    //
    Status = FvInstance->ReadFile (
                           FvInstance,
                           (EFI_GUID*)PcdGetPtr (PcdAcpiTableStorageFile),
                           NULL,
                           &Size,
                           &FileType,
                           &Attributes,
                           &FvStatus
                           );

    //
    // If we found it, then we are done
    //
    if (Status == EFI_SUCCESS) {
      *Instance = FvInstance;
      break;
    }
  }

  //
  // Our exit status is determined by the success of the previous operations
  // If the protocol was found, Instance already points to it.
  //

  //
  // Free any allocated buffers
  //
  gBS->FreePool (HandleBuffer);

  return Status;
}


/**
  This function calculates and updates an UINT8 checksum.

  @param  Buffer          Pointer to buffer to checksum
  @param  Size            Number of bytes to checksum

**/
VOID
AcpiPlatformChecksum (
  IN UINT8      *Buffer,
  IN UINTN      Size
  )
{
  UINTN ChecksumOffset;

  ChecksumOffset = OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, Checksum);

  //
  // Set checksum to 0 first
  //
  Buffer[ChecksumOffset] = 0;

  //
  // Update checksum value
  //
  Buffer[ChecksumOffset] = CalculateCheckSum8(Buffer, Size);
}


#ifdef VBOX

/* Disables the old code only copying a selection of tables, missing out a bunch of things available in the XSDT/RSDT. */
# define ACPI_NO_STATIC_TABLES_SELECTION 1

# define ACPI_RSD_PTR       SIGNATURE_64('R', 'S', 'D', ' ', 'P', 'T', 'R', ' ')
# define EBDA_BASE          (0x9FC0 << 4)

VOID *
FindAcpiRsdPtr(VOID)
{
  UINTN Address;
  UINTN Index;

  //
  // First Search 0x0e0000 - 0x0fffff for RSD Ptr
  //
  for (Address = 0xe0000; Address < 0xfffff; Address += 0x10) {
    if (*(UINT64 *)(Address) == ACPI_RSD_PTR) {
      return (VOID *)Address;
    }
  }

  //
  // Search EBDA
  //
  Address = EBDA_BASE;
  for (Index = 0; Index < 0x400 ; Index += 16) {
    if (*(UINT64 *)(Address + Index) == ACPI_RSD_PTR) {
      return (VOID *)Address;
    }
  }
  return NULL;
}

#ifndef ACPI_NO_STATIC_TABLES_SELECTION
VOID *FindSignature(VOID* Start, UINT32 Signature, BOOLEAN NoChecksum)
{
  UINT8 *Ptr = (UINT8*)Start;
  UINT32 Count = 0x10000; // 16 pages

  while (Count-- > 0) {
    if (   *(UINT32*)Ptr == Signature
        && ((EFI_ACPI_DESCRIPTION_HEADER *)Ptr)->Length <= Count
        && (NoChecksum ||
            CalculateCheckSum8(Ptr, ((EFI_ACPI_DESCRIPTION_HEADER *)Ptr)->Length) == 0
            )) {
      return Ptr;
    }

    Ptr++;
  }
  return NULL;
}
#endif

VOID
FillSysTablesInfo(VOID **Tables, UINT32 TablesSize)
{
    UINT32 Table = 0;
    EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *RsdPtr;
#ifndef ACPI_NO_STATIC_TABLES_SELECTION
    VOID *TablesPage;
#else
    EFI_ACPI_DESCRIPTION_HEADER *RsdtTbl;
#endif
    UINT64 *PtrTbl;
    UINT32 Index;

    RsdPtr = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER*)FindAcpiRsdPtr();
    ASSERT(RsdPtr != NULL);

#ifndef ACPI_NO_STATIC_TABLES_SELECTION
#define FLAG_OPTIONAL    1<<0
#define FLAG_NO_CHECKSUM 1<<1
    static struct {
        UINT32 Signature;
        UINT32 Flags;
        CHAR8* Name;
    } TableInfo[] = {
        // MADT, optional
        { SIGNATURE_32('A', 'P', 'I', 'C'), FLAG_OPTIONAL, "MADT"},
        // FACP (also called FADT)
        { SIGNATURE_32('F', 'A', 'C', 'P'), 0, "FADT"},
        // FACS, according 5.2.9 of ACPI v2. spec FACS doesn't have checksum field
        { SIGNATURE_32('F', 'A', 'C', 'S'), FLAG_NO_CHECKSUM, "FACS"},
        // DSDT
        { SIGNATURE_32('D', 'S', 'D', 'T'), 0, "DSDT"},
        // SSDT
        { SIGNATURE_32('S', 'S', 'D', 'T'), FLAG_OPTIONAL, "SSDT"},
        // HPET
        { SIGNATURE_32('H', 'P', 'E', 'T'), FLAG_OPTIONAL, "HPET"},
        // MCFG
        { SIGNATURE_32('M', 'C', 'F', 'G'), FLAG_OPTIONAL, "MCFG"}
    };

    TablesPage = (VOID *)(UINTN)((RsdPtr->RsdtAddress) & ~0xfff);
    DEBUG((DEBUG_INFO, "TablesPage:%p\n", TablesPage));

    for (Index = 0; Index < sizeof TableInfo / sizeof TableInfo[0]; Index++)
    {
        VOID *Ptr = FindSignature(TablesPage, TableInfo[Index].Signature,
                                  (BOOLEAN)((TableInfo[Index].Flags & FLAG_NO_CHECKSUM) != 0));
        if (TableInfo[Index].Signature == SIGNATURE_32('F', 'A', 'C', 'P'))
        {
             // we actually have 2 FADTs, see https://xtracker.innotek.de/index.php?bug=4082
            Ptr = FindSignature((UINT8*)Ptr+32, SIGNATURE_32('F', 'A', 'C', 'P'), FALSE);
        }
        if (!(TableInfo[Index].Flags & FLAG_OPTIONAL))
        {
             if (!Ptr)
               DEBUG((EFI_D_ERROR, "%a: isn't optional %p\n", TableInfo[Index].Name, Ptr));
             ASSERT(Ptr != NULL);
        }
        DEBUG((EFI_D_ERROR, "%a: %p\n", TableInfo[Index].Name, Ptr));
        if (Ptr)
           Tables[Table++] = Ptr;
    }
#else
    RsdtTbl = (EFI_ACPI_DESCRIPTION_HEADER*)(UINTN)RsdPtr->XsdtAddress;
    DEBUG((DEBUG_INFO, "RsdtTbl:%p\n", RsdtTbl));

    PtrTbl = (UINT64 *)(RsdtTbl + 1);
    for (Index = 0; Index < (RsdtTbl->Length - sizeof(*RsdtTbl)) / sizeof(UINT64); Index++)
    {
        EFI_ACPI_DESCRIPTION_HEADER *Header = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)*PtrTbl++;
        DEBUG ((DEBUG_VERBOSE, "Table %p found \"%-4.4a\" size 0x%x\n", Header, (CONST CHAR8 *)&Header->Signature, Header->Length));

        if (Header->Signature == SIGNATURE_32('F', 'A', 'C', 'P'))
        {
            /* Add the DSDT pointer from there. */
            EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *Fadt = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *)Header;
            DEBUG((DEBUG_INFO, "Found FACP: DSDT 0x%x FACS 0x%x XDsdt %p XFacs %p\n", Fadt->Dsdt, Fadt->FirmwareCtrl, Fadt->XDsdt, Fadt->XFirmwareCtrl));
            Tables[Table++] = (VOID *)(UINTN)Fadt->FirmwareCtrl;
            Tables[Table++] = (VOID *)(UINTN)Fadt->Dsdt;
        }

        Tables[Table++] = Header;
    }
#endif

#if 0
    // RSDT
    ASSERT(Table < TablesSize);
    Tables[Table] = FindSignature(TablesPage, SIGNATURE_32('R', 'S', 'D', 'T'));
    DEBUG ((EFI_D_ERROR, "RSDT: %p\n", Tables[Table]));
    ASSERT(Tables[Table] != NULL);
    Table++;

    // XSDT
    ASSERT(Table < TablesSize);
    Tables[Table] = FindSignature(TablesPage, SIGNATURE_32('X', 'S', 'D', 'T'));
    DEBUG ((EFI_D_ERROR, "XSDT: %p\n", Tables[Table]));
    ASSERT(Tables[Table] != NULL);
    Table++;
#endif

    DEBUG((DEBUG_INFO, "We found %d tables (max allowed %d)\n", Table, TablesSize));
    Tables[Table] = NULL;
}

#endif /* VBOX */


/**
  Entrypoint of Acpi Platform driver.

  @param  ImageHandle
  @param  SystemTable

  @return EFI_SUCCESS
  @return EFI_LOAD_ERROR
  @return EFI_OUT_OF_RESOURCES

**/
EFI_STATUS
EFIAPI
AcpiPlatformEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_ACPI_TABLE_PROTOCOL        *AcpiTable;
#ifdef VBOX
# ifndef ACPI_NO_STATIC_TABLES_SELECTION
  VOID                           *VBoxTables[10];
# else
  VOID                           *VBoxTables[128];
# endif
#else
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FwVol;
#endif
  INTN                           Instance;
  EFI_ACPI_COMMON_HEADER         *CurrentTable;
  UINTN                          TableHandle;
#ifndef VBOX
  UINT32                         FvStatus;
#endif
  UINTN                          TableSize;
  UINTN                          Size;

  Instance     = 0;
  CurrentTable = NULL;
  TableHandle  = 0;

  //
  // Find the AcpiTable protocol
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID**)&AcpiTable);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

#ifdef VBOX
  //
  // VBOX already has tables prepared in memory - just reuse them.
  //
  FillSysTablesInfo(VBoxTables, sizeof(VBoxTables)/sizeof(VBoxTables[0]));
#else
  //
  //
  // Locate the firmware volume protocol
  //
  Status = LocateFvInstanceWithTables (&FwVol);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
#endif
  //
  // Read tables from the storage file.
  //
  while (Status == EFI_SUCCESS) {

#ifdef VBOX
    CurrentTable = (EFI_ACPI_COMMON_HEADER *)VBoxTables[Instance];
    Status = (CurrentTable == NULL) ? EFI_NOT_FOUND : EFI_SUCCESS;
    if (CurrentTable) {
      Size = CurrentTable->Length;
      DEBUG((EFI_D_ERROR, "adding %p %d\n", CurrentTable, Size));
    } else
      Size = 0; // Just to shut up the compiler.
#else
    Status = FwVol->ReadSection (
                      FwVol,
                      (EFI_GUID*)PcdGetPtr (PcdAcpiTableStorageFile),
                      EFI_SECTION_RAW,
                      Instance,
                      (VOID**) &CurrentTable,
                      &Size,
                      &FvStatus
                      );
#endif
    if (!EFI_ERROR(Status)) {
      //
      // Add the table
      //
      TableHandle = 0;

      TableSize = ((EFI_ACPI_DESCRIPTION_HEADER *) CurrentTable)->Length;
#ifdef VBOX
      DEBUG((DEBUG_INFO, "Size:%d, TableSize:%d\n", Size, TableSize));
#endif
      ASSERT (Size >= TableSize);

      //
      // Checksum ACPI table
      //
      AcpiPlatformChecksum ((UINT8*)CurrentTable, TableSize);

      //
      // Install ACPI table
      //
      Status = AcpiTable->InstallAcpiTable (
                            AcpiTable,
                            CurrentTable,
                            TableSize,
                            &TableHandle
                            );

#ifndef VBOX /* In case we're reading ACPI tables from memory we haven't
                allocated this memory, so it isn't required to free it */
      //
      // Free memory allocated by ReadSection
      //
      gBS->FreePool (CurrentTable);

      if (EFI_ERROR(Status)) {
        return EFI_ABORTED;
      }
#endif

      //
      // Increment the instance
      //
      Instance++;
      CurrentTable = NULL;
    }
  }

  //
  // The driver does not require to be kept loaded.
  //
  return EFI_REQUEST_UNLOAD_IMAGE;
}

