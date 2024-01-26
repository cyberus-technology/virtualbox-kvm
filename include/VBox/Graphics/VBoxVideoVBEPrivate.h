/** @file
 * VirtualBox graphics card definitions, private interface for firmware
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef VBOX_INCLUDED_Graphics_VBoxVideoVBEPrivate_h
#define VBOX_INCLUDED_Graphics_VBoxVideoVBEPrivate_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#ifdef VBE
# include <stdint.h>
#else
# include <iprt/types.h>
#endif

/* VBE Mode Numbers */
#define VBE_MODE_VESA_DEFINED                            0x0100
#define VBE_MODE_REFRESH_RATE_USE_CRTC                   0x0800
#define VBE_MODE_LINEAR_FRAME_BUFFER                     0x4000
#define VBE_MODE_PRESERVE_DISPLAY_MEMORY                 0x8000

/* VBE GFX Mode Number */
#define VBE_VESA_MODE_640X400X8                          0x100
#define VBE_VESA_MODE_640X480X8                          0x101
#define VBE_VESA_MODE_800X600X4                          0x102
#define VBE_VESA_MODE_800X600X8                          0x103
#define VBE_VESA_MODE_1024X768X4                         0x104
#define VBE_VESA_MODE_1024X768X8                         0x105
#define VBE_VESA_MODE_1280X1024X4                        0x106
#define VBE_VESA_MODE_1280X1024X8                        0x107
#define VBE_VESA_MODE_320X200X1555                       0x10D
#define VBE_VESA_MODE_320X200X565                        0x10E
#define VBE_VESA_MODE_320X200X888                        0x10F
#define VBE_VESA_MODE_640X480X1555                       0x110
#define VBE_VESA_MODE_640X480X565                        0x111
#define VBE_VESA_MODE_640X480X888                        0x112
#define VBE_VESA_MODE_800X600X1555                       0x113
#define VBE_VESA_MODE_800X600X565                        0x114
#define VBE_VESA_MODE_800X600X888                        0x115
#define VBE_VESA_MODE_1024X768X1555                      0x116
#define VBE_VESA_MODE_1024X768X565                       0x117
#define VBE_VESA_MODE_1024X768X888                       0x118
#define VBE_VESA_MODE_1280X1024X1555                     0x119
#define VBE_VESA_MODE_1280X1024X565                      0x11A
#define VBE_VESA_MODE_1280X1024X888                      0x11B
#define VBE_VESA_MODE_1600X1200X8                        0x11C
#define VBE_VESA_MODE_1600X1200X1555                     0x11D
#define VBE_VESA_MODE_1600X1200X565                      0x11E
#define VBE_VESA_MODE_1600X1200X888                      0x11F

/* BOCHS/PLEX86 'own' mode numbers */
#define VBE_OWN_MODE_320X200X8888                        0x140
#define VBE_OWN_MODE_640X400X8888                        0x141
#define VBE_OWN_MODE_640X480X8888                        0x142
#define VBE_OWN_MODE_800X600X8888                        0x143
#define VBE_OWN_MODE_1024X768X8888                       0x144
#define VBE_OWN_MODE_1280X1024X8888                      0x145
#define VBE_OWN_MODE_320X200X8                           0x146
#define VBE_OWN_MODE_1600X1200X8888                      0x147
#define VBE_OWN_MODE_1152X864X8                          0x148
#define VBE_OWN_MODE_1152X864X1555                       0x149
#define VBE_OWN_MODE_1152X864X565                        0x14a
#define VBE_OWN_MODE_1152X864X888                        0x14b
#define VBE_OWN_MODE_1152X864X8888                       0x14c

/* VirtualBox 'own' mode numbers */
#define VBE_VBOX_MODE_CUSTOM1                            0x160
#define VBE_VBOX_MODE_CUSTOM2                            0x161
#define VBE_VBOX_MODE_CUSTOM3                            0x162
#define VBE_VBOX_MODE_CUSTOM4                            0x163
#define VBE_VBOX_MODE_CUSTOM5                            0x164
#define VBE_VBOX_MODE_CUSTOM6                            0x165
#define VBE_VBOX_MODE_CUSTOM7                            0x166
#define VBE_VBOX_MODE_CUSTOM8                            0x167
#define VBE_VBOX_MODE_CUSTOM9                            0x168
#define VBE_VBOX_MODE_CUSTOM10                           0x169
#define VBE_VBOX_MODE_CUSTOM11                           0x16a
#define VBE_VBOX_MODE_CUSTOM12                           0x16b
#define VBE_VBOX_MODE_CUSTOM13                           0x16c
#define VBE_VBOX_MODE_CUSTOM14                           0x16d
#define VBE_VBOX_MODE_CUSTOM15                           0x16e
#define VBE_VBOX_MODE_CUSTOM16                           0x16f

#define VBE_VESA_MODE_END_OF_LIST                        0xFFFF

/* Capabilities */
#define VBE_CAPABILITY_8BIT_DAC                          0x0001
#define VBE_CAPABILITY_NOT_VGA_COMPATIBLE                0x0002
#define VBE_CAPABILITY_RAMDAC_USE_BLANK_BIT              0x0004
#define VBE_CAPABILITY_STEREOSCOPIC_SUPPORT              0x0008
#define VBE_CAPABILITY_STEREO_VIA_VESA_EVC               0x0010

/* Mode Attributes */
#define VBE_MODE_ATTRIBUTE_SUPPORTED                     0x0001
#define VBE_MODE_ATTRIBUTE_EXTENDED_INFORMATION_AVAILABLE  0x0002
#define VBE_MODE_ATTRIBUTE_TTY_BIOS_SUPPORT              0x0004
#define VBE_MODE_ATTRIBUTE_COLOR_MODE                    0x0008
#define VBE_MODE_ATTRIBUTE_GRAPHICS_MODE                 0x0010
#define VBE_MODE_ATTRIBUTE_NOT_VGA_COMPATIBLE            0x0020
#define VBE_MODE_ATTRIBUTE_NO_VGA_COMPATIBLE_WINDOW      0x0040
#define VBE_MODE_ATTRIBUTE_LINEAR_FRAME_BUFFER_MODE      0x0080
#define VBE_MODE_ATTRIBUTE_DOUBLE_SCAN_MODE              0x0100
#define VBE_MODE_ATTRIBUTE_INTERLACE_MODE                0x0200
#define VBE_MODE_ATTRIBUTE_HARDWARE_TRIPLE_BUFFER        0x0400
#define VBE_MODE_ATTRIBUTE_HARDWARE_STEREOSCOPIC_DISPLAY 0x0800
#define VBE_MODE_ATTRIBUTE_DUAL_DISPLAY_START_ADDRESS    0x1000

#define VBE_MODE_ATTTRIBUTE_LFB_ONLY                     ( VBE_MODE_ATTRIBUTE_NO_VGA_COMPATIBLE_WINDOW | VBE_MODE_ATTRIBUTE_LINEAR_FRAME_BUFFER_MODE )

/* Window attributes */
#define VBE_WINDOW_ATTRIBUTE_RELOCATABLE                 0x01
#define VBE_WINDOW_ATTRIBUTE_READABLE                    0x02
#define VBE_WINDOW_ATTRIBUTE_WRITEABLE                   0x04

/* Memory model */
#define VBE_MEMORYMODEL_TEXT_MODE                        0x00
#define VBE_MEMORYMODEL_CGA_GRAPHICS                     0x01
#define VBE_MEMORYMODEL_HERCULES_GRAPHICS                0x02
#define VBE_MEMORYMODEL_PLANAR                           0x03
#define VBE_MEMORYMODEL_PACKED_PIXEL                     0x04
#define VBE_MEMORYMODEL_NON_CHAIN_4_256                  0x05
#define VBE_MEMORYMODEL_DIRECT_COLOR                     0x06
#define VBE_MEMORYMODEL_YUV                              0x07

/* DirectColorModeInfo */
#define VBE_DIRECTCOLOR_COLOR_RAMP_PROGRAMMABLE          0x01
#define VBE_DIRECTCOLOR_RESERVED_BITS_AVAILABLE          0x02

/* Video memory */
#define VGAMEM_GRAPH                                     0xA000

/**
 * VBE Bios Extra Data structure.
 */
typedef struct VBEHeader
{
    /** Signature (VBEHEADER_MAGIC). */
    uint16_t      u16Signature;
    /** Data size. */
    uint16_t      cbData;
} VBEHeader;

/** The value of the VBEHeader::u16Signature field. */
#define VBEHEADER_MAGIC      0x77CC

/** The extra port which is used to read the mode list. */
#define VBE_EXTRA_PORT       0x3b6

/** The extra port which is used for debug printf. */
#define VBE_PRINTF_PORT      0x3b7

/**
 * This one is for compactly storing a list of mode info blocks
 */
#pragma pack(1) /* pack(1) is important! (you'll get a byte extra for each of the u8 fields elsewise...)
                 * bird: Load of non-sense. You'll get two extra bytes before MaxPixelClock if you don't pack it.  */
typedef struct ModeInfoBlockCompact
{
    /* Mandatory information for all VBE revisions */
    uint16_t     ModeAttributes;
    uint8_t      WinAAttributes;
    uint8_t      WinBAttributes;
    uint16_t     WinGranularity;
    uint16_t     WinSize;
    uint16_t     WinASegment;
    uint16_t     WinBSegment;
    uint32_t     WinFuncPtr;
    uint16_t     BytesPerScanLine;
    /* Mandatory information for VBE 1.2 and above */
    uint16_t     XResolution;
    uint16_t     YResolution;
    uint8_t      XCharSize;
    uint8_t      YCharSize;
    uint8_t      NumberOfPlanes;
    uint8_t      BitsPerPixel;
    uint8_t      NumberOfBanks;
    uint8_t      MemoryModel;
    uint8_t      BankSize;
    uint8_t      NumberOfImagePages;
    uint8_t      Reserved_page;
    /* Direct Color fields (required for direct/6 and YUV/7 memory models) */
    uint8_t      RedMaskSize;
    uint8_t      RedFieldPosition;
    uint8_t      GreenMaskSize;
    uint8_t      GreenFieldPosition;
    uint8_t      BlueMaskSize;
    uint8_t      BlueFieldPosition;
    uint8_t      RsvdMaskSize;
    uint8_t      RsvdFieldPosition;
    uint8_t      DirectColorModeInfo;
    /* Mandatory information for VBE 2.0 and above */
    uint32_t     PhysBasePtr;
    uint32_t     OffScreenMemOffset;
    uint16_t     OffScreenMemSize;
    /* Mandatory information for VBE 3.0 and above */
    uint16_t     LinBytesPerScanLine;
    uint8_t      BnkNumberOfPages;
    uint8_t      LinNumberOfPages;
    uint8_t      LinRedMaskSize;
    uint8_t      LinRedFieldPosition;
    uint8_t      LinGreenMaskSize;
    uint8_t      LinGreenFieldPosition;
    uint8_t      LinBlueMaskSize;
    uint8_t      LinBlueFieldPosition;
    uint8_t      LinRsvdMaskSize;
    uint8_t      LinRsvdFieldPosition;
    uint32_t     MaxPixelClock;
} ModeInfoBlockCompact;
#pragma pack()

typedef struct ModeInfoListItem
{
    uint16_t                mode;
    ModeInfoBlockCompact    info;
} ModeInfoListItem;


#endif /* !VBOX_INCLUDED_Graphics_VBoxVideoVBEPrivate_h */

