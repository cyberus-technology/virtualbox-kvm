
#ifndef VBOX_INCLUDED_SRC_Graphics_BIOS_vbe_h
#define VBOX_INCLUDED_SRC_Graphics_BIOS_vbe_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "vgabios.h"

#include <VBoxVideoVBE.h>
#include <VBoxVideoVBEPrivate.h>

// DISPI helper function
//void dispi_set_enable(Boolean enable);

/** VBE int10 API
 *
 *  See the function descriptions in vbe.c for more information
 */

/* Far pointer for VBE info block usage. */
typedef union vbe_ptr {
    uint32_t    Ptr32;
    void __far  *Ptr;
    union {
        uint16_t    Off;
        uint16_t    Seg;
    };
} vbe_ptr;

// The official VBE Information Block
typedef struct VbeInfoBlock
{
   union        {
       uint8_t      SigChr[4];
       uint32_t     Sig32;
   }            VbeSignature;
   uint16_t     VbeVersion;
   vbe_ptr      OemString;
   uint8_t      Capabilities[4];
   uint16_t     VideoModePtr_Off;
   uint16_t     VideoModePtr_Seg;
   uint16_t     TotalMemory;
   uint16_t     OemSoftwareRev;
   vbe_ptr      OemVendorName;
   vbe_ptr      OemProductName;
   vbe_ptr      OemProductRev;
   uint16_t     Reserved[111]; // used for dynamically generated mode list
   uint8_t      OemData[256];
} VbeInfoBlock;


typedef struct ModeInfoBlock
{
// Mandatory information for all VBE revisions
   uint16_t ModeAttributes;
   uint8_t  WinAAttributes;
   uint8_t  WinBAttributes;
   uint16_t WinGranularity;
   uint16_t WinSize;
   uint16_t WinASegment;
   uint16_t WinBSegment;
   uint32_t WinFuncPtr;
   uint16_t BytesPerScanLine;
// Mandatory information for VBE 1.2 and above
   uint16_t XResolution;
   uint16_t YResolution;
   uint8_t  XCharSize;
   uint8_t  YCharSize;
   uint8_t  NumberOfPlanes;
   uint8_t  BitsPerPixel;
   uint8_t  NumberOfBanks;
   uint8_t  MemoryModel;
   uint8_t  BankSize;
   uint8_t  NumberOfImagePages;
   uint8_t  Reserved_page;
// Direct Color fields (required for direct/6 and YUV/7 memory models)
   uint8_t  RedMaskSize;
   uint8_t  RedFieldPosition;
   uint8_t  GreenMaskSize;
   uint8_t  GreenFieldPosition;
   uint8_t  BlueMaskSize;
   uint8_t  BlueFieldPosition;
   uint8_t  RsvdMaskSize;
   uint8_t  RsvdFieldPosition;
   uint8_t  DirectColorModeInfo;
// Mandatory information for VBE 2.0 and above
   uint32_t PhysBasePtr;
   uint32_t OffScreenMemOffset;
   uint16_t OffScreenMemSize;
// Mandatory information for VBE 3.0 and above
   uint16_t LinBytesPerScanLine;
   uint8_t  BnkNumberOfPages;
   uint8_t  LinNumberOfPages;
   uint8_t  LinRedMaskSize;
   uint8_t  LinRedFieldPosition;
   uint8_t  LinGreenMaskSize;
   uint8_t  LinGreenFieldPosition;
   uint8_t  LinBlueMaskSize;
   uint8_t  LinBlueFieldPosition;
   uint8_t  LinRsvdMaskSize;
   uint8_t  LinRsvdFieldPosition;
   uint32_t MaxPixelClock;
   uint8_t  Reserved[189];
} ModeInfoBlock;

// VBE Return Status Info
// AL
#define VBE_RETURN_STATUS_SUPPORTED                      0x4F
#define VBE_RETURN_STATUS_UNSUPPORTED                    0x00
// AH
#define VBE_RETURN_STATUS_SUCCESSFULL                    0x00
#define VBE_RETURN_STATUS_FAILED                         0x01
#define VBE_RETURN_STATUS_NOT_SUPPORTED                  0x02
#define VBE_RETURN_STATUS_INVALID                        0x03

#endif /* !VBOX_INCLUDED_SRC_Graphics_BIOS_vbe_h */
