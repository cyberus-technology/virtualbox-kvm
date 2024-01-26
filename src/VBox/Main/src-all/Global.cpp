/* $Id: Global.cpp $ */
/** @file
 * VirtualBox COM global definitions
 *
 * NOTE: This file is part of both VBoxC.dll and VBoxSVC.exe.
 */

/*
 * Copyright (C) 2008-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "Global.h"
#include "StringifyEnums.h"

#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/errcore.h>

#include "VBoxNls.h"

DECLARE_TRANSLATION_CONTEXT(GlobalCtx);


/* static */
const Global::OSType Global::sOSTypes[] =
{
    /* NOTE1: we assume that unknown is always the first two entries!
     * NOTE2: please use powers of 2 when specifying the size of harddisks since
     *        '2GB' looks better than '1.95GB' (= 2000MB)
     * NOTE3: if you add new guest OS types please check if the code in
     *        Machine::getEffectiveParavirtProvider and Console::i_configConstructorInner
     *        are still covering the relevant cases. */
    { "Other",   "Other",             "Other",              "Other/Unknown",
      VBOXOSTYPE_Unknown,         VBOXOSHINT_NONE,
      1,   64,   4,  2 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700 },

    { "Other",   "Other",             "Other_64",           "Other/Unknown (64-bit)",
      VBOXOSTYPE_Unknown_x64,     VBOXOSHINT_64BIT | VBOXOSHINT_PAE | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC,
      1,   64,   4,  2 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700 },

    { "Windows", "Microsoft Windows", "Windows31",          "Windows 3.1",
      VBOXOSTYPE_Win31,           VBOXOSHINT_FLOPPY,
      1,   32,   4,  1 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "Windows", "Microsoft Windows", "Windows95",          "Windows 95",
      VBOXOSTYPE_Win95,           VBOXOSHINT_FLOPPY,
      1,   64,   4,  2 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "Windows", "Microsoft Windows", "Windows98",          "Windows 98",
      VBOXOSTYPE_Win98,           VBOXOSHINT_FLOPPY,
      1,   64,   4,  2 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "Windows", "Microsoft Windows", "WindowsMe",          "Windows ME",
      VBOXOSTYPE_WinMe,           VBOXOSHINT_FLOPPY | VBOXOSHINT_USBTABLET,
      1,  128,   4,  4 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Windows", "Microsoft Windows", "WindowsNT3x",        "Windows NT 3.x",
      VBOXOSTYPE_WinNT3x,         VBOXOSHINT_NOUSB | VBOXOSHINT_FLOPPY,
      1,   64,   8,  _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_BusLogic, StorageBus_SCSI,
      StorageControllerType_BusLogic, StorageBus_SCSI, ChipsetType_PIIX3, IommuType_None, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "Windows", "Microsoft Windows", "WindowsNT4",         "Windows NT 4",
      VBOXOSTYPE_WinNT4,          VBOXOSHINT_NOUSB,
      1,  128,  16,  2 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "Windows", "Microsoft Windows", "Windows2000",        "Windows 2000",
      VBOXOSTYPE_Win2k,           VBOXOSHINT_USBTABLET,
      1,  168,  16,  4 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Windows", "Microsoft Windows", "WindowsXP",          "Windows XP (32-bit)",
      VBOXOSTYPE_WinXP,           VBOXOSHINT_USBTABLET,
      1,  192,  16, 10 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82543GC, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Windows", "Microsoft Windows", "WindowsXP_64",       "Windows XP (64-bit)",
      VBOXOSTYPE_WinXP_x64,       VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET,
      1,  512,  16, 10 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Windows", "Microsoft Windows", "Windows2003",        "Windows 2003 (32-bit)",
      VBOXOSTYPE_Win2k3,          VBOXOSHINT_USBTABLET,
      1,  512,  16, 20 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82543GC, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Windows", "Microsoft Windows", "Windows2003_64",     "Windows 2003 (64-bit)",
      VBOXOSTYPE_Win2k3_x64,      VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET,
      1,  512,  16, 20 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "WindowsVista",       "Windows Vista (32-bit)",
      VBOXOSTYPE_WinVista,        VBOXOSHINT_USBTABLET | VBOXOSHINT_WDDM_GRAPHICS,
      1,  512,  16, 25 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "WindowsVista_64",    "Windows Vista (64-bit)",
      VBOXOSTYPE_WinVista_x64,    VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET | VBOXOSHINT_WDDM_GRAPHICS,
      1,  512,  16, 25 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows2008",        "Windows 2008 (32-bit)",
      VBOXOSTYPE_Win2k8,          VBOXOSHINT_USBTABLET | VBOXOSHINT_WDDM_GRAPHICS,
      1, 1024,  16, 32 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows2008_64",     "Windows 2008 (64-bit)",
      VBOXOSTYPE_Win2k8_x64,      VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET | VBOXOSHINT_WDDM_GRAPHICS,
      1, 2048,  16, 32 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows7",           "Windows 7 (32-bit)",
      VBOXOSTYPE_Win7,            VBOXOSHINT_USBTABLET | VBOXOSHINT_WDDM_GRAPHICS,
      1, 1024,  16, 32 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows7_64",        "Windows 7 (64-bit)",
      VBOXOSTYPE_Win7_x64,        VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET | VBOXOSHINT_WDDM_GRAPHICS,
      1, 2048,  16, 32 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows8",           "Windows 8 (32-bit)",
      VBOXOSTYPE_Win8,            VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET | VBOXOSHINT_PAE | VBOXOSHINT_USB3 | VBOXOSHINT_WDDM_GRAPHICS,
      1, 1024, 128, 40 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows8_64",        "Windows 8 (64-bit)",
      VBOXOSTYPE_Win8_x64,        VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET | VBOXOSHINT_USB3 | VBOXOSHINT_WDDM_GRAPHICS,
      1, 2048, 128, 40 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows81",          "Windows 8.1 (32-bit)",
      VBOXOSTYPE_Win81,           VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET | VBOXOSHINT_PAE | VBOXOSHINT_USB3 | VBOXOSHINT_WDDM_GRAPHICS,
      1, 1024, 128, 40 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows81_64",       "Windows 8.1 (64-bit)",
      VBOXOSTYPE_Win81_x64,       VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET | VBOXOSHINT_USB3 | VBOXOSHINT_WDDM_GRAPHICS,
      1, 2048, 128, 40 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows2012_64",     "Windows 2012 (64-bit)",
      VBOXOSTYPE_Win2k12_x64,     VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET | VBOXOSHINT_USB3 | VBOXOSHINT_WDDM_GRAPHICS,
      1, 2048, 128, 50 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows10",          "Windows 10 (32-bit)",
      VBOXOSTYPE_Win10,           VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET | VBOXOSHINT_PAE | VBOXOSHINT_USB3 | VBOXOSHINT_WDDM_GRAPHICS,
      1, 1024, 128, 50 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows10_64",       "Windows 10 (64-bit)",
      VBOXOSTYPE_Win10_x64,       VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET | VBOXOSHINT_USB3 | VBOXOSHINT_WDDM_GRAPHICS,
      1, 2048, 128, 50 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows2016_64",     "Windows 2016 (64-bit)",
      VBOXOSTYPE_Win2k16_x64,     VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET | VBOXOSHINT_USB3 | VBOXOSHINT_WDDM_GRAPHICS,
      1, 2048, 128, 50 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows2019_64",     "Windows 2019 (64-bit)",
      VBOXOSTYPE_Win2k19_x64,     VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET | VBOXOSHINT_USB3 | VBOXOSHINT_WDDM_GRAPHICS,
      1, 2048, 128, 50 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows11_64",       "Windows 11 (64-bit)",
      VBOXOSTYPE_Win11_x64,       VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_EFI | VBOXOSHINT_USBTABLET | VBOXOSHINT_USB3 | VBOXOSHINT_EFI_SECUREBOOT | VBOXOSHINT_TPM2 | VBOXOSHINT_WDDM_GRAPHICS,
      2, 4096, 128, 80 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows2022_64",     "Windows 2022 (64-bit)",
      VBOXOSTYPE_Win2k22_x64,     VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET | VBOXOSHINT_USB3 | VBOXOSHINT_WDDM_GRAPHICS,
      1, 2048, 128, 50 * _1G64, GraphicsControllerType_VBoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "WindowsNT",          "Other Windows (32-bit)",
      VBOXOSTYPE_WinNT,           VBOXOSHINT_NONE,
      1,  512,  16, 20 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Windows", "Microsoft Windows", "WindowsNT_64",       "Other Windows (64-bit)",
      VBOXOSTYPE_WinNT_x64,       VBOXOSHINT_64BIT | VBOXOSHINT_PAE | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET,
      1,  512,  16, 20 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

#define VBOX_LINUX_OSHINTS_A_32   (VBOXOSHINT_RTCUTC | VBOXOSHINT_USBTABLET | VBOXOSHINT_X2APIC | VBOXOSHINT_PAE)
#define VBOX_LINUX_OSHINTS_A_64   (VBOXOSHINT_RTCUTC | VBOXOSHINT_USBTABLET | VBOXOSHINT_X2APIC | VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC)

#define VBOX_LINUX_OSHINTS_B_32   (VBOXOSHINT_RTCUTC | VBOXOSHINT_PAE | VBOXOSHINT_X2APIC)
#define VBOX_LINUX_OSHINTS_B_64   (VBOXOSHINT_RTCUTC | VBOXOSHINT_PAE | VBOXOSHINT_X2APIC | VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC)

#define VBOX_LINUX_OSHINTS_C_32   (VBOXOSHINT_RTCUTC | VBOXOSHINT_X2APIC | VBOXOSHINT_PAE)
#define VBOX_LINUX_OSHINTS_C_64   (VBOXOSHINT_RTCUTC | VBOXOSHINT_X2APIC | VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC)

#define VBOX_LINUX_OSHINTS_D_32   (VBOXOSHINT_RTCUTC | VBOXOSHINT_PAE)
#define VBOX_LINUX_OSHINTS_D_64   (VBOXOSHINT_RTCUTC | VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC)

#define VBOX_LINUX_OSTYPE_32(a_OStype)      (VBOXOSTYPE_ ## a_OStype)
#define VBOX_LINUX_OSTYPE_64(a_OStype)      (VBOXOSTYPE_ ## a_OStype ## _x64)

#define VBOX_LINUX_OSID_STR(a_OSid)         (# a_OSid)
#define VBOX_LINUX_OSID_STR_64(a_OSid)      VBOX_LINUX_OSID_STR(a_OSid ## _64)

#define VBOX_LINUX_SUBTYPE_TEMPLATE_32(a_Id, a_Description, a_OStype, a_OSHint, a_Memory, a_Vram, a_Diskspace, \
                                       a_NetworkAdapter, a_HDStorageController, a_HDStorageBusType) \
    { "Linux",   "Linux", VBOX_LINUX_OSID_STR(a_Id), a_Description, VBOX_LINUX_OSTYPE_32(a_OStype), a_OSHint, \
      1, a_Memory, a_Vram, a_Diskspace * _1G64, GraphicsControllerType_VMSVGA, a_NetworkAdapter, 0, StorageControllerType_PIIX4, StorageBus_IDE, \
      a_HDStorageController, a_HDStorageBusType, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_AD1980  }

#define VBOX_LINUX_SUBTYPE_TEMPLATE_64(a_Id, a_Description, a_OStype, a_OSHint, a_Memory, a_Vram, a_Diskspace, \
                                       a_NetworkAdapter, a_HDStorageController, a_HDStorageBusType) \
    { "Linux",   "Linux", VBOX_LINUX_OSID_STR_64(a_Id), a_Description, VBOX_LINUX_OSTYPE_64(a_OStype), a_OSHint, \
      1, a_Memory, a_Vram, a_Diskspace * _1G64, GraphicsControllerType_VMSVGA, a_NetworkAdapter, 0, StorageControllerType_PIIX4, StorageBus_IDE, \
      a_HDStorageController, a_HDStorageBusType, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_AD1980  }

/* Linux 32-bit sub-type template defaulting to 1 CPU with USB-tablet-mouse/VMSVGA/Intel-Pro1000/PIIX4+IDE DVD/AHCI+SATA disk/AC97 */
#define VBOX_LINUX_SUBTYPE_A_32(a_Id, a_Description, a_Memory, a_Vram, a_Diskspace) \
    VBOX_LINUX_SUBTYPE_TEMPLATE_32(a_Id, a_Description, a_Id, VBOX_LINUX_OSHINTS_A_32, a_Memory, a_Vram, a_Diskspace, \
                                   NetworkAdapterType_I82540EM, StorageControllerType_IntelAhci, StorageBus_SATA)

/* Linux 64-bit sub-type template defaulting to 1 CPU with USB-tablet-mouse/VMSVGA/Intel-Pro1000/PIIX4+IDE DVD/AHCI+SATA disk/AC97 */
#define VBOX_LINUX_SUBTYPE_A_64(a_Id, a_Description, a_Memory, a_Vram, a_Diskspace) \
    VBOX_LINUX_SUBTYPE_TEMPLATE_64(a_Id, a_Description, a_Id, VBOX_LINUX_OSHINTS_A_64, a_Memory, a_Vram, a_Diskspace, \
                                   NetworkAdapterType_I82540EM, StorageControllerType_IntelAhci, StorageBus_SATA)

#define VBOX_LINUX_SUBTYPE_A_WITH_OSTYPE_32(a_Id, a_Description, a_OStype, a_Memory, a_Vram, a_Diskspace) \
    VBOX_LINUX_SUBTYPE_TEMPLATE_32(a_Id, a_Description, a_OStype, VBOX_LINUX_OSHINTS_A_32, a_Memory, a_Vram, a_Diskspace, \
                                   NetworkAdapterType_I82540EM, StorageControllerType_IntelAhci, StorageBus_SATA)

#define VBOX_LINUX_SUBTYPE_A_WITH_OSTYPE_64(a_Id, a_Description, a_OStype, a_Memory, a_Vram, a_Diskspace) \
    VBOX_LINUX_SUBTYPE_TEMPLATE_64(a_Id, a_Description, a_OStype, VBOX_LINUX_OSHINTS_A_64, a_Memory, a_Vram, a_Diskspace, \
                                   NetworkAdapterType_I82540EM, StorageControllerType_IntelAhci, StorageBus_SATA)

/* Linux 32-bit sub-type template defaulting to 1 CPU with PS/2-mouse/PAE-NX/VMSVGA/Intel-Pro1000/PIIX4+IDE DVD/AHCI+SATA disk/AC97 */
#define VBOX_LINUX_SUBTYPE_B_32(a_Id, a_Description, a_Memory, a_Vram, a_Diskspace) \
    VBOX_LINUX_SUBTYPE_TEMPLATE_32(a_Id, a_Description, a_Id, VBOX_LINUX_OSHINTS_B_32, a_Memory, a_Vram, a_Diskspace, \
                                   NetworkAdapterType_I82540EM, StorageControllerType_IntelAhci, StorageBus_SATA)

/* Linux 64-bit sub-type template defaulting to 1 CPU with PS/2-mouse/PAE-NX/VMSVGA/Intel-Pro1000/PIIX4+IDE DVD/AHCI+SATA disk/AC97 */
#define VBOX_LINUX_SUBTYPE_B_64(a_Id, a_Description, a_Memory, a_Vram, a_Diskspace) \
    VBOX_LINUX_SUBTYPE_TEMPLATE_64(a_Id, a_Description, a_Id, VBOX_LINUX_OSHINTS_B_64, a_Memory, a_Vram, a_Diskspace, \
                                   NetworkAdapterType_I82540EM, StorageControllerType_IntelAhci, StorageBus_SATA)

/* Linux 32-bit sub-type template defaulting to 1 CPU with PS/2-mouse/VMSVGA/Intel-Pro1000/PIIX4+IDE DVD/AHCI+SATA disk/AC97 */
#define VBOX_LINUX_SUBTYPE_C_32(a_Id, a_Description, a_Memory, a_Vram, a_Diskspace) \
    VBOX_LINUX_SUBTYPE_TEMPLATE_32(a_Id, a_Description, a_Id, VBOX_LINUX_OSHINTS_C_32, a_Memory, a_Vram, a_Diskspace, \
                                   NetworkAdapterType_I82540EM, StorageControllerType_IntelAhci, StorageBus_SATA)

/* Linux 64-bit sub-type template defaulting to 1 CPU with PS/2-mouse/VMSVGA/Intel-Pro1000/PIIX4+IDE DVD/AHCI+SATA disk/AC97 */
#define VBOX_LINUX_SUBTYPE_C_64(a_Id, a_Description, a_Memory, a_Vram, a_Diskspace) \
    VBOX_LINUX_SUBTYPE_TEMPLATE_64(a_Id, a_Description, a_Id, VBOX_LINUX_OSHINTS_C_64, a_Memory, a_Vram, a_Diskspace, \
                                   NetworkAdapterType_I82540EM, StorageControllerType_IntelAhci, StorageBus_SATA)

/* Linux 32-bit sub-type template defaulting to 1 CPU with PS/2-mouse/VMSVGA/PCnet-FASTIII/PIIX4+IDE DVD/PIIX4+IDE disk/AC97 */
#define VBOX_LINUX_SUBTYPE_D_32(a_Id, a_Description, a_Memory, a_Vram, a_Diskspace) \
    VBOX_LINUX_SUBTYPE_TEMPLATE_32(a_Id, a_Description, a_Id, VBOX_LINUX_OSHINTS_D_32, a_Memory, a_Vram, a_Diskspace, \
                                   NetworkAdapterType_Am79C973, StorageControllerType_PIIX4, StorageBus_IDE)

/* Linux 64-bit sub-type template defaulting to 1 CPU with PS/2-mouse/VMSVGA/PCnet-FASTIII/PIIX4+IDE DVD/PIIX4+IDE disk/AC97 */
#define VBOX_LINUX_SUBTYPE_D_64(a_Id, a_Description, a_Memory, a_Vram, a_Diskspace) \
    VBOX_LINUX_SUBTYPE_TEMPLATE_64(a_Id, a_Description, a_Id, VBOX_LINUX_OSHINTS_D_64, a_Memory, a_Vram, a_Diskspace, \
                                   NetworkAdapterType_I82540EM, StorageControllerType_PIIX4, StorageBus_IDE)

    VBOX_LINUX_SUBTYPE_D_32(Linux22, "Linux 2.2 (32-bit)",                      64,  4, 2),
    VBOX_LINUX_SUBTYPE_D_32(Linux24, "Linux 2.4 (32-bit)",                     128, 16, 2),
    VBOX_LINUX_SUBTYPE_D_64(Linux24, "Linux 2.4 (64-bit)",                    1024, 16, 4),
    VBOX_LINUX_SUBTYPE_A_32(Linux26, "Linux 2.6 / 3.x / 4.x / 5.x (32-bit)",  1024, 16, 8),
    VBOX_LINUX_SUBTYPE_A_64(Linux26, "Linux 2.6 / 3.x / 4.x / 5.x (64-bit)",  1024, 16, 8),

    VBOX_LINUX_SUBTYPE_A_32(ArchLinux, "Arch Linux (32-bit)", 1024, 16, 8),
    VBOX_LINUX_SUBTYPE_A_64(ArchLinux, "Arch Linux (64-bit)", 1024, 16, 8),

    VBOX_LINUX_SUBTYPE_A_32(Debian,   "Debian (32-bit)",             2048, 16, 20),
    VBOX_LINUX_SUBTYPE_A_64(Debian,   "Debian (64-bit)",             2048, 16, 20),
    VBOX_LINUX_SUBTYPE_A_32(Debian31, "Debian 3.1 Sarge (32-bit)",   1024, 16, 8),  // 32-bit only
    VBOX_LINUX_SUBTYPE_A_32(Debian4,  "Debian 4.0 Etch (32-bit)",    1024, 16, 8),
    VBOX_LINUX_SUBTYPE_A_64(Debian4,  "Debian 4.0 Etch (64-bit)",    1024, 16, 8),
    VBOX_LINUX_SUBTYPE_A_32(Debian5,  "Debian 5.0 Lenny (32-bit)",   1024, 16, 8),
    VBOX_LINUX_SUBTYPE_A_64(Debian5,  "Debian 5.0 Lenny (64-bit)",   1024, 16, 8),
    VBOX_LINUX_SUBTYPE_A_32(Debian6,  "Debian 6.0 Squeeze (32-bit)", 1024, 16, 8),
    VBOX_LINUX_SUBTYPE_A_64(Debian6,  "Debian 6.0 Squeeze (64-bit)", 1024, 16, 8),
    VBOX_LINUX_SUBTYPE_A_32(Debian7,  "Debian 7 Wheezy (32-bit)",    2048, 16, 20),
    VBOX_LINUX_SUBTYPE_A_64(Debian7,  "Debian 7 Wheezy (64-bit)",    2048, 16, 20),
    VBOX_LINUX_SUBTYPE_A_32(Debian8,  "Debian 8 Jessie (32-bit)",    2048, 16, 20),
    VBOX_LINUX_SUBTYPE_A_64(Debian8,  "Debian 8 Jessie (64-bit)",    2048, 16, 20),
    VBOX_LINUX_SUBTYPE_A_32(Debian9,  "Debian 9 Stretch (32-bit)",   2048, 16, 20),
    VBOX_LINUX_SUBTYPE_A_64(Debian9,  "Debian 9 Stretch (64-bit)",   2048, 16, 20),
    VBOX_LINUX_SUBTYPE_A_32(Debian10, "Debian 10 Buster (32-bit)",   2048, 16, 20),
    VBOX_LINUX_SUBTYPE_A_64(Debian10, "Debian 10 Buster (64-bit)",   2048, 16, 20),
    VBOX_LINUX_SUBTYPE_A_32(Debian11, "Debian 11 Bullseye (32-bit)", 2048, 16, 20),
    VBOX_LINUX_SUBTYPE_A_64(Debian11, "Debian 11 Bullseye (64-bit)", 2048, 16, 20),
    VBOX_LINUX_SUBTYPE_A_32(Debian12, "Debian 12 Bookworm (32-bit)", 2048, 16, 20),
    VBOX_LINUX_SUBTYPE_A_64(Debian12, "Debian 12 Bookworm (64-bit)", 2048, 16, 20),

    VBOX_LINUX_SUBTYPE_A_WITH_OSTYPE_32(Fedora, "Fedora (32-bit)", FedoraCore, 2048, 16, 15),
    VBOX_LINUX_SUBTYPE_A_WITH_OSTYPE_64(Fedora, "Fedora (64-bit)", FedoraCore, 2048, 16, 15),

    VBOX_LINUX_SUBTYPE_A_32(Gentoo, "Gentoo (32-bit)", 1024, 16, 8),
    VBOX_LINUX_SUBTYPE_A_64(Gentoo, "Gentoo (64-bit)", 1024, 16, 8),

    VBOX_LINUX_SUBTYPE_A_32(Mandriva,  "Mandriva (32-bit)",        1024, 16, 8),
    VBOX_LINUX_SUBTYPE_A_64(Mandriva,  "Mandriva (64-bit)",        1024, 16, 8),
    VBOX_LINUX_SUBTYPE_A_32(OpenMandriva_Lx, "OpenMandriva Lx (32-bit)", 2048, 16, 10),
    VBOX_LINUX_SUBTYPE_A_64(OpenMandriva_Lx, "OpenMandriva Lx (64-bit)", 2048, 16, 10),
    VBOX_LINUX_SUBTYPE_A_32(PCLinuxOS, "PCLinuxOS / PCLOS (32-bit)",     2048, 16, 10),
    VBOX_LINUX_SUBTYPE_A_64(PCLinuxOS, "PCLinuxOS / PCLOS (64-bit)",     2048, 16, 10),
    VBOX_LINUX_SUBTYPE_A_32(Mageia,    "Mageia (32-bit)",                2048, 16, 10),
    VBOX_LINUX_SUBTYPE_A_64(Mageia,    "Mageia (64-bit)",                2048, 16, 10),

    VBOX_LINUX_SUBTYPE_B_32(Oracle,   "Oracle Linux (32-bit)",      2048, 16, 20),
    VBOX_LINUX_SUBTYPE_B_64(Oracle,   "Oracle Linux (64-bit)",      2048, 16, 20),
    VBOX_LINUX_SUBTYPE_B_32(Oracle4,  "Oracle Linux 4.x (32-bit)",  1024, 16, 8),
    VBOX_LINUX_SUBTYPE_B_64(Oracle4,  "Oracle Linux 4.x (64-bit)",  1024, 16, 8),
    VBOX_LINUX_SUBTYPE_B_32(Oracle5,  "Oracle Linux 5.x (32-bit)",  1024, 16, 8),
    VBOX_LINUX_SUBTYPE_B_64(Oracle5,  "Oracle Linux 5.x (64-bit)",  1024, 16, 8),
    VBOX_LINUX_SUBTYPE_B_32(Oracle6,  "Oracle Linux 6.x (32-bit)",  2048, 16, 10),
    VBOX_LINUX_SUBTYPE_B_64(Oracle6,  "Oracle Linux 6.x (64-bit)",  2048, 16, 10),
    VBOX_LINUX_SUBTYPE_B_64(Oracle7,  "Oracle Linux 7.x (64-bit)",  2048, 16, 20),  // 64-bit only
    VBOX_LINUX_SUBTYPE_B_64(Oracle8,  "Oracle Linux 8.x (64-bit)",  2048, 16, 20),  // 64-bit only
    VBOX_LINUX_SUBTYPE_B_64(Oracle9,  "Oracle Linux 9.x (64-bit)",  2048, 16, 20),  // 64-bit only

    VBOX_LINUX_SUBTYPE_B_32(RedHat,   "Red Hat (32-bit)",      2048, 16, 20),
    VBOX_LINUX_SUBTYPE_B_64(RedHat,   "Red Hat (64-bit)",      2048, 16, 20),
    VBOX_LINUX_SUBTYPE_B_32(RedHat3,  "Red Hat 3.x (32-bit)",  1024, 16, 8),
    VBOX_LINUX_SUBTYPE_B_64(RedHat3,  "Red Hat 3.x (64-bit)",  1024, 16, 8),
    VBOX_LINUX_SUBTYPE_B_32(RedHat4,  "Red Hat 4.x (32-bit)",  1024, 16, 8),
    VBOX_LINUX_SUBTYPE_B_64(RedHat4,  "Red Hat 4.x (64-bit)",  1024, 16, 8),
    VBOX_LINUX_SUBTYPE_B_32(RedHat5,  "Red Hat 5.x (32-bit)",  1024, 16, 8),
    VBOX_LINUX_SUBTYPE_B_64(RedHat5,  "Red Hat 5.x (64-bit)",  1024, 16, 8),
    VBOX_LINUX_SUBTYPE_B_32(RedHat6,  "Red Hat 6.x (32-bit)",  1024, 16, 10),
    VBOX_LINUX_SUBTYPE_B_64(RedHat6,  "Red Hat 6.x (64-bit)",  1024, 16, 10),
    VBOX_LINUX_SUBTYPE_B_64(RedHat7,  "Red Hat 7.x (64-bit)",  2048, 16, 20),  // 64-bit only
    VBOX_LINUX_SUBTYPE_B_64(RedHat8,  "Red Hat 8.x (64-bit)",  2048, 16, 20),  // 64-bit only
    VBOX_LINUX_SUBTYPE_B_64(RedHat9,  "Red Hat 9.x (64-bit)",  2048, 16, 20),  // 64-bit only

    VBOX_LINUX_SUBTYPE_A_32(OpenSUSE,            "openSUSE (32-bit)",               1024, 16, 8),
    VBOX_LINUX_SUBTYPE_A_64(OpenSUSE,            "openSUSE (64-bit)",               1024, 16, 8),
    VBOX_LINUX_SUBTYPE_A_64(OpenSUSE_Leap,       "openSUSE Leap (64-bit)",          2048, 16, 8),  // 64-bit only
    VBOX_LINUX_SUBTYPE_A_32(OpenSUSE_Tumbleweed, "openSUSE Tumbleweed (32-bit)",    2048, 16, 8),
    VBOX_LINUX_SUBTYPE_A_64(OpenSUSE_Tumbleweed, "openSUSE Tumbleweed (64-bit)",    2048, 16, 8),
    VBOX_LINUX_SUBTYPE_A_32(SUSE_LE,             "SUSE Linux Enterprise (32-bit)",  2048, 16, 8),
    VBOX_LINUX_SUBTYPE_A_64(SUSE_LE,             "SUSE Linux Enterprise (64-bit)",  2048, 16, 8),

    VBOX_LINUX_SUBTYPE_A_32(Turbolinux, "Turbolinux (32-bit)", 384, 16, 8),
    VBOX_LINUX_SUBTYPE_A_64(Turbolinux, "Turbolinux (64-bit)", 384, 16, 8),

    VBOX_LINUX_SUBTYPE_A_32(Ubuntu,       "Ubuntu (32-bit)",                             2048, 16, 25),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu,       "Ubuntu (64-bit)",                             2048, 16, 25),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu10_LTS, "Ubuntu 10.04 LTS (Lucid Lynx) (32-bit)",       256, 16, 3),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu10_LTS, "Ubuntu 10.04 LTS (Lucid Lynx) (64-bit)",       256, 16, 3),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu10,     "Ubuntu 10.10 (Maverick Meerkat) (32-bit)",     256, 16, 3),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu10,     "Ubuntu 10.10 (Maverick Meerkat) (64-bit)",     256, 16, 3),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu11,     "Ubuntu 11.04 (Natty Narwhal) / 11.10 (Oneiric Ocelot) (32-bit)",  384, 16, 5),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu11,     "Ubuntu 11.04 (Natty Narwhal) / 11.10 (Oneiric Ocelot) (64-bit)",  384, 16, 5),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu12_LTS, "Ubuntu 12.04 LTS (Precise Pangolin) (32-bit)", 768, 16, 5),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu12_LTS, "Ubuntu 12.04 LTS (Precise Pangolin) (64-bit)", 768, 16, 5),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu12,     "Ubuntu 12.10 (Quantal Quetzal) (32-bit)",      768, 16, 5),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu12,     "Ubuntu 12.10 (Quantal Quetzal) (64-bit)",      768, 16, 5),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu13,     "Ubuntu 13.04 (Raring Ringtail) / 13.10 (Saucy Salamander) (32-bit)",  768, 16, 5),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu13,     "Ubuntu 13.04 (Raring Ringtail) / 13.10 (Saucy Salamander) (64-bit)",  768, 16, 5),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu14_LTS, "Ubuntu 14.04 LTS (Trusty Tahr) (32-bit)",     1536, 16, 7),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu14_LTS, "Ubuntu 14.04 LTS (Trusty Tahr) (64-bit)",     1536, 16, 7),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu14,     "Ubuntu 14.10 (Utopic Unicorn) (32-bit)",      1536, 16, 7),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu14,     "Ubuntu 14.10 (Utopic Unicorn) (64-bit)",      1536, 16, 7),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu15,     "Ubuntu 15.04 (Vivid Vervet) / 15.10 (Wily Werewolf) (32-bit)",  1536, 16, 7),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu15,     "Ubuntu 15.04 (Vivid Vervet) / 15.10 (Wily Werewolf) (64-bit)",  1536, 16, 7),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu16_LTS, "Ubuntu 16.04 LTS (Xenial Xerus) (32-bit)",    1536, 16, 10),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu16_LTS, "Ubuntu 16.04 LTS (Xenial Xerus) (64-bit)",    1536, 16, 10),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu16,     "Ubuntu 16.10 (Yakkety Yak) (32-bit)",         1536, 16, 10),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu16,     "Ubuntu 16.10 (Yakkety Yak) (64-bit)",         1536, 16, 10),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu17,     "Ubuntu 17.04 (Zesty Zapus) / 17.10 (Artful Aardvark) (32-bit)", 1536, 16, 10),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu17,     "Ubuntu 17.04 (Zesty Zapus) / 17.10 (Artful Aardvark) (64-bit)", 1536, 16, 10),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu18_LTS, "Ubuntu 18.04 LTS (Bionic Beaver) (32-bit)",   2048, 16, 25),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu18_LTS, "Ubuntu 18.04 LTS (Bionic Beaver) (64-bit)",   2048, 16, 25),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu18,     "Ubuntu 18.10 (Cosmic Cuttlefish) (32-bit)",   2048, 16, 25),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu18,     "Ubuntu 18.10 (Cosmic Cuttlefish) (64-bit)",   2048, 16, 25),
    VBOX_LINUX_SUBTYPE_A_32(Ubuntu19,     "Ubuntu 19.04 (Disco Dingo) / 19.10 (Eoan Ermine) (32-bit)",     2048, 16, 25),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu19,     "Ubuntu 19.04 (Disco Dingo) / 19.10 (Eoan Ermine) (64-bit)",     2048, 16, 25),
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu20_LTS, "Ubuntu 20.04 LTS (Focal Fossa) (64-bit)",     2048, 16, 25),  // 64-bit only
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu20,     "Ubuntu 20.10 (Groovy Gorilla) (64-bit)",      2048, 16, 25),  // 64-bit only
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu21,     "Ubuntu 21.04 (Hirsute Hippo) / 21.10 (Impish Indri) (64-bit)",  2048, 16, 25), // 64-bit only
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu22_LTS, "Ubuntu 22.04 LTS (Jammy Jellyfish) (64-bit)", 2048, 16, 25), // 64-bit only
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu22,     "Ubuntu 22.10 (Kinetic Kudu) (64-bit)", 2048, 16, 25), // 64-bit only
    VBOX_LINUX_SUBTYPE_A_64(Ubuntu23,     "Ubuntu 23.04 (Lunar Lobster) (64-bit)", 2048, 16, 25), // 64-bit only
    VBOX_LINUX_SUBTYPE_A_32(Lubuntu,      "Lubuntu (32-bit)",  1024, 16, 10),
    VBOX_LINUX_SUBTYPE_A_64(Lubuntu,      "Lubuntu (64-bit)",  1024, 16, 10),
    VBOX_LINUX_SUBTYPE_A_32(Xubuntu,      "Xubuntu (32-bit)",  1024, 16, 10),
    VBOX_LINUX_SUBTYPE_A_64(Xubuntu,      "Xubuntu (64-bit)",  1024, 16, 10),

    VBOX_LINUX_SUBTYPE_C_32(Xandros, "Xandros (32-bit)", 1024, 16, 8),
    VBOX_LINUX_SUBTYPE_C_64(Xandros, "Xandros (64-bit)", 1024, 16, 8),

    VBOX_LINUX_SUBTYPE_A_32(Linux, "Other Linux (32-bit)", 256, 16, 8),
    VBOX_LINUX_SUBTYPE_B_64(Linux, "Other Linux (64-bit)", 512, 16, 8),

    { "Solaris", "Solaris",           "Solaris",            "Oracle Solaris 10 5/09 and earlier (32-bit)",
      VBOXOSTYPE_Solaris,         VBOXOSHINT_NONE,
      1, 1024,  16, 32 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Solaris", "Solaris",           "Solaris_64",         "Oracle Solaris 10 5/09 and earlier (64-bit)",
      VBOXOSTYPE_Solaris_x64,     VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC,
      1, 2048,  16, 32 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Solaris", "Solaris",           "Solaris10U8_or_later",        "Oracle Solaris 10 10/09 and later (32-bit)",
      VBOXOSTYPE_Solaris10U8_or_later,     VBOXOSHINT_USBTABLET,
      1, 1024,  16, 32 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Solaris", "Solaris",           "Solaris10U8_or_later_64",     "Oracle Solaris 10 10/09 and later (64-bit)",
      VBOXOSTYPE_Solaris10U8_or_later_x64, VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET,
      1, 2048,  16, 32 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Solaris", "Solaris",           "Solaris11_64",       "Oracle Solaris 11 (64-bit)",
      VBOXOSTYPE_Solaris11_x64,   VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET | VBOXOSHINT_RTCUTC,
      1, 4096,  16, 32 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Solaris", "Solaris",           "OpenSolaris",        "OpenSolaris / Illumos / OpenIndiana (32-bit)",
      VBOXOSTYPE_OpenSolaris,     VBOXOSHINT_USBTABLET,
      1, 1024,  16, 32 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Solaris", "Solaris",           "OpenSolaris_64",     "OpenSolaris / Illumos / OpenIndiana (64-bit)",
      VBOXOSTYPE_OpenSolaris_x64, VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_USBTABLET,
      1, 2048,  16, 32 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "BSD",     "BSD",               "FreeBSD",            "FreeBSD (32-bit)",
      VBOXOSTYPE_FreeBSD,         VBOXOSHINT_NONE,
      1, 1024,  16,  2 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "BSD",     "BSD",               "FreeBSD_64",         "FreeBSD (64-bit)",
      VBOXOSTYPE_FreeBSD_x64,     VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC,
      1, 1024,  16, 16 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "BSD",     "BSD",               "OpenBSD",            "OpenBSD (32-bit)",
      VBOXOSTYPE_OpenBSD,         VBOXOSHINT_HWVIRTEX,
      1, 1024,  16, 16 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "BSD",     "BSD",               "OpenBSD_64",         "OpenBSD (64-bit)",
      VBOXOSTYPE_OpenBSD_x64,     VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC,
      1, 1024,  16, 16 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "BSD",     "BSD",               "NetBSD",             "NetBSD (32-bit)",
      VBOXOSTYPE_NetBSD,          VBOXOSHINT_RTCUTC,
      1, 1024,  16, 16 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "BSD",     "BSD",               "NetBSD_64",          "NetBSD (64-bit)",
      VBOXOSTYPE_NetBSD_x64,      VBOXOSHINT_64BIT | VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_RTCUTC,
      1, 1024,  16, 16 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "OS2",     "IBM OS/2",          "OS21x",              "OS/2 1.x",
      VBOXOSTYPE_OS21x,           VBOXOSHINT_FLOPPY | VBOXOSHINT_NOUSB | VBOXOSHINT_TFRESET,
      1,    8,   4, 500 * _1M, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "OS2",     "IBM OS/2",          "OS2Warp3",           "OS/2 Warp 3",
      VBOXOSTYPE_OS2Warp3,        VBOXOSHINT_HWVIRTEX | VBOXOSHINT_FLOPPY,
      1,   48,   4,  1 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "OS2",     "IBM OS/2",          "OS2Warp4",           "OS/2 Warp 4",
      VBOXOSTYPE_OS2Warp4,        VBOXOSHINT_HWVIRTEX | VBOXOSHINT_FLOPPY,
      1,   64,   4,  2 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "OS2",     "IBM OS/2",          "OS2Warp45",          "OS/2 Warp 4.5",
      VBOXOSTYPE_OS2Warp45,       VBOXOSHINT_HWVIRTEX | VBOXOSHINT_FLOPPY,
      1,  128,   4,  2 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "OS2",     "IBM OS/2",          "OS2eCS",             "eComStation",
      VBOXOSTYPE_ECS,             VBOXOSHINT_HWVIRTEX | VBOXOSHINT_FLOPPY,
      1,  256,   4,  2 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "OS2",     "IBM OS/2",          "OS2ArcaOS",          "ArcaOS",
      VBOXOSTYPE_ArcaOS,          VBOXOSHINT_HWVIRTEX | VBOXOSHINT_FLOPPY,
      1, 1024,   4,  2 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82540EM, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700 },

    { "OS2",     "IBM OS/2",          "OS2",                "Other OS/2",
      VBOXOSTYPE_OS2,             VBOXOSHINT_HWVIRTEX | VBOXOSHINT_FLOPPY | VBOXOSHINT_NOUSB,
      1,   96,   4,  2 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "MacOS",   "Mac OS X",          "MacOS",              "Mac OS X (32-bit)",
      VBOXOSTYPE_MacOS,           VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_EFI | VBOXOSHINT_PAE
                                | VBOXOSHINT_USBHID | VBOXOSHINT_HPET | VBOXOSHINT_RTCUTC | VBOXOSHINT_USBTABLET,
      1, 2048,  16, 20 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS_64",           "Mac OS X (64-bit)",
      VBOXOSTYPE_MacOS_x64,       VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_EFI | VBOXOSHINT_PAE |  VBOXOSHINT_64BIT
                                | VBOXOSHINT_USBHID | VBOXOSHINT_HPET | VBOXOSHINT_RTCUTC | VBOXOSHINT_USBTABLET,
      1, 2048,  16, 20 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS106",           "Mac OS X 10.6 Snow Leopard (32-bit)",
      VBOXOSTYPE_MacOS106,        VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_EFI | VBOXOSHINT_PAE
                                | VBOXOSHINT_USBHID | VBOXOSHINT_HPET | VBOXOSHINT_RTCUTC | VBOXOSHINT_USBTABLET,
      1, 2048,  16, 20 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS106_64",        "Mac OS X 10.6 Snow Leopard (64-bit)",
      VBOXOSTYPE_MacOS106_x64,    VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_EFI | VBOXOSHINT_PAE |  VBOXOSHINT_64BIT
                                | VBOXOSHINT_USBHID | VBOXOSHINT_HPET | VBOXOSHINT_RTCUTC | VBOXOSHINT_USBTABLET,
      1, 2048,  16, 20 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS107_64",        "Mac OS X 10.7 Lion (64-bit)",
      VBOXOSTYPE_MacOS107_x64,    VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_EFI | VBOXOSHINT_PAE |  VBOXOSHINT_64BIT
                                | VBOXOSHINT_USBHID | VBOXOSHINT_HPET | VBOXOSHINT_RTCUTC | VBOXOSHINT_USBTABLET,
      1, 2048,  16, 20 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS108_64",        "Mac OS X 10.8 Mountain Lion (64-bit)",  /* Aka "Mountain Kitten". */
      VBOXOSTYPE_MacOS108_x64,    VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_EFI | VBOXOSHINT_PAE |  VBOXOSHINT_64BIT
                                | VBOXOSHINT_USBHID | VBOXOSHINT_HPET | VBOXOSHINT_RTCUTC | VBOXOSHINT_USBTABLET,
      1, 2048,  16, 20 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS109_64",        "Mac OS X 10.9 Mavericks (64-bit)", /* Not to be confused with McCain. */
      VBOXOSTYPE_MacOS109_x64,    VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_EFI | VBOXOSHINT_PAE |  VBOXOSHINT_64BIT
                                | VBOXOSHINT_USBHID | VBOXOSHINT_HPET | VBOXOSHINT_RTCUTC | VBOXOSHINT_USBTABLET,
      1, 2048,  16, 25 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS1010_64",       "Mac OS X 10.10 Yosemite (64-bit)",
      VBOXOSTYPE_MacOS1010_x64,   VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_EFI | VBOXOSHINT_PAE |  VBOXOSHINT_64BIT
                                | VBOXOSHINT_USBHID | VBOXOSHINT_HPET | VBOXOSHINT_RTCUTC | VBOXOSHINT_USBTABLET,
      1, 2048,  16, 25 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS1011_64",       "Mac OS X 10.11 El Capitan (64-bit)",
      VBOXOSTYPE_MacOS1011_x64,   VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_EFI | VBOXOSHINT_PAE |  VBOXOSHINT_64BIT
                                | VBOXOSHINT_USBHID | VBOXOSHINT_HPET | VBOXOSHINT_RTCUTC | VBOXOSHINT_USBTABLET,
      1, 2048,  16, 30 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS1012_64",       "macOS 10.12 Sierra (64-bit)",
      VBOXOSTYPE_MacOS1012_x64,   VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_EFI | VBOXOSHINT_PAE |  VBOXOSHINT_64BIT
                                | VBOXOSHINT_USBHID | VBOXOSHINT_HPET | VBOXOSHINT_RTCUTC | VBOXOSHINT_USBTABLET,
      1, 2048,  16, 30 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS1013_64",       "macOS 10.13 High Sierra (64-bit)",
      VBOXOSTYPE_MacOS1013_x64,   VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_EFI | VBOXOSHINT_PAE |  VBOXOSHINT_64BIT
                                | VBOXOSHINT_USBHID | VBOXOSHINT_HPET | VBOXOSHINT_RTCUTC | VBOXOSHINT_USBTABLET,
      1, 2048,  16, 30 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, IommuType_None, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Other",   "Other",             "DOS",                "DOS",
      VBOXOSTYPE_DOS,             VBOXOSHINT_FLOPPY | VBOXOSHINT_NOUSB,
      1,   32,   4,  500 * _1M, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "Other",   "Other",             "Netware",            "Netware",
      VBOXOSTYPE_Netware,         VBOXOSHINT_HWVIRTEX | VBOXOSHINT_FLOPPY | VBOXOSHINT_NOUSB,
      1,  512,   4,  4 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Other",   "Other",             "L4",                 "L4",
      VBOXOSTYPE_L4,              VBOXOSHINT_NONE,
      1,   64,   4,  2 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Other",   "Other",             "QNX",                "QNX",
      VBOXOSTYPE_QNX,             VBOXOSHINT_HWVIRTEX,
      1,  512,   4,  4 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Other",   "Other",             "JRockitVE",          "JRockitVE",
      VBOXOSTYPE_JRockitVE,       VBOXOSHINT_HWVIRTEX | VBOXOSHINT_IOAPIC | VBOXOSHINT_PAE,
      1, 1024,   4,  8 * _1G64, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_BusLogic, StorageBus_SCSI, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Other",   "Other",             "VBoxBS_64",          "VirtualBox Bootsector Test (64-bit)",
      VBOXOSTYPE_VBoxBS_x64,      VBOXOSHINT_HWVIRTEX | VBOXOSHINT_FLOPPY | VBOXOSHINT_IOAPIC | VBOXOSHINT_PAE | VBOXOSHINT_64BIT,
      1,  128,   4,  0, GraphicsControllerType_VBoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, IommuType_None, AudioControllerType_AC97, AudioCodecType_STAC9700  },
};

size_t Global::cOSTypes = RT_ELEMENTS(Global::sOSTypes);

/**
 * Returns an OS Type ID for the given VBOXOSTYPE value.
 *
 * The returned ID will correspond to the IGuestOSType::id value of one of the
 * objects stored in the IVirtualBox::guestOSTypes
 * (VirtualBoxImpl::COMGETTER(GuestOSTypes)) collection.
 */
/* static */
const char *Global::OSTypeId(VBOXOSTYPE aOSType)
{
    for (size_t i = 0; i < RT_ELEMENTS(sOSTypes); ++i)
    {
        if (sOSTypes[i].osType == aOSType)
            return sOSTypes[i].id;
    }

    return sOSTypes[0].id;
}

/**
 * Maps an OS type ID string to index into sOSTypes.
 *
 * @returns index on success, UINT32_MAX if not found.
 * @param   pszId       The OS type ID string.
 */
/* static */ uint32_t Global::getOSTypeIndexFromId(const char *pszId)
{
    for (size_t i = 0; i < RT_ELEMENTS(sOSTypes); ++i)
        if (!RTStrICmp(pszId, Global::sOSTypes[i].id))
            return (uint32_t)i;
    return UINT32_MAX;
}

/*static*/ uint32_t Global::getMaxNetworkAdapters(ChipsetType_T aChipsetType)
{
    switch (aChipsetType)
    {
        case ChipsetType_PIIX3:
            return 8;
        case ChipsetType_ICH9:
            return 36;
        default:
            return 0;
    }
}

/*static*/ const char *
Global::stringifyMachineState(MachineState_T aState)
{
    switch (aState)
    {
        case MachineState_Null:                 return GlobalCtx::tr("Null");
        case MachineState_PoweredOff:           return GlobalCtx::tr("PoweredOff");
        case MachineState_Saved:                return GlobalCtx::tr("Saved");
        case MachineState_Teleported:           return GlobalCtx::tr("Teleported");
        case MachineState_Aborted:              return GlobalCtx::tr("Aborted");
        case MachineState_AbortedSaved:         return GlobalCtx::tr("Aborted-Saved");
        case MachineState_Running:              return GlobalCtx::tr("Running");
        case MachineState_Paused:               return GlobalCtx::tr("Paused");
        case MachineState_Stuck:                return GlobalCtx::tr("GuruMeditation");
        case MachineState_Teleporting:          return GlobalCtx::tr("Teleporting");
        case MachineState_LiveSnapshotting:     return GlobalCtx::tr("LiveSnapshotting");
        case MachineState_Starting:             return GlobalCtx::tr("Starting");
        case MachineState_Stopping:             return GlobalCtx::tr("Stopping");
        case MachineState_Saving:               return GlobalCtx::tr("Saving");
        case MachineState_Restoring:            return GlobalCtx::tr("Restoring");
        case MachineState_TeleportingPausedVM:  return GlobalCtx::tr("TeleportingPausedVM");
        case MachineState_TeleportingIn:        return GlobalCtx::tr("TeleportingIn");
        case MachineState_DeletingSnapshotOnline: return GlobalCtx::tr("DeletingSnapshotOnline");
        case MachineState_DeletingSnapshotPaused: return GlobalCtx::tr("DeletingSnapshotPaused");
        case MachineState_OnlineSnapshotting:   return GlobalCtx::tr("OnlineSnapshotting");
        case MachineState_RestoringSnapshot:    return GlobalCtx::tr("RestoringSnapshot");
        case MachineState_DeletingSnapshot:     return GlobalCtx::tr("DeletingSnapshot");
        case MachineState_SettingUp:            return GlobalCtx::tr("SettingUp");
        case MachineState_Snapshotting:         return GlobalCtx::tr("Snapshotting");
        default:
            AssertMsgFailedReturn(("%d (%#x)\n", aState, aState), ::stringifyMachineState(aState));
    }
}

/*static*/ const char *
Global::stringifySessionState(SessionState_T aState)
{
    switch (aState)
    {
        case SessionState_Null:         return GlobalCtx::tr("Null");
        case SessionState_Unlocked:     return GlobalCtx::tr("Unlocked");
        case SessionState_Locked:       return GlobalCtx::tr("Locked");
        case SessionState_Spawning:     return GlobalCtx::tr("Spawning");
        case SessionState_Unlocking:    return GlobalCtx::tr("Unlocking");
        default:
            AssertMsgFailedReturn(("%d (%#x)\n", aState, aState), ::stringifySessionState(aState));
    }
}

/*static*/ const char *
Global::stringifyStorageControllerType(StorageControllerType_T aType)
{
    switch (aType)
    {
        case StorageControllerType_Null:        return GlobalCtx::tr("Null");
        case StorageControllerType_LsiLogic:    return GlobalCtx::tr("LsiLogic");
        case StorageControllerType_BusLogic:    return GlobalCtx::tr("BusLogic");
        case StorageControllerType_IntelAhci:   return GlobalCtx::tr("AHCI");
        case StorageControllerType_PIIX3:       return GlobalCtx::tr("PIIX3");
        case StorageControllerType_PIIX4 :      return GlobalCtx::tr("PIIX4");
        case StorageControllerType_ICH6:        return GlobalCtx::tr("ICH6");
        case StorageControllerType_I82078:      return GlobalCtx::tr("I82078");
        case StorageControllerType_LsiLogicSas: return GlobalCtx::tr("LsiLogicSas");
        case StorageControllerType_USB:         return GlobalCtx::tr("USB");
        case StorageControllerType_NVMe:        return GlobalCtx::tr("NVMe");
        case StorageControllerType_VirtioSCSI:  return GlobalCtx::tr("VirtioSCSI");
        default:
            AssertMsgFailedReturn(("%d (%#x)\n", aType, aType), ::stringifyStorageControllerType(aType));
    }
}

/*static*/ const char *
Global::stringifyDeviceType(DeviceType_T aType)
{
    switch (aType)
    {
        case DeviceType_Null:         return GlobalCtx::tr("Null");
        case DeviceType_Floppy:       return GlobalCtx::tr("Floppy");
        case DeviceType_DVD:          return GlobalCtx::tr("DVD");
        case DeviceType_HardDisk:     return GlobalCtx::tr("HardDisk");
        case DeviceType_Network:      return GlobalCtx::tr("Network");
        case DeviceType_USB:          return GlobalCtx::tr("USB");
        case DeviceType_SharedFolder: return GlobalCtx::tr("ShardFolder");
        default:
            AssertMsgFailedReturn(("%d (%#x)\n", aType, aType), ::stringifyDeviceType(aType));
    }
}

#if 0 /* unused */

/*static*/ const char *
Global::stringifyStorageBus(StorageBus_T aBus)
{
    switch (aBus)
    {
        case StorageBus_Null:           return GlobalCtx::tr("Null");
        case StorageBus_IDE:            return GlobalCtx::tr("IDE");
        case StorageBus_SATA:           return GlobalCtx::tr("SATA");
        case StorageBus_Floppy:         return GlobalCtx::tr("Floppy");
        case StorageBus_SAS:            return GlobalCtx::tr("SAS");
        case StorageBus_USB:            return GlobalCtx::tr("USB");
        case StorageBus_PCIe:           return GlobalCtx::tr("PCIe");
        case StorageBus_VirtioSCSI:     return GlobalCtx::tr("VirtioSCSI");
        default:
            AssertMsgFailedReturn(("%d (%#x)\n", aBus, aBus), ::stringifyStorageBus(aBus));
    }
}

/*static*/ const char *
Global::stringifyReason(Reason_T aReason)
{
    switch (aReason)
    {
        case Reason_Unspecified:      return GlobalCtx::tr("unspecified");
        case Reason_HostSuspend:      return GlobalCtx::tr("host suspend");
        case Reason_HostResume:       return GlobalCtx::tr("host resume");
        case Reason_HostBatteryLow:   return GlobalCtx::tr("host battery low");
        case Reason_Snapshot:         return GlobalCtx::tr("snapshot");
        default:
            AssertMsgFailedReturn(("%d (%#x)\n", aReason, aReason), ::stringifyReason(aReason));
    }
}

#endif /* unused */

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
