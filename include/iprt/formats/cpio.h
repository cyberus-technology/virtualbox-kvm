/** @file
 * IPRT - CPIO archive format.
 */

/*
 * Copyright (C) 2020-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_formats_cpio_h
#define IPRT_INCLUDED_formats_cpio_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#include <iprt/assertcompile.h>


/** @defgroup grp_rt_formats_cpio   CPIO Archive format
 * @ingroup grp_rt_formats
 *
 * @{ */

/** This denotes the end of the archive (record with this filename, zero size and
 * a zero mode). */
#define CPIO_EOS_FILE_NAME                          "TRAILER!!!"


/**
 * The old binary header.
 */
typedef struct CPIOHDRBIN
{
    /** 0x00: Magic identifying the old header. */
    uint16_t                    u16Magic;
    /** 0x02: Device number. */
    uint16_t                    u16Dev;
    /** 0x04: Inode number. */
    uint16_t                    u16Inode;
    /** 0x06: Mode. */
    uint16_t                    u16Mode;
    /** 0x08: User ID. */
    uint16_t                    u16Uid;
    /** 0x0a: Group ID. */
    uint16_t                    u16Gid;
    /** 0x0c: Number of links to this file. */
    uint16_t                    u16NLinks;
    /** 0x0e: Associated device number for block and character device entries. */
    uint16_t                    u16RDev;
    /** 0x10: Modification time stored as two independent 16bit integers. */
    uint16_t                    au16MTime[2];
    /** 0x14: Number of bytes in the path name (including zero terminator) following the header. */
    uint16_t                    u16NameSize;
    /** 0x16: Size of the file stored as two independent 16bit integers. */
    uint16_t                    au16FileSize[2];
} CPIOHDRBIN;
AssertCompileSize(CPIOHDRBIN, 13 * 2);
typedef CPIOHDRBIN *PCPIOHDRBIN;
typedef const CPIOHDRBIN *PCCPIOHDRBIN;


/** The magic for the binary header. */
#define CPIO_HDR_BIN_MAGIC                          UINT16_C(070707)


/**
 * Portable ASCII format header as defined by SUSv2.
 */
typedef struct CPIOHDRSUSV2
{
    /** 0x00: Magic identifying the header. */
    char                        achMagic[6];
    /** 0x06: Device number. */
    char                        achDev[6];
    /** 0x0c: Inode number. */
    char                        achInode[6];
    /** 0x12: Mode. */
    char                        achMode[6];
    /** 0x18: User ID. */
    char                        achUid[6];
    /** 0x1e: Group ID. */
    char                        achGid[6];
    /** 0x24: Number of links to this file. */
    char                        achNLinks[6];
    /** 0x2a: Associated device number for block and character device entries. */
    char                        achRDev[6];
    /** 0x30: Modification time stored as two independent 16bit integers. */
    char                        achMTime[11];
    /** 0x36: Number of bytes in the path name (including zero terminator) following the header. */
    char                        achNameSize[6];
    /** 0x3c: Size of the file stored as two independent 16bit integers. */
    char                        achFileSize[11];
} CPIOHDRSUSV2;
AssertCompileSize(CPIOHDRSUSV2, 9 * 6 + 2 * 11);
typedef CPIOHDRSUSV2 *PCPIOHDRSUSV2;
typedef const CPIOHDRSUSV2 *PCCPIOHDRSUSV2;


/** The magic for the SuSv2 CPIO header. */
#define CPIO_HDR_SUSV2_MAGIC                        "070707"


/**
 * New ASCII format header.
 */
typedef struct CPIOHDRNEW
{
    /** 0x00: Magic identifying the header. */
    char                        achMagic[6];
    /** 0x06: Inode number. */
    char                        achInode[8];
    /** 0x0e: Mode. */
    char                        achMode[8];
    /** 0x16: User ID. */
    char                        achUid[8];
    /** 0x1e: Group ID. */
    char                        achGid[8];
    /** 0x26: Number of links to this file. */
    char                        achNLinks[8];
    /** 0x2e: Modification time. */
    char                        achMTime[8];
    /** 0x36: Size of the file stored as two independent 16bit integers. */
    char                        achFileSize[8];
    /** 0x3e: Device major number. */
    char                        achDevMajor[8];
    /** 0x46: Device minor number. */
    char                        achDevMinor[8];
    /** 0x4e: Assigned device major number for block or character device files. */
    char                        achRDevMajor[8];
    /** 0x56: Assigned device minor number for block or character device files. */
    char                        achRDevMinor[8];
    /** 0x5e: Number of bytes in the path name (including zero terminator) following the header. */
    char                        achNameSize[8];
    /** 0x66: Checksum of the file data if used. */
    char                        achCheck[8];
} CPIOHDRNEW;
AssertCompileSize(CPIOHDRNEW, 6 + 13 * 8);
AssertCompileMemberOffset(CPIOHDRNEW, achMagic,     0x00);
AssertCompileMemberOffset(CPIOHDRNEW, achInode,     0x06);
AssertCompileMemberOffset(CPIOHDRNEW, achMode,      0x0e);
AssertCompileMemberOffset(CPIOHDRNEW, achUid,       0x16);
AssertCompileMemberOffset(CPIOHDRNEW, achGid,       0x1e);
AssertCompileMemberOffset(CPIOHDRNEW, achNLinks,    0x26);
AssertCompileMemberOffset(CPIOHDRNEW, achMTime,     0x2e);
AssertCompileMemberOffset(CPIOHDRNEW, achFileSize,  0x36);
AssertCompileMemberOffset(CPIOHDRNEW, achDevMajor,  0x3e);
AssertCompileMemberOffset(CPIOHDRNEW, achDevMinor,  0x46);
AssertCompileMemberOffset(CPIOHDRNEW, achRDevMajor, 0x4e);
AssertCompileMemberOffset(CPIOHDRNEW, achRDevMinor, 0x56);
AssertCompileMemberOffset(CPIOHDRNEW, achNameSize,  0x5e);
AssertCompileMemberOffset(CPIOHDRNEW, achCheck,     0x66);
typedef CPIOHDRNEW *PCPIOHDRNEW;
typedef const CPIOHDRNEW *PCCPIOHDRNEW;


/** The magic for the new ASCII CPIO header. */
#define CPIO_HDR_NEW_MAGIC                          "070701"
/** The magic for the new ASCII CPIO header + checksum. */
#define CPIO_HDR_NEW_CHKSUM_MAGIC                   "070702"


/**
 * CPIO header union.
 */
typedef union CPIOHDR
{
    /** byte view. */
    uint8_t                     ab[110];
    /** The ancient binary header. */
    CPIOHDRBIN                  AncientBin;
    /** The SuSv2 ASCII header. */
    CPIOHDRSUSV2                AsciiSuSv2;
    /** The new ASCII header format. */
    CPIOHDRNEW                  AsciiNew;
} CPIOHDR;
typedef CPIOHDR *PCPIOHDR;
typedef const CPIOHDR *PCCPIOHDR;


/** @} */

#endif /* !IPRT_INCLUDED_formats_cpio_h */

