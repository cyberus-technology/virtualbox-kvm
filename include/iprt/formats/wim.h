/** @file
 * IPRT - Windows Imaging (WIM) format.
 */

/*
 * Copyright (C) 2022-2023 Oracle and/or its affiliates.
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_formats_wim_h
#define IPRT_INCLUDED_formats_wim_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#include <iprt/assertcompile.h>
#include <iprt/uuid.h>


/** @defgroup grp_rt_formats_win   Windows Imaging (WIM) format
 * @ingroup grp_rt_formats
 *
 * Specification:
 * http://download.microsoft.com/download/f/e/f/fefdc36e-392d-4678-9e4e-771ffa2692ab/Windows%20Imaging%20File%20Format.rtf
 *
 * @{ */


/**
 * A short WIM resource entry.
 *
 * This is a simplified version of the specs.
 */
typedef struct RESHDRDISKSHORT
{
    /** 0x00 - The compressed size. */
    RT_GCC_EXTENSION
    uint64_t        cb : 56;
    /** 0x07 - Flags, RESHDR_FLAGS_XXX.  */
    RT_GCC_EXTENSION
    uint64_t        bFlags : 8;
    /** 0x08 - Offset.
     * @note This is signed in the specficiation...  */
    uint64_t        off;
    /** 0x10 - The uncompressed original size.
     * @note This is signed in the specficiation...  */
    uint64_t        cbOriginal;
} RESHDRDISKSHORT;
AssertCompileSize(RESHDRDISKSHORT, 0x18);
/** Pointer to a short WIM resource entry. */
typedef RESHDRDISKSHORT *PRESHDRDISKSHORT;
/** Pointer to a const short WIM resource entry. */
typedef RESHDRDISKSHORT *PCRESHDRDISKSHORT;

/** @name RESHDR_FLAGS_XXX
 * @{ */
#define RESHDR_FLAGS_FREE           UINT8_C(0x01)
#define RESHDR_FLAGS_METADATA       UINT8_C(0x02)
#define RESHDR_FLAGS_COMPRESSED     UINT8_C(0x04)
#define RESHDR_FLAGS_SPANNED        UINT8_C(0x08)
/** @} */

/**
 * WIM file header, version 1.
 *
 * The field names have been normalized to our coding style.
 */
#pragma pack(4)
typedef struct WIMHEADERV1
{
    /** 0x00 - Magic value WIMHEADER_MAGIC. */
    char            szMagic[8];
    /** 0x08 - The size of this header structure. */
    uint32_t        cbHeader;
    /** 0x0c - The header version structure. */
    uint32_t        uVersion;
    /** 0x10 - Flags. */
    uint32_t        fFlags;
    /** 0x14 - ??. */
    uint32_t        cbCompression;
    /** 0x18 - Unique identifier. */
    RTUUID          WIMGuid;
    /** 0x28 - Part number in spanned (split) wim set. Unsplit use part number 1. */
    uint16_t        idxPartNumber;
    /** 0x2a - Total number of parts in spanned set. */
    uint16_t        cTotalParts;
    /** 0x2c - Number of images in the archive. */
    uint32_t        cImages;
    /** 0x30 - Resource lookup table offset & size. */
    RESHDRDISKSHORT OffsetTable;
    /** 0x48 - XML data offset & size.   */
    RESHDRDISKSHORT XmlData;
    /** 0x60 - Boot metadata offset & size.  */
    RESHDRDISKSHORT BootMetadata;
    /** 0x78 - Bootable image index, zero if no bootable image.   */
    uint32_t        idxBoot;
    /** 0x7c - Integrity data offset & size.
     *  @note Misaligned! */
    RESHDRDISKSHORT Integrity;
    /** 0x94 - Reserved   */
    uint8_t         abUnused[60];
} WIMHEADERV1;
#pragma pack()
AssertCompileSize(WIMHEADERV1, 0xd0);
/** Pointer to a XAR header. */
typedef WIMHEADERV1 *PWIMHEADERV1;
/** Pointer to a const XAR header. */
typedef WIMHEADERV1 const *PCWIMHEADERV1;

/** The WIMHEADERV1::szMagic value. */
#define WIMHEADER_MAGIC "MSWIM\0\0"
AssertCompile(sizeof(WIMHEADER_MAGIC) == 8);

/** @name WIMHEADER_FLAGS_XXX - WINHEADERV1::fFlags.
 * @note Specfication names these FLAG_HEADER_XXX.
 * @{  */
#define WIMHEADER_FLAGS_RESERVED            RT_BIT_32(0)
#define WIMHEADER_FLAGS_COMPRESSION         RT_BIT_32(1)
#define WIMHEADER_FLAGS_READONLY            RT_BIT_32(2)
#define WIMHEADER_FLAGS_SPANNED             RT_BIT_32(3)
#define WIMHEADER_FLAGS_RESOURCE_ONLY       RT_BIT_32(4)
#define WIMHEADER_FLAGS_METADATA_ONLY       RT_BIT_32(5)
#define WIMHEADER_FLAGS_WRITE_IN_PROGRESS   RT_BIT_32(5)
#define WIMHEADER_FLAGS_RP_FIX              RT_BIT_32(6)
#define WIMHEADER_FLAGS_COMPRESS_RESERVED   RT_BIT_32(16)
#define WIMHEADER_FLAGS_COMPRESS_XPRESS     RT_BIT_32(17)
#define WIMHEADER_FLAGS_COMPRESS_LZX        RT_BIT_32(18)
/** @} */

/** @} */

#endif /* !IPRT_INCLUDED_formats_wim_h */

