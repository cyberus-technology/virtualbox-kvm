/** @file
 * IPRT Disk Volume Management API (DVM).
 */

/*
 * Copyright (C) 2011-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_dvm_h
#define IPRT_INCLUDED_dvm_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

RT_C_DECLS_BEGIN


/** @defgroup grp_rt_dvm           IPRT Disk Volume Management
 * @ingroup grp_rt
 * @{
 */

/**
 * Volume type.
 * Comparable to the FS type in MBR partition maps
 * or the partition type GUIDs in GPT tables.
 */
typedef enum RTDVMVOLTYPE
{
    /** Invalid. */
    RTDVMVOLTYPE_INVALID = 0,
    /** Unknown. */
    RTDVMVOLTYPE_UNKNOWN,
    /** Volume hosts a NTFS filesystem. */
    RTDVMVOLTYPE_NTFS,
    /** Volume hosts a FAT12 filesystem. */
    RTDVMVOLTYPE_FAT12,
    /** Volume hosts a FAT16 filesystem. */
    RTDVMVOLTYPE_FAT16,
    /** Volume hosts a FAT32 filesystem. */
    RTDVMVOLTYPE_FAT32,

    /** EFI system partition (c12a7328-f81f-11d2-ba4b-00a0c93ec93b). */
    RTDVMVOLTYPE_EFI_SYSTEM,

    /** Volume hosts a Mac OS X HFS or HFS+ filesystem. */
    RTDVMVOLTYPE_DARWIN_HFS,
    /** Volume hosts a Mac OS X APFS filesystem. */
    RTDVMVOLTYPE_DARWIN_APFS,

    /** Volume hosts a Linux swap. */
    RTDVMVOLTYPE_LINUX_SWAP,
    /** Volume hosts a Linux filesystem. */
    RTDVMVOLTYPE_LINUX_NATIVE,
    /** Volume hosts a Linux LVM. */
    RTDVMVOLTYPE_LINUX_LVM,
    /** Volume hosts a Linux SoftRaid. */
    RTDVMVOLTYPE_LINUX_SOFTRAID,

    /** Volume hosts a FreeBSD disklabel. */
    RTDVMVOLTYPE_FREEBSD,
    /** Volume hosts a NetBSD disklabel. */
    RTDVMVOLTYPE_NETBSD,
    /** Volume hosts a OpenBSD disklabel. */
    RTDVMVOLTYPE_OPENBSD,
    /** Volume hosts a Solaris volume. */
    RTDVMVOLTYPE_SOLARIS,

    /** Volume hosts a Windows basic data partition . */
    RTDVMVOLTYPE_WIN_BASIC,
    /** Volume hosts a Microsoft reserved partition (MSR). */
    RTDVMVOLTYPE_WIN_MSR,
    /** Volume hosts a Windows logical disk manager (LDM) metadata partition. */
    RTDVMVOLTYPE_WIN_LDM_META,
    /** Volume hosts a Windows logical disk manager (LDM) data partition. */
    RTDVMVOLTYPE_WIN_LDM_DATA,
    /** Volume hosts a Windows recovery partition. */
    RTDVMVOLTYPE_WIN_RECOVERY,
    /** Volume hosts a storage spaces partition. */
    RTDVMVOLTYPE_WIN_STORAGE_SPACES,

    /** Volume hosts an IBM general parallel file system (GPFS). */
    RTDVMVOLTYPE_IBM_GPFS,

    /** OS/2 (Arca Noae) type 1 partition. */
    RTDVMVOLTYPE_ARCA_OS2,

    /** End of the valid values. */
    RTDVMVOLTYPE_END,
    /** Usual 32bit hack. */
    RTDVMVOLTYPE_32BIT_HACK = 0x7fffffff
} RTDVMVOLTYPE;

/** @defgroup grp_dvm_flags     Flags used by RTDvmCreate.
 * @{ */
/** DVM flags - Blocks are always marked as unused if the volume has
 *              no block status callback set.
 *              The default is to mark them as used. */
#define DVM_FLAGS_NO_STATUS_CALLBACK_MARK_AS_UNUSED     RT_BIT_32(0)
/** DVM flags - Space which is unused in the map will be marked as used
 *              when calling RTDvmMapQueryBlockStatus(). */
#define DVM_FLAGS_UNUSED_SPACE_MARK_AS_USED             RT_BIT_32(1)
/** Mask of all valid flags. */
#define DVM_FLAGS_VALID_MASK                            UINT32_C(0x00000003)
/** @}  */


/** @defgroup grp_dvm_vol_flags     Volume flags used by RTDvmVolumeGetFlags().
 * @{ */
/** Volume flags - Volume is bootable. */
#define DVMVOLUME_FLAGS_BOOTABLE    RT_BIT_64(0)
/** Volume flags - Volume is active. */
#define DVMVOLUME_FLAGS_ACTIVE      RT_BIT_64(1)
/** Volume is contiguous on the underlying medium and RTDvmVolumeQueryRange(). */
#define DVMVOLUME_F_CONTIGUOUS      RT_BIT_64(2)
/** @}  */

/** A handle to a volume manager. */
typedef struct RTDVMINTERNAL       *RTDVM;
/** A pointer to a volume manager handle. */
typedef RTDVM                      *PRTDVM;
/** NIL volume manager handle. */
#define NIL_RTDVM                   ((RTDVM)~0)

/** A handle to a volume in a volume map. */
typedef struct RTDVMVOLUMEINTERNAL *RTDVMVOLUME;
/** A pointer to a volume handle. */
typedef RTDVMVOLUME                *PRTDVMVOLUME;
/** NIL volume handle. */
#define NIL_RTDVMVOLUME             ((RTDVMVOLUME)~0)

/**
 * Callback for querying the block allocation status of a volume.
 *
 * @returns IPRT status code.
 * @param   pvUser         Opaque user data passed when setting the callback.
 * @param   off            Offset relative to the start of the volume.
 * @param   cb             Range to check in bytes.
 * @param   pfAllocated    Where to store the allocation status on success.
 */
typedef DECLCALLBACKTYPE(int, FNDVMVOLUMEQUERYBLOCKSTATUS,(void *pvUser, uint64_t off, uint64_t cb, bool *pfAllocated));
/** Pointer to a query block allocation status callback. */
typedef FNDVMVOLUMEQUERYBLOCKSTATUS *PFNDVMVOLUMEQUERYBLOCKSTATUS;

/**
 * Create a new volume manager.
 *
 * @returns IPRT status.
 * @param   phVolMgr    Where to store the handle to the volume manager on
 *                      success.
 * @param   hVfsFile    The disk/container/whatever.
 * @param   cbSector    Size of one sector in bytes.
 * @param   fFlags      Combination of RTDVM_FLAGS_*
 */
RTDECL(int) RTDvmCreate(PRTDVM phVolMgr, RTVFSFILE hVfsFile, uint32_t cbSector, uint32_t fFlags);

/**
 * Retain a given volume manager.
 *
 * @returns New reference count on success, UINT32_MAX on failure.
 * @param   hVolMgr     The volume manager to retain.
 */
RTDECL(uint32_t) RTDvmRetain(RTDVM hVolMgr);

/**
 * Releases a given volume manager.
 *
 * @returns New reference count on success (0 if closed), UINT32_MAX on failure.
 * @param   hVolMgr     The volume manager to release.
 */
RTDECL(uint32_t) RTDvmRelease(RTDVM hVolMgr);

/**
 * Probes the underyling disk for the best volume manager format handler
 * and opens it.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_FOUND if no backend can handle the volume map on the disk.
 * @param   hVolMgr     The volume manager handle.
 */
RTDECL(int) RTDvmMapOpen(RTDVM hVolMgr);

/**
 * Initializes a new volume map using the given format handler.
 *
 * @returns IPRT status code.
 * @param   hVolMgr     The volume manager handle.
 * @param   pszFmt      The format to use for the new map.
 */
RTDECL(int) RTDvmMapInitialize(RTDVM hVolMgr, const char *pszFmt);

/**
 * Gets the name of the currently used format of the disk map.
 *
 * @returns Name of the format.
 * @param   hVolMgr     The volume manager handle.
 */
RTDECL(const char *) RTDvmMapGetFormatName(RTDVM hVolMgr);

/**
 * DVM format types.
 */
typedef enum RTDVMFORMATTYPE
{
    /** Invalid zero value. */
    RTDVMFORMATTYPE_INVALID = 0,
    /** Master boot record. */
    RTDVMFORMATTYPE_MBR,
    /** GUID partition table. */
    RTDVMFORMATTYPE_GPT,
    /** BSD labels. */
    RTDVMFORMATTYPE_BSD_LABEL,
    /** End of valid values. */
    RTDVMFORMATTYPE_END,
    /** 32-bit type size hack. */
    RTDVMFORMATTYPE_32BIT_HACK = 0x7fffffff
} RTDVMFORMATTYPE;

/**
 * Gets the format type of the current disk map.
 *
 * @returns Format type. RTDVMFORMATTYPE_INVALID on invalid input.
 * @param   hVolMgr     The volume manager handle.
 */
RTDECL(RTDVMFORMATTYPE) RTDvmMapGetFormatType(RTDVM hVolMgr);

/**
 * Gets the UUID of the disk if applicable.
 *
 * Disks using the MBR format may return the 32-bit disk identity in the
 * u32TimeLow field and set the rest to zero.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if the partition scheme doesn't do UUIDs.
 * @retval  VINF_NOT_SUPPORTED if non-UUID disk ID is returned.
 * @param   hVolMgr     The volume manager handle.
 * @param   pUuid       Where to return the UUID.
 *
 * @todo It's quite possible this should be turned into a map-level edition of
 *       RTDvmVolumeQueryProp...
 */
RTDECL(int) RTDvmMapQueryDiskUuid(RTDVM hVolMgr, PRTUUID pUuid);

/**
 * Gets the number of valid partitions in the map.
 *
 * @returns The number of valid volumes in the map or UINT32_MAX on failure.
 * @param   hVolMgr     The volume manager handle.
 */
RTDECL(uint32_t) RTDvmMapGetValidVolumes(RTDVM hVolMgr);

/**
 * Gets the maximum number of partitions the map can hold.
 *
 * @returns The maximum number of volumes in the map or UINT32_MAX on failure.
 * @param   hVolMgr         The volume manager handle.
 */
RTDECL(uint32_t) RTDvmMapGetMaxVolumes(RTDVM hVolMgr);

/**
 * Get the first valid volume from a map.
 *
 * @returns IPRT status code.
 * @param   hVolMgr         The volume manager handle.
 * @param   phVol           Where to store the handle to the first volume on
 *                          success. Release with RTDvmVolumeRelease().
 */
RTDECL(int) RTDvmMapQueryFirstVolume(RTDVM hVolMgr, PRTDVMVOLUME phVol);

/**
 * Get the first valid volume from a map.
 *
 * @returns IPRT status code.
 * @param   hVolMgr         The volume manager handle.
 * @param   hVol            Handle of the current volume.
 * @param   phVolNext       Where to store the handle to the next volume on
 *                          success. Release with RTDvmVolumeRelease().
 */
RTDECL(int) RTDvmMapQueryNextVolume(RTDVM hVolMgr, RTDVMVOLUME hVol, PRTDVMVOLUME phVolNext);

/**
 * Returns whether the given block on the disk is in use.
 *
 * @returns IPRT status code.
 * @param   hVolMgr         The volume manager handle.
 * @param   off             The start offset to check for.
 * @param   cb              The range in bytes to check.
 * @param   pfAllocated     Where to store the in-use status on success.
 *
 * @remark This method will return true even if a part of the range is not in use.
 */
RTDECL(int) RTDvmMapQueryBlockStatus(RTDVM hVolMgr, uint64_t off, uint64_t cb, bool *pfAllocated);

/**
 * Partition/map table location information.
 * @sa RTDvmMapQueryTableLocations
 */
typedef struct RTDVMTABLELOCATION
{
    /** The byte offset on the underlying media. */
    uint64_t    off;
    /** The table size in bytes. */
    uint64_t    cb;
    /** Number of padding bytes / free space between the actual table and
     *  first partition. */
    uint64_t    cbPadding;
} RTDVMTABLELOCATION;
/** Pointer to partition table location info. */
typedef RTDVMTABLELOCATION *PRTDVMTABLELOCATION;
/** Pointer to const partition table location info. */
typedef RTDVMTABLELOCATION const *PCRTDVMTABLELOCATION;


/** @name RTDVMMAPQTABLOC_F_XXX - Flags for RTDvmMapQueryTableLocations
 * @{ */
/** Make sure GPT includes the protective MBR. */
#define RTDVMMAPQTABLOC_F_INCLUDE_LEGACY    RT_BIT_32(0)
/** Valid flags.   */
#define RTDVMMAPQTABLOC_F_VALID_MASK        UINT32_C(1)
/** @} */

/**
 * Query the partition table locations.
 *
 * @returns IPRT status code.
 * @retval  VERR_BUFFER_OVERFLOW if the table is too small, @a *pcActual will be
 *          set to the required size.
 * @retval  VERR_BUFFER_UNDERFLOW if the table is too big and @a pcActual is
 *          NULL.
 * @param   hVolMgr         The volume manager handle.
 * @param   fFlags          Flags, see RTDVMMAPQTABLOC_F_XXX.
 * @param   paLocations     Where to return the info.  This can be NULL if @a
 *                          cLocations is zero and @a pcActual is given.
 * @param   cLocations      The size of @a paLocations in items.
 * @param   pcActual        Where to return the actual number of locations, or
 *                          on VERR_BUFFER_OVERFLOW the necessary table size.
 *                          Optional, when not specified the cLocations value
 *                          must match exactly or it fails with
 *                          VERR_BUFFER_UNDERFLOW.
 */
RTDECL(int) RTDvmMapQueryTableLocations(RTDVM hVolMgr, uint32_t fFlags,
                                        PRTDVMTABLELOCATION paLocations, size_t cLocations, size_t *pcActual);

/**
 * Retains a valid volume handle.
 *
 * @returns New reference count on success, UINT32_MAX on failure.
 * @param   hVol            The volume to retain.
 */
RTDECL(uint32_t) RTDvmVolumeRetain(RTDVMVOLUME hVol);

/**
 * Releases a valid volume handle.
 *
 * @returns New reference count on success (0 if closed), UINT32_MAX on failure.
 * @param   hVol            The volume to release.
 */
RTDECL(uint32_t) RTDvmVolumeRelease(RTDVMVOLUME hVol);

/**
 * Sets the callback to query the block allocation status for a volume.
 * This overwrites any other callback set previously.
 *
 * @param   hVol                   The volume handle.
 * @param   pfnQueryBlockStatus    The callback to set. Can be NULL to disable
 *                                 a previous callback.
 * @param   pvUser                 Opaque user data passed in the callback.
 */
RTDECL(void) RTDvmVolumeSetQueryBlockStatusCallback(RTDVMVOLUME hVol,
                                                    PFNDVMVOLUMEQUERYBLOCKSTATUS pfnQueryBlockStatus,
                                                    void *pvUser);

/**
 * Get the size of a volume in bytes.
 *
 * @returns Size of the volume in bytes or 0 on failure.
 * @param   hVol            The volume handle.
 */
RTDECL(uint64_t) RTDvmVolumeGetSize(RTDVMVOLUME hVol);

/**
 * Gets the name of the volume if supported.
 *
 * @returns IPRT status code.
 * @param   hVol            The volume handle.
 * @param   ppszVolName     Where to store the name of the volume on success.
 *                          The string must be freed with RTStrFree().
 */
RTDECL(int) RTDvmVolumeQueryName(RTDVMVOLUME hVol, char **ppszVolName);

/**
 * Get the volume type of the volume if supported.
 *
 * @returns The volume type on success, DVMVOLTYPE_INVALID if hVol is invalid.
 * @param   hVol            The volume handle.
 */
RTDECL(RTDVMVOLTYPE) RTDvmVolumeGetType(RTDVMVOLUME hVol);

/**
 * Get the volume flags of the volume if supported.
 *
 * @returns The volume flags or UINT64_MAX on failure.
 * @param   hVol            The volume handle.
 */
RTDECL(uint64_t) RTDvmVolumeGetFlags(RTDVMVOLUME hVol);

/**
 * Queries the range of the given volume on the underlying medium.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if the DVMVOLUME_F_CONTIGUOUS flag is not returned by RTDvmVolumeGetFlags().
 * @param   hVol            The volume handle.
 * @param   poffStart       Where to store the start offset in bytes on the underlying medium.
 * @param   poffLast        Where to store the last offset in bytes on the underlying medium (inclusive).
 */
RTDECL(int) RTDvmVolumeQueryRange(RTDVMVOLUME hVol, uint64_t *poffStart, uint64_t *poffLast);

/**
 * Returns the partition/whatever table location of the volume.
 *
 * For volume format with a single table, like GPT and BSD-labels, it will
 * return the location of that table.  Though for GPT, the fake MBR will not be
 * included.
 *
 * For logical (extended) MBR-style volumes, this will return the location of
 * the extended partition table.  For primary volumes the MBR location is
 * returned.  The special MBR case is why this operation is done on the volume
 * rather than the volume manager.
 *
 * Using RTDvmVolumeGetIndex with RTDVMVOLIDX_IN_PART_TABLE should get you
 * the index in the table returned by this function.
 *
 * @returns IPRT status code.
 * @param   hVol            The volume handle.
 * @param   poffTable       Where to return the byte offset on the underlying
 *                          media of the (partition/volume/whatever) table.
 * @param   pcbTable        Where to return the table size in bytes.  (This does
 *                          not include any alignment padding or such, just
 *                          padding up to sector/block size.)
 */
RTDECL(int) RTDvmVolumeQueryTableLocation(RTDVMVOLUME hVol, uint64_t *poffTable, uint64_t *pcbTable);

/**
 * RTDvmVolumeGetIndex indexes.
 */
typedef enum RTDVMVOLIDX
{
    /** Invalid zero value. */
    RTDVMVOLIDX_INVALID = 0,
    /** Index matching the host's volume numbering.
     * This is a pseudo index, that gets translated to one of the others depending
     * on which host we're running on. */
    RTDVMVOLIDX_HOST,
    /** Only consider user visible ones, i.e. don't count MBR extended partition
     *  entries and such like. */
    RTDVMVOLIDX_USER_VISIBLE,
    /** Index when all volumes, user visible, hidden, special, whatever ones are
     * included.
     *
     * For MBR this is 1-based index where all primary entires are included whether
     * in use or not.  Only non-empty entries in extended tables are counted, though
     * the forward link is included. */
    RTDVMVOLIDX_ALL,
    /** The raw index within the partition/volume/whatever table.  This have a kind
     *  of special meaning to MBR, where there are multiple tables. */
    RTDVMVOLIDX_IN_TABLE,
    /** Follows the linux /dev/sdaX convention as closely as absolutely possible. */
    RTDVMVOLIDX_LINUX,
    /** End of valid indexes. */
    RTDVMVOLIDX_END,
    /** Make sure the type is 32-bit.   */
    RTDVMVOLIDX_32BIT_HACK = 0x7fffffff
} RTDVMVOLIDX;

/**
 * Gets the tiven index for the specified volume.
 *
 * @returns The requested index, UINT32_MAX on failure.
 * @param   hVol            The volume handle.
 * @param   enmIndex        Which kind of index to get for the volume.
 */
RTDECL(uint32_t) RTDvmVolumeGetIndex(RTDVMVOLUME hVol, RTDVMVOLIDX enmIndex);

/**
 * Volume properties queriable via RTDvmVolumeQueryProp.
 *
 * @note Integer values can typically be queried in multiple sizes.  This is
 *       handled by the frontend code.  The format specific backends only
 *       have to handle the smallest allowed size.
 */
typedef enum RTDVMVOLPROP
{
    /** Customary invalid zero value. */
    RTDVMVOLPROP_INVALID = 0,
    /** unsigned[16,32,64]:     MBR first cylinder (0-based, CHS). */
    RTDVMVOLPROP_MBR_FIRST_CYLINDER,
    /** unsigned[8,16,32,64]:   MBR first head (0-based, CHS). */
    RTDVMVOLPROP_MBR_FIRST_HEAD,
    /** unsigned[8,16,32,64]:   MBR first sector (1-based, CHS). */
    RTDVMVOLPROP_MBR_FIRST_SECTOR,
    /** unsigned[16,32,64]:     MBR last cylinder (0-based, CHS). */
    RTDVMVOLPROP_MBR_LAST_CYLINDER,
    /** unsigned[8,16,32,64]:   MBR last head (0-based, CHS). */
    RTDVMVOLPROP_MBR_LAST_HEAD,
    /** unsigned[8,16,32,64]:   MBR last sector (1-based, CHS). */
    RTDVMVOLPROP_MBR_LAST_SECTOR,
    /** unsigned[8,16,32,64]:   MBR partition type. */
    RTDVMVOLPROP_MBR_TYPE,
    /** RTUUID:                 GPT volume type. */
    RTDVMVOLPROP_GPT_TYPE,
    /** RTUUID:                 GPT volume UUID. */
    RTDVMVOLPROP_GPT_UUID,
    /** End of valid values. */
    RTDVMVOLPROP_END,
    /** Make sure the type is 32-bit. */
    RTDVMVOLPROP_32BIT_HACK = 0x7fffffff
} RTDVMVOLPROP;

/**
 * Query a generic volume property.
 *
 * This is an extensible interface for retrieving mostly format specific
 * information, or information that's not commonly used.  (It's modeled after
 * RTLdrQueryPropEx.)
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if the property query isn't supported (either all
 *          or that specific property).  The caller  must  handle this result.
 * @retval  VERR_NOT_FOUND is currently not returned, but intended for cases
 *          where it wasn't present in the tables.
 * @retval  VERR_INVALID_FUNCTION if the @a enmProperty value is wrong.
 * @retval  VERR_INVALID_PARAMETER if the fixed buffer size is wrong. Correct
 *          size in @a *pcbBuf.
 * @retval  VERR_BUFFER_OVERFLOW if the property doesn't have a fixed size
 *          buffer and the buffer isn't big enough. Correct size in @a *pcbBuf.
 * @retval  VERR_INVALID_HANDLE if the handle is invalid.
 * @param   hVol        Handle to the volume.
 * @param   enmProperty The property to query.
 * @param   pvBuf       Pointer to the input / output buffer.  In most cases
 *                      it's only used for returning data.
 * @param   cbBuf       The size of the buffer.
 * @param   pcbBuf      Where to return the amount of data returned.  On
 *                      buffer size errors, this is set to the correct size.
 *                      Optional.
 * @sa      RTDvmVolumeGetPropU64
 */
RTDECL(int) RTDvmVolumeQueryProp(RTDVMVOLUME hVol, RTDVMVOLPROP enmProperty, void *pvBuf, size_t cbBuf, size_t *pcbBuf);

/**
 * Wrapper around RTDvmVolumeQueryProp for simplifying getting unimportant
 * integer properties.
 *
 * @returns The property value if supported and found, the default value if not.
 *          Errors other than VERR_NOT_SUPPORTED and VERR_NOT_FOUND are
 *          asserted.
 * @param   hVol        Handle to the volume.
 * @param   enmProperty The property to query.
 * @param   uDefault    The value to return on error.
 * @sa      RTDvmVolumeQueryProp
 */
RTDECL(uint64_t) RTDvmVolumeGetPropU64(RTDVMVOLUME hVol, RTDVMVOLPROP enmProperty, uint64_t uDefault);

/**
 * Reads data from the given volume.
 *
 * @returns IPRT status code.
 * @param   hVol            The volume handle.
 * @param   off             Where to start reading from - 0 is the beginning of
 *                          the volume.
 * @param   pvBuf           Where to store the read data.
 * @param   cbRead          How many bytes to read.
 */
RTDECL(int) RTDvmVolumeRead(RTDVMVOLUME hVol, uint64_t off, void *pvBuf, size_t cbRead);

/**
 * Writes data to the given volume.
 *
 * @returns IPRT status code.
 * @param   hVol            The volume handle.
 * @param   off             Where to start writing to - 0 is the beginning of
 *                          the volume.
 * @param   pvBuf           The data to write.
 * @param   cbWrite         How many bytes to write.
 */
RTDECL(int) RTDvmVolumeWrite(RTDVMVOLUME hVol, uint64_t off, const void *pvBuf, size_t cbWrite);

/**
 * Returns the description of a given volume type.
 *
 * @returns The description of the type.
 * @param   enmVolType    The volume type.
 */
RTDECL(const char *) RTDvmVolumeTypeGetDescr(RTDVMVOLTYPE enmVolType);

/**
 * Creates an VFS file from a volume handle.
 *
 * @returns IPRT status code.
 * @param   hVol            The volume handle.
 * @param   fOpen           RTFILE_O_XXX.
 * @param   phVfsFileOut    Where to store the VFS file handle on success.
 */
RTDECL(int) RTDvmVolumeCreateVfsFile(RTDVMVOLUME hVol, uint64_t fOpen, PRTVFSFILE phVfsFileOut);

RT_C_DECLS_END

/** @} */

#endif /* !IPRT_INCLUDED_dvm_h */

