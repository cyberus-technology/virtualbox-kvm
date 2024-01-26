/** @file
 * IPRT - Filesystem, VFS implementations.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_fsvfs_h
#define IPRT_INCLUDED_fsvfs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_fs_vfs  VFS File System Implementations
 * @ingroup grp_rt_fs
 * @{
 */

/**
 * Opens a FAT file system volume.
 *
 * @returns IPRT status code.
 * @param   hVfsFileIn      The file or device backing the volume.
 * @param   fReadOnly       Whether to mount it read-only.
 * @param   offBootSector   The offset of the boot sector relative to the start
 *                          of @a hVfsFileIn.  Pass 0 for floppies.
 * @param   phVfs           Where to return the virtual file system handle.
 * @param   pErrInfo        Where to return additional error information.
 */
RTDECL(int) RTFsFatVolOpen(RTVFSFILE hVfsFileIn, bool fReadOnly, uint64_t offBootSector, PRTVFS phVfs, PRTERRINFO pErrInfo);


/**
 * FAT type (format).
 */
typedef enum RTFSFATTYPE
{
    RTFSFATTYPE_INVALID = 0,
    RTFSFATTYPE_FAT12,
    RTFSFATTYPE_FAT16,
    RTFSFATTYPE_FAT32,
    RTFSFATTYPE_END
} RTFSFATTYPE;


/** @name RTFSFATVOL_FMT_F_XXX - RTFsFatVolFormat flags
 * @{ */
/** Perform a full format, filling unused sectors with 0xf6. */
#define RTFSFATVOL_FMT_F_FULL           UINT32_C(0)
/** Perform a quick format.
 * I.e. just write the boot sector, FATs and root directory.  */
#define RTFSFATVOL_FMT_F_QUICK          RT_BIT_32(0)
/** Mask containing all valid flags.   */
#define RTFSFATVOL_FMT_F_VALID_MASK     UINT32_C(0x00000001)
/** @} */

/**
 * Formats a FAT volume.
 *
 * @returns IRPT status code.
 * @param   hVfsFile            The volume file.
 * @param   offVol              The offset into @a hVfsFile of the file.
 *                              Typically 0.
 * @param   cbVol               The size of the volume.  Pass 0 if the rest of
 *                              hVfsFile should be used.
 * @param   fFlags              See RTFSFATVOL_FMT_F_XXX.
 * @param   cbSector            The logical sector size.  Must be power of two.
 *                              Optional, pass zero to use 512.
 * @param   cSectorsPerCluster  Number of sectors per cluster.  Power of two.
 *                              Optional, pass zero to auto detect.
 * @param   enmFatType          The FAT type (12, 16, 32) to use.
 *                              Optional, pass RTFSFATTYPE_INVALID for default.
 * @param   cHeads              The number of heads to report in the BPB.
 *                              Optional, pass zero to auto detect.
 * @param   cSectorsPerTrack    The number of sectors per track to put in the
 *                              BPB. Optional, pass zero to auto detect.
 * @param   bMedia              The media byte value and FAT ID to use.
 *                              Optional, pass zero to auto detect.
 * @param   cRootDirEntries     Number of root directory entries.
 *                              Optional, pass zero to auto detect.
 * @param   cHiddenSectors      Number of hidden sectors.  Pass 0 for
 *                              unpartitioned media.
 * @param   pErrInfo            Additional error information, maybe.  Optional.
 */
RTDECL(int) RTFsFatVolFormat(RTVFSFILE hVfsFile, uint64_t offVol, uint64_t cbVol, uint32_t fFlags, uint16_t cbSector,
                             uint16_t cSectorsPerCluster, RTFSFATTYPE enmFatType, uint32_t cHeads, uint32_t cSectorsPerTrack,
                             uint8_t bMedia, uint16_t cRootDirEntries, uint32_t cHiddenSectors, PRTERRINFO pErrInfo);

/**
 * Formats a 1.44MB floppy image.
 *
 * @returns IPRT status code.
 * @param   hVfsFile            The image.  Will be grown to 1.44MB if
 *                              necessary.
 * @param   fQuick              Whether to quick format the floppy or not.
 */
RTDECL(int) RTFsFatVolFormat144(RTVFSFILE hVfsFile, bool fQuick);

/**
 * Formats a 2.88MB floppy image.
 *
 * @returns IPRT status code.
 * @param   hVfsFile            The image.  Will be grown to 1.44MB if
 *                              necessary.
 * @param   fQuick              Whether to quick format the floppy or not.
 */
RTDECL(int) RTFsFatVolFormat288(RTVFSFILE hVfsFile, bool fQuick);


/**
 * Opens an EXT2/3/4 file system volume.
 *
 * @returns IPRT status code.
 * @param   hVfsFileIn      The file or device backing the volume.
 * @param   fMntFlags       RTVFSMNT_F_XXX.
 * @param   fExtFlags       Reserved, MBZ.
 * @param   phVfs           Where to return the virtual file system handle.
 * @param   pErrInfo        Where to return additional error information.
 */
RTDECL(int) RTFsExtVolOpen(RTVFSFILE hVfsFileIn, uint32_t fMntFlags, uint32_t fExtFlags, PRTVFS phVfs, PRTERRINFO pErrInfo);



/** @name RTFSISO9660_F_XXX - ISO 9660 mount flags.
 * @{ */
/** Do not use the UDF part if present. */
#define RTFSISO9660_F_NO_UDF        RT_BIT_32(0)
/** Do not use the joliet part. */
#define RTFSISO9660_F_NO_JOLIET     RT_BIT_32(1)
/** Do not use the rock ridge extensions if present. */
#define RTFSISO9660_F_NO_ROCK       RT_BIT_32(2)
/** Valid ISO 9660 mount option mask.   */
#define RTFSISO9660_F_VALID_MASK    UINT32_C(0x00000007)
/** Checks if @a a_fNoType is the only acceptable volume type. */
#define RTFSISO9660_F_IS_ONLY_TYPE(a_fFlags, a_fNoType) \
    (   ((a_fFlags)   & (RTFSISO9660_F_NO_UDF | RTFSISO9660_F_NO_JOLIET | RTFSISO9660_F_NO_ROCK)) \
     == (~(a_fNoType) & (RTFSISO9660_F_NO_UDF | RTFSISO9660_F_NO_JOLIET | RTFSISO9660_F_NO_ROCK)) )
/** @}  */

/**
 * Opens an ISO 9660 file system volume.
 *
 * @returns IPRT status code.
 * @param   hVfsFileIn      The file or device backing the volume.
 * @param   fFlags          RTFSISO9660_F_XXX.
 * @param   phVfs           Where to return the virtual file system handle.
 * @param   pErrInfo        Where to return additional error information.
 */
RTDECL(int) RTFsIso9660VolOpen(RTVFSFILE hVfsFileIn, uint32_t fFlags, PRTVFS phVfs, PRTERRINFO pErrInfo);


/**
 * Opens an NTFS file system volume.
 *
 * @returns IPRT status code.
 * @param   hVfsFileIn      The file or device backing the volume.
 * @param   fMntFlags       RTVFSMNT_F_XXX.
 * @param   fNtfsFlags      Reserved, MBZ.
 * @param   phVfs           Where to return the virtual file system handle.
 * @param   pErrInfo        Where to return additional error information.
 */
RTDECL(int) RTFsNtfsVolOpen(RTVFSFILE hVfsFileIn, uint32_t fMntFlags, uint32_t fNtfsFlags, PRTVFS phVfs, PRTERRINFO pErrInfo);


/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_fsvfs_h */

