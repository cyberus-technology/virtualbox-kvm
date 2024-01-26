/** @file
 * VBox HDD Container API.
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

#ifndef VBOX_INCLUDED_vd_h
#define VBOX_INCLUDED_vd_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/mem.h>
#include <iprt/file.h>
#include <iprt/net.h>
#include <iprt/sg.h>
#include <iprt/vfs.h>
#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/vdmedia.h>
#include <VBox/vd-ifs.h>

RT_C_DECLS_BEGIN

#ifdef IN_RING0
# error "There are no VBox HDD Container APIs available in Ring-0 Host Context!"
#endif

/** @defgroup grp_vd            Virtual Disk Container
 * @{
 */

/** Current VMDK image version. */
#define VMDK_IMAGE_VERSION          (0x0001)

/** Current VDI image major version. */
#define VDI_IMAGE_VERSION_MAJOR     (0x0001)
/** Current VDI image minor version. */
#define VDI_IMAGE_VERSION_MINOR     (0x0001)
/** Current VDI image version. */
#define VDI_IMAGE_VERSION           ((VDI_IMAGE_VERSION_MAJOR << 16) | VDI_IMAGE_VERSION_MINOR)

/** Get VDI major version from combined version. */
#define VDI_GET_VERSION_MAJOR(uVer)    ((uVer) >> 16)
/** Get VDI minor version from combined version. */
#define VDI_GET_VERSION_MINOR(uVer)    ((uVer) & 0xffff)

/** Placeholder for specifying the last opened image. */
#define VD_LAST_IMAGE               0xffffffffU

/** Placeholder for VDCopyEx to indicate that the image content is unknown. */
#define VD_IMAGE_CONTENT_UNKNOWN    0xffffffffU

/** @name VBox HDD container image flags
 * Same values as MediumVariant API enum.
 * @{
 */
/** No flags. */
#define VD_IMAGE_FLAGS_NONE                     (0)
/** Fixed image. */
#define VD_IMAGE_FLAGS_FIXED                    (0x10000)
/** Diff image. Mutually exclusive with fixed image. */
#define VD_IMAGE_FLAGS_DIFF                     (0x20000)
/** VMDK: Split image into 2GB extents. */
#define VD_VMDK_IMAGE_FLAGS_SPLIT_2G            (0x0001)
/** VMDK: Raw disk image (giving access to a number of host partitions). */
#define VD_VMDK_IMAGE_FLAGS_RAWDISK             (0x0002)
/** VMDK: stream optimized image, read only. */
#define VD_VMDK_IMAGE_FLAGS_STREAM_OPTIMIZED    (0x0004)
/** VMDK: ESX variant, use in addition to other flags. */
#define VD_VMDK_IMAGE_FLAGS_ESX                 (0x0008)
/** VDI: Fill new blocks with zeroes while expanding image file. Only valid
 * for newly created images, never set for opened existing images. */
#define VD_VDI_IMAGE_FLAGS_ZERO_EXPAND          (0x0100)

/** Mask of valid image flags for VMDK. */
#define VD_VMDK_IMAGE_FLAGS_MASK            (   VD_IMAGE_FLAGS_FIXED | VD_IMAGE_FLAGS_DIFF | VD_IMAGE_FLAGS_NONE \
                                             |  VD_VMDK_IMAGE_FLAGS_SPLIT_2G | VD_VMDK_IMAGE_FLAGS_RAWDISK \
                                             | VD_VMDK_IMAGE_FLAGS_STREAM_OPTIMIZED | VD_VMDK_IMAGE_FLAGS_ESX)

/** Mask of valid image flags for VDI. */
#define VD_VDI_IMAGE_FLAGS_MASK             (VD_IMAGE_FLAGS_FIXED | VD_IMAGE_FLAGS_DIFF | VD_IMAGE_FLAGS_NONE | VD_VDI_IMAGE_FLAGS_ZERO_EXPAND)

/** Mask of all valid image flags for all formats. */
#define VD_IMAGE_FLAGS_MASK                 (VD_VMDK_IMAGE_FLAGS_MASK | VD_VDI_IMAGE_FLAGS_MASK)

/** Default image flags. */
#define VD_IMAGE_FLAGS_DEFAULT              (VD_IMAGE_FLAGS_NONE)
/** @} */

/** @name VD image repair flags
 * @{
 */
/** Don't repair the image but check what needs to be done. */
#define VD_REPAIR_DRY_RUN                       RT_BIT_32(0)

/** Mask of all valid repair flags. */
#define VD_REPAIR_FLAGS_MASK                    (VD_REPAIR_DRY_RUN)
/** @} */

/** @name VD image VFS file flags
 * @{
 */
/** Destroy the VD disk container when the VFS file is released. */
#define VD_VFSFILE_DESTROY_ON_RELEASE           RT_BIT_32(0)

/** Mask of all valid repair flags. */
#define VD_VFSFILE_FLAGS_MASK                   (VD_VFSFILE_DESTROY_ON_RELEASE)
/** @} */

/** @name VDISKRAW_XXX - VBox raw disk or partition flags
 * @{
 */
/** No special treatment. */
#define VDISKRAW_NORMAL       0
/** Whether this is a raw disk (where the partition information is ignored) or
 * not. Valid only in the raw disk descriptor. */
#define VDISKRAW_DISK         RT_BIT(0)
/** Open the corresponding raw disk or partition for reading only, no matter
 * how the image is created or opened. */
#define VDISKRAW_READONLY     RT_BIT(1)
/** @} */

/**
 * Auxiliary type for describing partitions on raw disks.
 *
 * The entries must be in ascending order (as far as uStart is concerned), and
 * must not overlap. Note that this does not correspond 1:1 to partitions, it is
 * describing the general meaning of contiguous areas on the disk.
 */
typedef struct VDISKRAWPARTDESC
{
    /** Device to use for this partition/data area. Can be the disk device if
     * the offset field is set appropriately. If this is NULL, then this
     * partition will not be accessible to the guest. The size of the data area
     * must still be set correctly. */
    char           *pszRawDevice;
    /** Pointer to the partitioning info. NULL means this is a regular data
     * area on disk, non-NULL denotes data which should be copied to the
     * partition data overlay. */
    void           *pvPartitionData;
    /** Offset where the data starts in this device. */
    uint64_t        offStartInDevice;
    /** Offset where the data starts in the disk. */
    uint64_t        offStartInVDisk;
    /** Size of the data area. */
    uint64_t        cbData;
    /** Flags for special treatment, see VDISKRAW_XXX. */
    uint32_t        uFlags;
} VDISKRAWPARTDESC, *PVDISKRAWPARTDESC;

/**
 * Auxiliary data structure for difference between GPT and MBR disks.
 */
typedef enum VDISKPARTTYPE
{
    VDISKPARTTYPE_MBR = 0,
    VDISKPARTTYPE_GPT
} VDISKPARTTYPE;

/**
 * Auxiliary data structure for creating raw disks.
 */
typedef struct VDISKRAW
{
    /** Signature for structure. Must be 'R', 'A', 'W', '\\0'. Actually a trick
     * to make logging of the comment string produce sensible results. */
    char            szSignature[4];
    /** Flags for special treatment, see VDISKRAW_XXX. */
    uint32_t        uFlags;
    /** Filename for the raw disk. Ignored for partitioned raw disks.
     * For Linux e.g. /dev/sda, and for Windows e.g. //./PhysicalDisk0. */
    char           *pszRawDisk;
    /** Partitioning type of the disk */
    VDISKPARTTYPE   enmPartitioningType;
    /** Number of entries in the partition descriptor array. */
    uint32_t        cPartDescs;
    /** Pointer to the partition descriptor array. */
    PVDISKRAWPARTDESC pPartDescs;
} VDISKRAW, *PVDISKRAW;


/** @name VBox HDD container image open mode flags
 * @{
 */
/** Try to open image in read/write exclusive access mode if possible, or in read-only elsewhere. */
#define VD_OPEN_FLAGS_NORMAL        0
/** Open image in read-only mode with sharing access with others. */
#define VD_OPEN_FLAGS_READONLY      RT_BIT(0)
/** Honor zero block writes instead of ignoring them whenever possible.
 * This is not supported by all formats. It is silently ignored in this case. */
#define VD_OPEN_FLAGS_HONOR_ZEROES  RT_BIT(1)
/** Honor writes of the same data instead of ignoring whenever possible.
 * This is handled generically, and is only meaningful for differential image
 * formats. It is silently ignored otherwise. */
#define VD_OPEN_FLAGS_HONOR_SAME    RT_BIT(2)
/** Do not perform the base/diff image check on open. This does NOT imply
 * opening the image as readonly (would break e.g. adding UUIDs to VMDK files
 * created by other products). Images opened with this flag should only be
 * used for querying information, and nothing else. */
#define VD_OPEN_FLAGS_INFO          RT_BIT(3)
/** Open image for asynchronous access. Only available if VD_CAP_ASYNC_IO is
 * set. VDOpen fails with VERR_NOT_SUPPORTED if this operation is not supported for
 * this kind of image. */
#define VD_OPEN_FLAGS_ASYNC_IO      RT_BIT(4)
/** Allow sharing of the image for writable images. May be ignored if the
 * format backend doesn't support this type of concurrent access. */
#define VD_OPEN_FLAGS_SHAREABLE     RT_BIT(5)
/** Ask the backend to switch to sequential accesses if possible. Opening
 * will not fail if it cannot do this, the flag will be simply ignored. */
#define VD_OPEN_FLAGS_SEQUENTIAL    RT_BIT(6)
/** Allow the discard operation if supported. Only available if VD_CAP_DISCARD
 * is set. VDOpen fails with VERR_VD_DISCARD_NOT_SUPPORTED if discarding is not
 * supported. */
#define VD_OPEN_FLAGS_DISCARD       RT_BIT(7)
/** Ignore all flush requests to workaround certain filesystems which are slow
 * when writing a lot of cached data to the medium.
 * Use with extreme care as a host crash can result in completely corrupted and
 * unusable images.
 */
#define VD_OPEN_FLAGS_IGNORE_FLUSH  RT_BIT(8)
/**
 * Return VINF_VD_NEW_ZEROED_BLOCK for reads from unallocated blocks.
 * The caller who uses the flag has to make sure that the read doesn't cross
 * a block boundary. Because the block size can differ between images reading one
 * sector at a time is the safest solution.
 */
#define VD_OPEN_FLAGS_INFORM_ABOUT_ZERO_BLOCKS RT_BIT(9)
/**
 * Don't do unnecessary consistency checks when opening the image.
 * Only valid when the image is opened in readonly because inconsistencies
 * can lead to corrupted images in read-write mode.
 */
#define VD_OPEN_FLAGS_SKIP_CONSISTENCY_CHECKS  RT_BIT(10)
/** Mask of valid flags. */
#define VD_OPEN_FLAGS_MASK          (VD_OPEN_FLAGS_NORMAL | VD_OPEN_FLAGS_READONLY | VD_OPEN_FLAGS_HONOR_ZEROES | VD_OPEN_FLAGS_HONOR_SAME | VD_OPEN_FLAGS_INFO | VD_OPEN_FLAGS_ASYNC_IO | VD_OPEN_FLAGS_SHAREABLE | VD_OPEN_FLAGS_SEQUENTIAL | VD_OPEN_FLAGS_DISCARD | VD_OPEN_FLAGS_IGNORE_FLUSH | VD_OPEN_FLAGS_INFORM_ABOUT_ZERO_BLOCKS | VD_OPEN_FLAGS_SKIP_CONSISTENCY_CHECKS)
/** @}*/

/** @name VBox HDD container filter flags
 * @{
 */
/** The filter is applied during writes. */
#define VD_FILTER_FLAGS_WRITE RT_BIT(0)
/** The filter is applied during reads. */
#define VD_FILTER_FLAGS_READ  RT_BIT(1)
/** Open the filter in info mode. */
#define VD_FILTER_FLAGS_INFO  RT_BIT(2)
/** Default set of filter flags. */
#define VD_FILTER_FLAGS_DEFAULT (VD_FILTER_FLAGS_WRITE | VD_FILTER_FLAGS_READ)
/** Mask of valid flags. */
#define VD_FILTER_FLAGS_MASK    (VD_FILTER_FLAGS_WRITE | VD_FILTER_FLAGS_READ | VD_FILTER_FLAGS_INFO)
/** @} */

/**
 * Helper functions to handle open flags.
 */

/**
 * Translate VD_OPEN_FLAGS_* to RTFile open flags.
 *
 * @return  RTFile open flags.
 * @param   fOpenFlags      VD_OPEN_FLAGS_* open flags.
 * @param   fCreate         Flag that the file should be created.
 */
DECLINLINE(uint32_t) VDOpenFlagsToFileOpenFlags(unsigned fOpenFlags, bool fCreate)
{
    uint32_t fOpen;
    AssertMsg(!(fOpenFlags & VD_OPEN_FLAGS_READONLY) || !fCreate, ("Image can't be opened readonly while being created\n"));

    if (fOpenFlags & VD_OPEN_FLAGS_READONLY)
        fOpen = RTFILE_O_READ | RTFILE_O_DENY_NONE;
    else
    {
        fOpen = RTFILE_O_READWRITE;

        if (fOpenFlags & VD_OPEN_FLAGS_SHAREABLE)
            fOpen |= RTFILE_O_DENY_NONE;
        else
            fOpen |= RTFILE_O_DENY_WRITE;
    }

    if (!fCreate)
        fOpen |= RTFILE_O_OPEN;
    else
        fOpen |= RTFILE_O_CREATE | RTFILE_O_NOT_CONTENT_INDEXED;

    return fOpen;
}


/** @name VBox HDD container backend capability flags
 * @{
 */
/** Supports UUIDs as expected by VirtualBox code. */
#define VD_CAP_UUID                 RT_BIT(0)
/** Supports creating fixed size images, allocating all space instantly. */
#define VD_CAP_CREATE_FIXED         RT_BIT(1)
/** Supports creating dynamically growing images, allocating space on demand. */
#define VD_CAP_CREATE_DYNAMIC       RT_BIT(2)
/** Supports creating images split in chunks of a bit less than 2GBytes. */
#define VD_CAP_CREATE_SPLIT_2G      RT_BIT(3)
/** Supports being used as differencing image format backend. */
#define VD_CAP_DIFF                 RT_BIT(4)
/** Supports asynchronous I/O operations for at least some configurations. */
#define VD_CAP_ASYNC                RT_BIT(5)
/** The backend operates on files. The caller needs to know to handle the
 * location appropriately. */
#define VD_CAP_FILE                 RT_BIT(6)
/** The backend uses the config interface. The caller needs to know how to
 * provide the mandatory configuration parts this way. */
#define VD_CAP_CONFIG               RT_BIT(7)
/** The backend uses the network stack interface. The caller has to provide
 * the appropriate interface. */
#define VD_CAP_TCPNET               RT_BIT(8)
/** The backend supports VFS (virtual filesystem) functionality since it uses
 * VDINTERFACEIO exclusively for all file operations. */
#define VD_CAP_VFS                  RT_BIT(9)
/** The backend supports the discard operation. */
#define VD_CAP_DISCARD              RT_BIT(10)
/** This is a frequently used backend. */
#define VD_CAP_PREFERRED            RT_BIT(11)
/** @}*/

/** @name Configuration interface key handling flags.
 * @{
 */
/** Mandatory config key. Not providing a value for this key will cause
 * the backend to fail. */
#define VD_CFGKEY_MANDATORY         RT_BIT(0)
/** Expert config key. Not showing it by default in the GUI is is probably
 * a good idea, as the average user won't understand it easily. */
#define VD_CFGKEY_EXPERT            RT_BIT(1)
/** Key only need at media creation, not to be retained in registry.
 *  Should not be exposed in the GUI */
#define VD_CFGKEY_CREATEONLY        RT_BIT(2)
/** @}*/


/**
 * Configuration value type for configuration information interface.
 */
typedef enum VDCFGVALUETYPE
{
    /** Integer value. */
    VDCFGVALUETYPE_INTEGER = 1,
    /** String value. */
    VDCFGVALUETYPE_STRING,
    /** Bytestring value. */
    VDCFGVALUETYPE_BYTES
} VDCFGVALUETYPE;


/**
 * Structure describing configuration keys required/supported by a backend
 * through the config interface.
 */
typedef struct VDCONFIGINFO
{
    /** Key name of the configuration. */
    const char *pszKey;
    /** Pointer to default value (descriptor). NULL if no useful default value
     * can be specified. */
    const char *pszDefaultValue;
    /** Value type for this key. */
    VDCFGVALUETYPE enmValueType;
    /** Key handling flags (a combination of VD_CFGKEY_* flags). */
    uint64_t uKeyFlags;
} VDCONFIGINFO;

/** Pointer to structure describing configuration keys. */
typedef VDCONFIGINFO *PVDCONFIGINFO;

/** Pointer to const structure describing configuration keys. */
typedef const VDCONFIGINFO *PCVDCONFIGINFO;

/**
 * Structure describing a file extension.
 */
typedef struct VDFILEEXTENSION
{
    /** Pointer to the NULL-terminated string containing the extension. */
    const char *pszExtension;
    /** The device type the extension supports. */
    VDTYPE      enmType;
} VDFILEEXTENSION;

/** Pointer to a structure describing a file extension. */
typedef VDFILEEXTENSION *PVDFILEEXTENSION;

/** Pointer to a const structure describing a file extension. */
typedef const VDFILEEXTENSION *PCVDFILEEXTENSION;

/**
 * Data structure for returning a list of backend capabilities.
 */
typedef struct VDBACKENDINFO
{
    /** Name of the backend. Must be unique even with case insensitive comparison. */
    const char *pszBackend;
    /** Capabilities of the backend (a combination of the VD_CAP_* flags). */
    uint64_t uBackendCaps;
    /** Pointer to a NULL-terminated array of strings, containing the supported
     * file extensions. Note that some backends do not work on files, so this
     * pointer may just contain NULL. */
    PCVDFILEEXTENSION paFileExtensions;
    /** Pointer to an array of structs describing each supported config key.
     * Terminated by a NULL config key. Note that some backends do not support
     * the configuration interface, so this pointer may just contain NULL.
     * Mandatory if the backend sets VD_CAP_CONFIG. */
    PCVDCONFIGINFO paConfigInfo;
    /** Returns a human readable hard disk location string given a
     *  set of hard disk configuration keys. The returned string is an
     *  equivalent of the full file path for image-based hard disks.
     *  Mandatory for backends with no VD_CAP_FILE and NULL otherwise. */
    DECLR3CALLBACKMEMBER(int, pfnComposeLocation, (PVDINTERFACE pConfig, char **pszLocation));
    /** Returns a human readable hard disk name string given a
     *  set of hard disk configuration keys. The returned string is an
     *  equivalent of the file name part in the full file path for
     *  image-based hard disks. Mandatory for backends with no
     *  VD_CAP_FILE and NULL otherwise. */
    DECLR3CALLBACKMEMBER(int, pfnComposeName, (PVDINTERFACE pConfig, char **pszName));
} VDBACKENDINFO, *PVDBACKENDINFO;

/**
 * Data structure for returning a list of filter capabilities.
 */
typedef struct VDFILTERINFO
{
    /** Name of the filter. Must be unique even with case insensitive comparison. */
    const char *pszFilter;
    /** Pointer to an array of structs describing each supported config key.
     * Terminated by a NULL config key. Note that some filters do not support
     * the configuration interface, so this pointer may just contain NULL. */
    PCVDCONFIGINFO paConfigInfo;
} VDFILTERINFO, *PVDFILTERINFO;


/**
 * Request completion callback for the async read/write API.
 */
typedef DECLCALLBACKTYPE(void, FNVDASYNCTRANSFERCOMPLETE,(void *pvUser1, void *pvUser2, int rcReq));
/** Pointer to a transfer compelte callback. */
typedef FNVDASYNCTRANSFERCOMPLETE *PFNVDASYNCTRANSFERCOMPLETE;

/**
 * VD Container main structure.
 */
/* Forward declaration, VDISK structure is visible only inside VD module. */
struct VDISK;
typedef struct VDISK VDISK;
typedef VDISK *PVDISK;

/**
 * Initializes HDD backends.
 *
 * @returns VBox status code.
 */
VBOXDDU_DECL(int) VDInit(void);

/**
 * Destroys loaded HDD backends.
 *
 * @returns VBox status code.
 */
VBOXDDU_DECL(int) VDShutdown(void);

/**
 * Loads a single plugin given by filename.
 *
 * @returns VBox status code.
 * @param   pszFilename     The plugin filename to load.
 */
VBOXDDU_DECL(int) VDPluginLoadFromFilename(const char *pszFilename);

/**
 * Load all plugins from a given path.
 *
 * @returns VBox statuse code.
 * @param   pszPath         The path to load plugins from.
 */
VBOXDDU_DECL(int) VDPluginLoadFromPath(const char *pszPath);

/**
 * Unloads a single plugin given by filename.
 *
 * @returns VBox status code.
 * @param   pszFilename     The plugin filename to unload.
 */
VBOXDDU_DECL(int) VDPluginUnloadFromFilename(const char *pszFilename);

/**
 * Unload all plugins from a given path.
 *
 * @returns VBox statuse code.
 * @param   pszPath         The path to unload plugins from.
 */
VBOXDDU_DECL(int) VDPluginUnloadFromPath(const char *pszPath);

/**
 * Lists all HDD backends and their capabilities in a caller-provided buffer.
 *
 * @return  VBox status code.
 *          VERR_BUFFER_OVERFLOW if not enough space is passed.
 * @param   cEntriesAlloc   Number of list entries available.
 * @param   pEntries        Pointer to array for the entries.
 * @param   pcEntriesUsed   Number of entries returned.
 */
VBOXDDU_DECL(int) VDBackendInfo(unsigned cEntriesAlloc, PVDBACKENDINFO pEntries,
                                unsigned *pcEntriesUsed);

/**
 * Lists the capabilities of a backend identified by its name.
 *
 * @return  VBox status code.
 * @param   pszBackend      The backend name (case insensitive).
 * @param   pEntry          Pointer to an entry.
 */
VBOXDDU_DECL(int) VDBackendInfoOne(const char *pszBackend, PVDBACKENDINFO pEntry);

/**
 * Lists all filters and their capabilities in a caller-provided buffer.
 *
 * @return  VBox status code.
 *          VERR_BUFFER_OVERFLOW if not enough space is passed.
 * @param   cEntriesAlloc   Number of list entries available.
 * @param   pEntries        Pointer to array for the entries.
 * @param   pcEntriesUsed   Number of entries returned.
 */
VBOXDDU_DECL(int) VDFilterInfo(unsigned cEntriesAlloc, PVDFILTERINFO pEntries,
                               unsigned *pcEntriesUsed);

/**
 * Lists the capabilities of a filter identified by its name.
 *
 * @return  VBox status code.
 * @param   pszFilter       The filter name (case insensitive).
 * @param   pEntry          Pointer to an entry.
 */
VBOXDDU_DECL(int) VDFilterInfoOne(const char *pszFilter, PVDFILTERINFO pEntry);

/**
 * Allocates and initializes an empty HDD container.
 * No image files are opened.
 *
 * @return  VBox status code.
 * @param   pVDIfsDisk      Pointer to the per-disk VD interface list.
 * @param   enmType         Type of the image container.
 * @param   ppDisk          Where to store the reference to HDD container.
 */
VBOXDDU_DECL(int) VDCreate(PVDINTERFACE pVDIfsDisk, VDTYPE enmType, PVDISK *ppDisk);

/**
 * Destroys HDD container.
 * If container has opened image files they will be closed.
 *
 * @return  VBox status code.
 * @param   pDisk           Pointer to HDD container.
 */
VBOXDDU_DECL(int) VDDestroy(PVDISK pDisk);

/**
 * Try to get the backend name which can use this image.
 *
 * @return  VBox status code.
 *          VINF_SUCCESS if a plugin was found.
 *                       ppszFormat contains the string which can be used as backend name.
 *          VERR_NOT_SUPPORTED if no backend was found.
 * @param   pVDIfsDisk      Pointer to the per-disk VD interface list.
 * @param   pVDIfsImage     Pointer to the per-image VD interface list.
 * @param   pszFilename     Name of the image file for which the backend is queried.
 * @param   enmDesiredType  The desired image type, VDTYPE_INVALID if anything goes.
 * @param   ppszFormat      Receives pointer of the UTF-8 string which contains the format name.
 *                          The returned pointer must be freed using RTStrFree().
 * @param   penmType        Where to store the type of the image.
 */
VBOXDDU_DECL(int) VDGetFormat(PVDINTERFACE pVDIfsDisk, PVDINTERFACE pVDIfsImage,
                              const char *pszFilename, VDTYPE enmDesiredType,
                              char **ppszFormat, VDTYPE *penmType);

/**
 * Opens an image file.
 *
 * The first opened image file in HDD container must have a base image type,
 * others (next opened images) must be differencing or undo images.
 * Linkage is checked for differencing image to be consistent with the previously opened image.
 * When another differencing image is opened and the last image was opened in read/write access
 * mode, then the last image is reopened in read-only with deny write sharing mode. This allows
 * other processes to use images in read-only mode too.
 *
 * Note that the image is opened in read-only mode if a read/write open is not possible.
 * Use VDIsReadOnly to check open mode.
 *
 * @return  VBox status code.
 * @param   pDisk           Pointer to HDD container.
 * @param   pszBackend      Name of the image file backend to use (case insensitive).
 * @param   pszFilename     Name of the image file to open.
 * @param   uOpenFlags      Image file open mode, see VD_OPEN_FLAGS_* constants.
 * @param   pVDIfsImage     Pointer to the per-image VD interface list.
 */
VBOXDDU_DECL(int) VDOpen(PVDISK pDisk, const char *pszBackend,
                         const char *pszFilename, unsigned uOpenFlags,
                         PVDINTERFACE pVDIfsImage);

/**
 * Opens a cache image.
 *
 * @return  VBox status code.
 * @param   pDisk           Pointer to the HDD container which should use the cache image.
 * @param   pszBackend      Name of the cache file backend to use (case insensitive).
 * @param   pszFilename     Name of the cache image to open.
 * @param   uOpenFlags      Image file open mode, see VD_OPEN_FLAGS_* constants.
 * @param   pVDIfsCache     Pointer to the per-cache VD interface list.
 */
VBOXDDU_DECL(int) VDCacheOpen(PVDISK pDisk, const char *pszBackend,
                              const char *pszFilename, unsigned uOpenFlags,
                              PVDINTERFACE pVDIfsCache);

/**
 * Adds a filter to the disk.
 *
 * @returns VBox status code.
 * @param   pDisk           Pointer to the HDD container which should use the filter.
 * @param   pszFilter       Name of the filter backend to use (case insensitive).
 * @param   fFlags          Flags which apply to the filter, combination of VD_FILTER_FLAGS_*
 *                          defines.
 * @param   pVDIfsFilter    Pointer to the per-filter VD interface list.
 */
VBOXDDU_DECL(int) VDFilterAdd(PVDISK pDisk, const char *pszFilter, uint32_t fFlags,
                              PVDINTERFACE pVDIfsFilter);

/**
 * Creates and opens a new base image file.
 *
 * @return  VBox status code.
 * @param   pDisk           Pointer to HDD container.
 * @param   pszBackend      Name of the image file backend to use (case insensitive).
 * @param   pszFilename     Name of the image file to create.
 * @param   cbSize          Image size in bytes.
 * @param   uImageFlags     Flags specifying special image features.
 * @param   pszComment      Pointer to image comment. NULL is ok.
 * @param   pPCHSGeometry   Pointer to physical disk geometry <= (16383,16,63). Not NULL.
 * @param   pLCHSGeometry   Pointer to logical disk geometry <= (x,255,63). Not NULL.
 * @param   pUuid           New UUID of the image. If NULL, a new UUID is created.
 * @param   uOpenFlags      Image file open mode, see VD_OPEN_FLAGS_* constants.
 * @param   pVDIfsImage     Pointer to the per-image VD interface list.
 * @param   pVDIfsOperation Pointer to the per-operation VD interface list.
 */
VBOXDDU_DECL(int) VDCreateBase(PVDISK pDisk, const char *pszBackend,
                               const char *pszFilename, uint64_t cbSize,
                               unsigned uImageFlags, const char *pszComment,
                               PCVDGEOMETRY pPCHSGeometry,
                               PCVDGEOMETRY pLCHSGeometry,
                               PCRTUUID pUuid, unsigned uOpenFlags,
                               PVDINTERFACE pVDIfsImage,
                               PVDINTERFACE pVDIfsOperation);

/**
 * Creates and opens a new differencing image file in HDD container.
 * See comments for VDOpen function about differencing images.
 *
 * @return  VBox status code.
 * @param   pDisk           Pointer to HDD container.
 * @param   pszBackend      Name of the image file backend to use (case insensitive).
 * @param   pszFilename     Name of the differencing image file to create.
 * @param   uImageFlags     Flags specifying special image features.
 * @param   pszComment      Pointer to image comment. NULL is ok.
 * @param   pUuid           New UUID of the image. If NULL, a new UUID is created.
 * @param   pParentUuid     New parent UUID of the image. If NULL, the UUID is queried automatically.
 * @param   uOpenFlags      Image file open mode, see VD_OPEN_FLAGS_* constants.
 * @param   pVDIfsImage     Pointer to the per-image VD interface list.
 * @param   pVDIfsOperation Pointer to the per-operation VD interface list.
 */
VBOXDDU_DECL(int) VDCreateDiff(PVDISK pDisk, const char *pszBackend,
                               const char *pszFilename, unsigned uImageFlags,
                               const char *pszComment, PCRTUUID pUuid,
                               PCRTUUID pParentUuid, unsigned uOpenFlags,
                               PVDINTERFACE pVDIfsImage,
                               PVDINTERFACE pVDIfsOperation);

/**
 * Creates and opens new cache image file in HDD container.
 *
 * @return  VBox status code.
 * @param   pDisk           Name of the cache file backend to use (case insensitive).
 * @param   pszBackend      Name of the image file backend to use (case insensitive).
 * @param   pszFilename     Name of the differencing cache file to create.
 * @param   cbSize          Maximum size of the cache.
 * @param   uImageFlags     Flags specifying special cache features.
 * @param   pszComment      Pointer to image comment. NULL is ok.
 * @param   pUuid           New UUID of the image. If NULL, a new UUID is created.
 * @param   uOpenFlags      Image file open mode, see VD_OPEN_FLAGS_* constants.
 * @param   pVDIfsCache     Pointer to the per-cache VD interface list.
 * @param   pVDIfsOperation Pointer to the per-operation VD interface list.
 */
VBOXDDU_DECL(int) VDCreateCache(PVDISK pDisk, const char *pszBackend,
                                const char *pszFilename, uint64_t cbSize,
                                unsigned uImageFlags, const char *pszComment,
                                PCRTUUID pUuid, unsigned uOpenFlags,
                                PVDINTERFACE pVDIfsCache, PVDINTERFACE pVDIfsOperation);

/**
 * Merges two images (not necessarily with direct parent/child relationship).
 * As a side effect the source image and potentially the other images which
 * are also merged to the destination are deleted from both the disk and the
 * images in the HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImageFrom      Image number to merge from, counts from 0. 0 is always base image of container.
 * @param   nImageTo        Image number to merge to, counts from 0. 0 is always base image of container.
 * @param   pVDIfsOperation Pointer to the per-operation VD interface list.
 */
VBOXDDU_DECL(int) VDMerge(PVDISK pDisk, unsigned nImageFrom,
                          unsigned nImageTo, PVDINTERFACE pVDIfsOperation);

/**
 * Copies an image from one HDD container to another - extended version.
 *
 * The copy is opened in the target HDD container.  It is possible to convert
 * between different image formats, because the backend for the destination may
 * be different from the source.  If both the source and destination reference
 * the same HDD container, then the image is moved (by copying/deleting or
 * renaming) to the new location.  The source container is unchanged if the move
 * operation fails, otherwise the image at the new location is opened in the
 * same way as the old one was.
 *
 * @note The read/write accesses across disks are not synchronized, just the
 * accesses to each disk. Once there is a use case which requires a defined
 * read/write behavior in this situation this needs to be extended.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 *
 * @param   pDiskFrom       Pointer to source HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image
 *                          of container.
 * @param   pDiskTo         Pointer to destination HDD container.
 * @param   pszBackend      Name of the image file backend to use (may be NULL
 *                          to use the same as the source, case insensitive).
 * @param   pszFilename     New name of the image (may be NULL to specify that
 *                          the copy destination is the destination container,
 *                          or if pDiskFrom == pDiskTo, i.e. when moving).
 * @param   fMoveByRename   If true, attempt to perform a move by renaming (if
 *                          successful the new size is ignored).
 * @param   cbSize          New image size (0 means leave unchanged).
 * @param   nImageFromSame  The number of the last image in the source chain
 *                          having the same content as the image in the
 *                          destination chain given by nImageToSame or
 *                          VD_IMAGE_CONTENT_UNKNOWN to indicate that the
 *                          content of both containers is unknown.  See the
 *                          notes for further information.
 * @param   nImageToSame    The number of the last image in the destination
 *                          chain having the same content as the image in the
 *                          source chain given by nImageFromSame or
 *                          VD_IMAGE_CONTENT_UNKNOWN to indicate that the
 *                          content of both containers is unknown. See the notes
 *                          for further information.
 * @param   uImageFlags     Flags specifying special destination image features.
 * @param   pDstUuid        New UUID of the destination image. If NULL, a new
 *                          UUID is created. This parameter is used if and only
 *                          if a true copy is created. In all rename/move cases
 *                          or copy to existing image cases the modification
 *                          UUIDs are copied over.
 * @param   uOpenFlags      Image file open mode, see VD_OPEN_FLAGS_* constants.
 *                          Only used if the destination image is created.
 * @param   pVDIfsOperation Pointer to the per-operation VD interface list.
 * @param   pDstVDIfsImage  Pointer to the per-image VD interface list, for the
 *                          destination image.
 * @param   pDstVDIfsOperation  Pointer to the per-operation VD interface list,
 *                          for the destination operation.
 *
 * @note Using nImageFromSame and nImageToSame can lead to a significant speedup
 *       when copying an image but can also lead to a corrupted copy if used
 *       incorrectly. It is mainly useful when cloning a chain of images and it
 *       is known that the virtual disk content of the two chains is exactly the
 *       same upto a certain image. Example:
 *          Imagine the chain of images which consist of a base and one diff
 *          image. Copying the chain starts with the base image. When copying
 *          the first diff image VDCopy() will read the data from the diff of
 *          the source chain and probably from the base image again in case the
 *          diff doesn't has data for the block. However the block will be
 *          optimized away because VDCopy() reads data from the base image of
 *          the destination chain compares the to and suppresses the write
 *          because the data is unchanged. For a lot of diff images this will be
 *          a huge waste of I/O bandwidth if the diff images contain only few
 *          changes. Because it is known that the base image of the source and
 *          the destination chain have the same content it is enough to check
 *          the diff image for changed data and copy it to the destination diff
 *          image which is achieved with nImageFromSame and nImageToSame.
 *          Setting both to 0 can suppress a lot of I/O.
 */
VBOXDDU_DECL(int) VDCopyEx(PVDISK pDiskFrom, unsigned nImage, PVDISK pDiskTo,
                           const char *pszBackend, const char *pszFilename,
                           bool fMoveByRename, uint64_t cbSize,
                           unsigned nImageFromSame, unsigned nImageToSame,
                           unsigned uImageFlags, PCRTUUID pDstUuid,
                           unsigned uOpenFlags, PVDINTERFACE pVDIfsOperation,
                           PVDINTERFACE pDstVDIfsImage,
                           PVDINTERFACE pDstVDIfsOperation);

/**
 * Copies an image from one HDD container to another.
 * The copy is opened in the target HDD container.
 * It is possible to convert between different image formats, because the
 * backend for the destination may be different from the source.
 * If both the source and destination reference the same HDD container,
 * then the image is moved (by copying/deleting or renaming) to the new location.
 * The source container is unchanged if the move operation fails, otherwise
 * the image at the new location is opened in the same way as the old one was.
 *
 * @note The read/write accesses across disks are not synchronized, just the
 * accesses to each disk. Once there is a use case which requires a defined
 * read/write behavior in this situation this needs to be extended.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDiskFrom       Pointer to source HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pDiskTo         Pointer to destination HDD container.
 * @param   pszBackend      Name of the image file backend to use (may be NULL to use the same as the source, case insensitive).
 * @param   pszFilename     New name of the image (may be NULL to specify that the
 *                          copy destination is the destination container, or
 *                          if pDiskFrom == pDiskTo, i.e. when moving).
 * @param   fMoveByRename   If true, attempt to perform a move by renaming (if successful the new size is ignored).
 * @param   cbSize          New image size (0 means leave unchanged).
 * @param   uImageFlags     Flags specifying special destination image features.
 * @param   pDstUuid        New UUID of the destination image. If NULL, a new UUID is created.
 *                          This parameter is used if and only if a true copy is created.
 *                          In all rename/move cases or copy to existing image cases the modification UUIDs are copied over.
 * @param   uOpenFlags      Image file open mode, see VD_OPEN_FLAGS_* constants.
 *                          Only used if the destination image is created.
 * @param   pVDIfsOperation Pointer to the per-operation VD interface list.
 * @param   pDstVDIfsImage  Pointer to the per-image VD interface list, for the
 *                          destination image.
 * @param   pDstVDIfsOperation Pointer to the per-operation VD interface list,
 *                          for the destination operation.
 */
VBOXDDU_DECL(int) VDCopy(PVDISK pDiskFrom, unsigned nImage, PVDISK pDiskTo,
                         const char *pszBackend, const char *pszFilename,
                         bool fMoveByRename, uint64_t cbSize,
                         unsigned uImageFlags, PCRTUUID pDstUuid,
                         unsigned uOpenFlags, PVDINTERFACE pVDIfsOperation,
                         PVDINTERFACE pDstVDIfsImage,
                         PVDINTERFACE pDstVDIfsOperation);

/**
 * Optimizes the storage consumption of an image. Typically the unused blocks
 * have to be wiped with zeroes to achieve a substantial reduced storage use.
 * Another optimization done is reordering the image blocks, which can provide
 * a significant performance boost, as reads and writes tend to use less random
 * file offsets.
 *
 * @note Compaction is treated as a single operation with regard to thread
 * synchronization, which means that it potentially blocks other activities for
 * a long time. The complexity of compaction would grow even more if concurrent
 * accesses have to be handled.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @return  VERR_VD_IMAGE_READ_ONLY if image is not writable.
 * @return  VERR_NOT_SUPPORTED if this kind of image can be compacted, but
 *                             this isn't supported yet.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pVDIfsOperation Pointer to the per-operation VD interface list.
 */
VBOXDDU_DECL(int) VDCompact(PVDISK pDisk, unsigned nImage, PVDINTERFACE pVDIfsOperation);

/**
 * Resizes the given disk image to the given size. It is OK if there are
 * multiple images open in the container. In this case the last disk image
 * will be resized.
 *
 * @return  VBox status
 * @return  VERR_VD_IMAGE_READ_ONLY if image is not writable.
 * @return  VERR_NOT_SUPPORTED if this kind of image can't be compacted.
 *
 * @param   pDisk           Pointer to the HDD container.
 * @param   cbSize          New size of the image.
 * @param   pPCHSGeometry   Pointer to the new physical disk geometry <= (16383,16,63). Not NULL.
 * @param   pLCHSGeometry   Pointer to the new logical disk geometry <= (x,255,63). Not NULL.
 * @param   pVDIfsOperation Pointer to the per-operation VD interface list.
 */
VBOXDDU_DECL(int) VDResize(PVDISK pDisk, uint64_t cbSize,
                           PCVDGEOMETRY pPCHSGeometry,
                           PCVDGEOMETRY pLCHSGeometry,
                           PVDINTERFACE pVDIfsOperation);

/**
 * Prepares the given disk for use by the added filters. This applies to all
 * opened images in the chain which might be opened read/write temporary.
 *
 * @return  VBox status code.
 *
 * @param   pDisk           Pointer to the HDD container.
 * @param   pVDIfsOperation Pointer to the per-operation VD interface list.
 */
VBOXDDU_DECL(int) VDPrepareWithFilters(PVDISK pDisk, PVDINTERFACE pVDIfsOperation);

/**
 * Closes the last opened image file in HDD container.
 * If previous image file was opened in read-only mode (the normal case) and
 * the last opened image is in read-write mode then the previous image will be
 * reopened in read/write mode.
 *
 * @return  VBox status code.
 * @return  VERR_VD_NOT_OPENED if no image is opened in HDD container.
 * @param   pDisk           Pointer to HDD container.
 * @param   fDelete         If true, delete the image from the host disk.
 */
VBOXDDU_DECL(int) VDClose(PVDISK pDisk, bool fDelete);

/**
 * Removes the last added filter in the HDD container from the specified chain.
 *
 * @return  VBox status code.
 * @retval  VERR_VD_NOT_OPENED if no filter is present for the disk.
 * @param   pDisk           Pointer to HDD container.
 * @param   fFlags          Combination of VD_FILTER_FLAGS_* defines.
 */
VBOXDDU_DECL(int) VDFilterRemove(PVDISK pDisk, uint32_t fFlags);

/**
 * Closes the currently opened cache image file in HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_NOT_OPENED if no cache is opened in HDD container.
 * @param   pDisk           Pointer to HDD container.
 * @param   fDelete         If true, delete the image from the host disk.
 */
VBOXDDU_DECL(int) VDCacheClose(PVDISK pDisk, bool fDelete);

/**
 * Closes all opened image files in HDD container.
 *
 * @return  VBox status code.
 * @param   pDisk           Pointer to HDD container.
 */
VBOXDDU_DECL(int) VDCloseAll(PVDISK pDisk);

/**
 * Removes all filters of the given HDD container.
 *
 * @return  VBox status code.
 * @param   pDisk           Pointer to HDD container.
 */
VBOXDDU_DECL(int) VDFilterRemoveAll(PVDISK pDisk);

/**
 * Read data from virtual HDD.
 *
 * @return  VBox status code.
 * @retval  VERR_VD_NOT_OPENED if no image is opened in HDD container.
 * @param   pDisk           Pointer to HDD container.
 * @param   uOffset         Offset of first reading byte from start of disk.
 *                          Must be aligned to a sector boundary.
 * @param   pvBuf           Pointer to buffer for reading data.
 * @param   cbRead          Number of bytes to read.
 *                          Must be aligned to a sector boundary.
 */
VBOXDDU_DECL(int) VDRead(PVDISK pDisk, uint64_t uOffset, void *pvBuf, size_t cbRead);

/**
 * Write data to virtual HDD.
 *
 * @return  VBox status code.
 * @retval  VERR_VD_NOT_OPENED if no image is opened in HDD container.
 * @param   pDisk           Pointer to HDD container.
 * @param   uOffset         Offset of first writing byte from start of disk.
 *                          Must be aligned to a sector boundary.
 * @param   pvBuf           Pointer to buffer for writing data.
 * @param   cbWrite         Number of bytes to write.
 *                          Must be aligned to a sector boundary.
 */
VBOXDDU_DECL(int) VDWrite(PVDISK pDisk, uint64_t uOffset, const void *pvBuf, size_t cbWrite);

/**
 * Make sure the on disk representation of a virtual HDD is up to date.
 *
 * @return  VBox status code.
 * @retval  VERR_VD_NOT_OPENED if no image is opened in HDD container.
 * @param   pDisk           Pointer to HDD container.
 */
VBOXDDU_DECL(int) VDFlush(PVDISK pDisk);

/**
 * Get number of opened images in HDD container.
 *
 * @return  Number of opened images for HDD container. 0 if no images have been opened.
 * @param   pDisk           Pointer to HDD container.
 */
VBOXDDU_DECL(unsigned) VDGetCount(PVDISK pDisk);

/**
 * Get read/write mode of HDD container.
 *
 * @return  Virtual disk ReadOnly status.
 * @return  true if no image is opened in HDD container.
 * @param   pDisk           Pointer to HDD container.
 */
VBOXDDU_DECL(bool) VDIsReadOnly(PVDISK pDisk);

/**
 * Get sector size of an image in HDD container.
 *
 * @return  Virtual disk sector size in bytes.
 * @return  0 if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 */
VBOXDDU_DECL(uint32_t) VDGetSectorSize(PVDISK pDisk, unsigned nImage);

/**
 * Get total capacity of an image in HDD container.
 *
 * @return  Virtual disk size in bytes.
 * @return  0 if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 */
VBOXDDU_DECL(uint64_t) VDGetSize(PVDISK pDisk, unsigned nImage);

/**
 * Get total file size of an image in HDD container.
 *
 * @return  Virtual disk size in bytes.
 * @return  0 if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 */
VBOXDDU_DECL(uint64_t) VDGetFileSize(PVDISK pDisk, unsigned nImage);

/**
 * Get virtual disk PCHS geometry of an image in HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @return  VERR_VD_GEOMETRY_NOT_SET if no geometry present in the HDD container.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pPCHSGeometry   Where to store PCHS geometry. Not NULL.
 */
VBOXDDU_DECL(int) VDGetPCHSGeometry(PVDISK pDisk, unsigned nImage, PVDGEOMETRY pPCHSGeometry);

/**
 * Store virtual disk PCHS geometry of an image in HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pPCHSGeometry   Where to load PCHS geometry from. Not NULL.
 */
VBOXDDU_DECL(int) VDSetPCHSGeometry(PVDISK pDisk, unsigned nImage, PCVDGEOMETRY pPCHSGeometry);

/**
 * Get virtual disk LCHS geometry of an image in HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @return  VERR_VD_GEOMETRY_NOT_SET if no geometry present in the HDD container.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pLCHSGeometry   Where to store LCHS geometry. Not NULL.
 */
VBOXDDU_DECL(int) VDGetLCHSGeometry(PVDISK pDisk, unsigned nImage, PVDGEOMETRY pLCHSGeometry);

/**
 * Store virtual disk LCHS geometry of an image in HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pLCHSGeometry   Where to load LCHS geometry from. Not NULL.
 */
VBOXDDU_DECL(int) VDSetLCHSGeometry(PVDISK pDisk, unsigned nImage, PCVDGEOMETRY pLCHSGeometry);

/**
 * Queries the available regions of an image in the given VD container.
 *
 * @return  VBox status code.
 * @retval  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @retval  VERR_NOT_SUPPORTED if the image backend doesn't support region lists.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   fFlags          Combination of VD_REGION_LIST_F_* flags.
 * @param   ppRegionList    Where to store the pointer to the region list on success, must be freed
 *                          with VDRegionListFree().
 */
VBOXDDU_DECL(int) VDQueryRegions(PVDISK pDisk, unsigned nImage, uint32_t fFlags,
                                 PPVDREGIONLIST ppRegionList);

/**
 * Frees a region list previously queried with VDQueryRegions().
 *
 * @param   pRegionList     The region list to free.
 */
VBOXDDU_DECL(void) VDRegionListFree(PVDREGIONLIST pRegionList);

/**
 * Get version of image in HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   puVersion       Where to store the image version.
 */
VBOXDDU_DECL(int) VDGetVersion(PVDISK pDisk, unsigned nImage, unsigned *puVersion);

/**
 * List the capabilities of image backend in HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to the HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pBackendInfo    Where to store the backend information.
 */
VBOXDDU_DECL(int) VDBackendInfoSingle(PVDISK pDisk, unsigned nImage, PVDBACKENDINFO pBackendInfo);

/**
 * Get flags of image in HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   puImageFlags    Where to store the image flags.
 */
VBOXDDU_DECL(int) VDGetImageFlags(PVDISK pDisk, unsigned nImage, unsigned *puImageFlags);

/**
 * Get open flags of image in HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   puOpenFlags     Where to store the image open flags.
 */
VBOXDDU_DECL(int) VDGetOpenFlags(PVDISK pDisk, unsigned nImage, unsigned *puOpenFlags);

/**
 * Set open flags of image in HDD container.
 * This operation may cause file locking changes and/or files being reopened.
 * Note that in case of unrecoverable error all images in HDD container will be closed.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   uOpenFlags      Image file open mode, see VD_OPEN_FLAGS_* constants.
 */
VBOXDDU_DECL(int) VDSetOpenFlags(PVDISK pDisk, unsigned nImage, unsigned uOpenFlags);

/**
 * Get base filename of image in HDD container. Some image formats use
 * other filenames as well, so don't use this for anything but informational
 * purposes.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @return  VERR_BUFFER_OVERFLOW if pszFilename buffer too small to hold filename.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pszFilename     Where to store the image file name.
 * @param   cbFilename      Size of buffer pszFilename points to.
 */
VBOXDDU_DECL(int) VDGetFilename(PVDISK pDisk, unsigned nImage, char *pszFilename, unsigned cbFilename);

/**
 * Get the comment line of image in HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @return  VERR_BUFFER_OVERFLOW if pszComment buffer too small to hold comment text.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pszComment      Where to store the comment string of image. NULL is ok.
 * @param   cbComment       The size of pszComment buffer. 0 is ok.
 */
VBOXDDU_DECL(int) VDGetComment(PVDISK pDisk, unsigned nImage, char *pszComment, unsigned cbComment);

/**
 * Changes the comment line of image in HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pszComment      New comment string (UTF-8). NULL is allowed to reset the comment.
 */
VBOXDDU_DECL(int) VDSetComment(PVDISK pDisk, unsigned nImage, const char *pszComment);

/**
 * Get UUID of image in HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pUuid           Where to store the image UUID.
 */
VBOXDDU_DECL(int) VDGetUuid(PVDISK pDisk, unsigned nImage, PRTUUID pUuid);

/**
 * Set the image's UUID. Should not be used by normal applications.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pUuid           New UUID of the image. If NULL, a new UUID is created.
 */
VBOXDDU_DECL(int) VDSetUuid(PVDISK pDisk, unsigned nImage, PCRTUUID pUuid);

/**
 * Get last modification UUID of image in HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pUuid           Where to store the image modification UUID.
 */
VBOXDDU_DECL(int) VDGetModificationUuid(PVDISK pDisk, unsigned nImage, PRTUUID pUuid);

/**
 * Set the image's last modification UUID. Should not be used by normal applications.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pUuid           New modification UUID of the image. If NULL, a new UUID is created.
 */
VBOXDDU_DECL(int) VDSetModificationUuid(PVDISK pDisk, unsigned nImage, PCRTUUID pUuid);

/**
 * Get parent UUID of image in HDD container.
 *
 * @return  VBox status code.
 * @return  VERR_VD_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of the container.
 * @param   pUuid           Where to store the parent image UUID.
 */
VBOXDDU_DECL(int) VDGetParentUuid(PVDISK pDisk, unsigned nImage, PRTUUID pUuid);

/**
 * Set the image's parent UUID. Should not be used by normal applications.
 *
 * @return  VBox status code.
 * @param   pDisk           Pointer to HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pUuid           New parent UUID of the image. If NULL, a new UUID is created.
 */
VBOXDDU_DECL(int) VDSetParentUuid(PVDISK pDisk, unsigned nImage, PCRTUUID pUuid);


/**
 * Debug helper - dumps all opened images in HDD container into the log file.
 *
 * @param   pDisk           Pointer to HDD container.
 */
VBOXDDU_DECL(void) VDDumpImages(PVDISK pDisk);


/**
 * Discards unused ranges given as a list.
 *
 * @return  VBox status code.
 * @param   pDisk           Pointer to HDD container.
 * @param   paRanges        The array of ranges to discard.
 * @param   cRanges         Number of entries in the array.
 *
 * @note In contrast to VDCompact() the ranges are always discarded even if they
 *       appear to contain data. This method is mainly used to implement TRIM support.
 */
VBOXDDU_DECL(int) VDDiscardRanges(PVDISK pDisk, PCRTRANGE paRanges, unsigned cRanges);


/**
 * Start an asynchronous read request.
 *
 * @return  VBox status code.
 * @param   pDisk           Pointer to the HDD container.
 * @param   off             The offset of the virtual disk to read from.
 * @param   cbRead          How many bytes to read.
 * @param   pSgBuf          Pointer to the S/G buffer to read into.
 * @param   pfnComplete     Completion callback.
 * @param   pvUser1         User data which is passed on completion.
 * @param   pvUser2         User data which is passed on completion.
 */
VBOXDDU_DECL(int) VDAsyncRead(PVDISK pDisk, uint64_t off, size_t cbRead,
                              PCRTSGBUF pSgBuf,
                              PFNVDASYNCTRANSFERCOMPLETE pfnComplete,
                              void *pvUser1, void *pvUser2);


/**
 * Start an asynchronous write request.
 *
 * @return  VBox status code.
 * @param   pDisk           Pointer to the HDD container.
 * @param   off             The offset of the virtual disk to write to.
 * @param   cbWrite         How many bytes to write.
 * @param   pSgBuf          Pointer to the S/G buffer to write from.
 * @param   pfnComplete     Completion callback.
 * @param   pvUser1         User data which is passed on completion.
 * @param   pvUser2         User data which is passed on completion.
 */
VBOXDDU_DECL(int) VDAsyncWrite(PVDISK pDisk, uint64_t off, size_t cbWrite,
                               PCRTSGBUF pSgBuf,
                               PFNVDASYNCTRANSFERCOMPLETE pfnComplete,
                               void *pvUser1, void *pvUser2);


/**
 * Start an asynchronous flush request.
 *
 * @return  VBox status code.
 * @param   pDisk           Pointer to the HDD container.
 * @param   pfnComplete     Completion callback.
 * @param   pvUser1         User data which is passed on completion.
 * @param   pvUser2         User data which is passed on completion.
 */
VBOXDDU_DECL(int) VDAsyncFlush(PVDISK pDisk,
                               PFNVDASYNCTRANSFERCOMPLETE pfnComplete,
                               void *pvUser1, void *pvUser2);

/**
 * Start an asynchronous discard request.
 *
 * @return  VBox status code.
 * @param   pDisk           Pointer to HDD container.
 * @param   paRanges        The array of ranges to discard.
 * @param   cRanges         Number of entries in the array.
 * @param   pfnComplete     Completion callback.
 * @param   pvUser1         User data which is passed on completion.
 * @param   pvUser2         User data which is passed on completion.
 */
VBOXDDU_DECL(int) VDAsyncDiscardRanges(PVDISK pDisk, PCRTRANGE paRanges, unsigned cRanges,
                                       PFNVDASYNCTRANSFERCOMPLETE pfnComplete,
                                       void *pvUser1, void *pvUser2);

/**
 * Tries to repair a corrupted image.
 *
 * @return  VBox status code.
 * @retval  VERR_VD_IMAGE_REPAIR_NOT_SUPPORTED if the backend does not support repairing the image.
 * @retval  VERR_VD_IMAGE_REPAIR_IMPOSSIBLE if the corruption is to severe to repair the image.
 * @param   pVDIfsDisk      Pointer to the per-disk VD interface list.
 * @param   pVDIfsImage     Pointer to the per-image VD interface list.
 * @param   pszFilename     Name of the image file to repair.
 * @param   pszBackend      The backend to use.
 * @param   fFlags          Combination of the VD_REPAIR_* flags.
 */
VBOXDDU_DECL(int) VDRepair(PVDINTERFACE pVDIfsDisk, PVDINTERFACE pVDIfsImage,
                           const char *pszFilename, const char *pszBackend, uint32_t fFlags);

/**
 * Create a VFS file handle from the given HDD container.
 *
 * @return  VBox status code.
 * @param   pDisk           Pointer to HDD container.
 * @param   fFlags          Combination of the VD_VFSFILE_* flags.
 * @param   phVfsFile       Where to store the handle to the VFS file on
 *                          success.
 */
VBOXDDU_DECL(int) VDCreateVfsFileFromDisk(PVDISK pDisk, uint32_t fFlags,
                                          PRTVFSFILE phVfsFile);



/** @defgroup grp_vd_ifs_def    Default implementations for certain VD interfaces.
 * @{
 */
/** Internal per interface instance data. */
typedef struct VDIFINSTINT *VDIFINST;
/** Pointer to the per instance interface data. */
typedef VDIFINST *PVDIFINST;

/**
 * Creates a new VD TCP/IP interface instance and adds it to the given interface list.
 *
 * @returns VBox status code.
 * @param   phTcpNetInst        Where to store the TCP/IP interface handle on success.
 * @param   ppVdIfs             Pointer to the VD interface list.
 */
VBOXDDU_DECL(int) VDIfTcpNetInstDefaultCreate(PVDIFINST phTcpNetInst, PVDINTERFACE *ppVdIfs);

/**
 * Destroys the given VD TCP/IP interface instance.
 *
 * @param   hTcpNetInst         The TCP/IP interface instance handle.
 */
VBOXDDU_DECL(void) VDIfTcpNetInstDefaultDestroy(VDIFINST hTcpNetInst);
/** @} */



/** @defgroup grp_vd_ioiter     I/O iterator
 * @{
 */

/** Read metadata coming before each main data block addressed in the segment. */
#define VD_IOITER_SEG_F_PRE_METADATA  RT_BIT_32(0)
/** Read the main user data of each addressed block in the segment. */
#define VD_IOITER_SEG_F_MAIN_DATA     RT_BIT_32(1)
/** Read metadata coming after each main data block addressed in the segment. */
#define VD_IOITER_SEG_F_POST_METADATA RT_BIT_32(2)
/** Read checksum data of each data block addressed in the segment. */
#define VD_IOITER_SEG_F_CHKSUM        RT_BIT_32(3)
/** Read all available data for each addressed block in the segment. */
#define VD_IOITER_SEG_F_AVAILABLE     RT_BIT_32(4)

/** The offset and size members in the segments use byte granularity instead of a
 * block address and number of blocks respectively. */
#define VDIOITER_F_BYTE_OFFSET_AND_SIZE RT_BIT_32(0)

/**
 * VD I/O iterator segment.
 */
typedef struct VDIOITERSEG
{
    /** Start offset for this segment. */
    uint64_t            offStartSeg;
    /** Size of the segment (bytes or blocks). */
    uint64_t            cSizeSeg;
    /** Flags for this segment, see VD_IOITER_SEG_F_*. */
    uint32_t            fFlags;
} VDIOITERSEG;
/** Pointer to a I/O iterator segment. */
typedef VDIOITERSEG *PVDIOITERSEG;
/** Pointer to a constant I/O iterator segment. */
typedef VDIOITERSEG *PCVDIOITERSEG;

/** I/O iterator handle. */
typedef struct VDIOITERINT *VDIOITER;
/** Pointer to a I/O iterator handle. */
typedef VDIOITER *PVDIOITER;

/**
 * Create a new I/O iterator.
 *
 * @returns VBox status code.
 * @param   pDisk           The disk to create the iterator for.
 * @param   phVdIoIter      Where to store the handle to the I/O iterator on success.
 * @param   paIoIterSegs    The segments for the iterator, can be destroyed after the call.
 * @param   cIoIterSegs     Number of segments.
 * @param   fFlags          Flags for the iterator, see VDIOITER_F_*
 */
VBOXDDU_DECL(int) VDIoIterCreate(PVDISK pDisk, PVDIOITER phVdIoIter, PCVDIOITERSEG paIoIterSegs,
                                 uint32_t cIoIterSegs, uint32_t fFlags);

/**
 * Retains the reference count of the given I/O iterator.
 *
 * @returns New reference count.
 * @param   hVdIoIter       The I/O iterator handle.
 */
VBOXDDU_DECL(uint32_t) VDIoIterRetain(VDIOITER hVdIoIter);

/**
 * Releases the reference count of the given I/O iterator.
 *
 * @returns New reference count, on 0 the iterator is destroyed.
 * @param   hVdIoIter       The I/O iterator handle.
 */
VBOXDDU_DECL(uint32_t) VDIoIterRelease(VDIOITER hVdIoIter);

/**
 * Returns the number of segments in the given I/O iterator.
 *
 * @returns Number of segments.
 * @param   hVdIoIter       The I/O iterator handle.
 */
VBOXDDU_DECL(uint32_t) VDIoIterGetSegmentCount(VDIOITER hVdIoIter);

/**
 * Returns the flags of the given I/O iterator.
 *
 * @returns Flags.
 * @param   hVdIoIter       The I/O iterator handle.
 */
VBOXDDU_DECL(uint32_t) VDIoIterGetFlags(VDIOITER hVdIoIter);

/**
 * Queries the properties of the given segment for the given I/O iterator.
 *
 * @returns VBox status code.
 * @param   hVdIoIter       The I/O iterator handle.
 * @param   idx             The segment index to query.
 * @param   pSegment        Where to store the segment properties on success.
 */
VBOXDDU_DECL(int) VDIoIterQuerySegment(VDIOITER hVdIoIter, uint32_t idx, PVDIOITERSEG pSegment);

/** @} */


/** @defgroup grp_vd_io_buf     I/O buffer management API.
 * @{
 */

/** VD I/O buffer manager handle. */
typedef struct VDIOBUFMGRINT *VDIOBUFMGR;
/** Pointer to VD I/O buffer manager handle. */
typedef VDIOBUFMGR *PVDIOBUFMGR;

/** VD I/O buffer handle. */
typedef struct VDIOBUFINT *VDIOBUF;
/** Pointer to a VD I/O buffer handle. */
typedef VDIOBUF *PVDIOBUF;

/** Default I/O buffer manager flags. */
#define VD_IOBUFMGR_F_DEFAULT             (0)
/** I/O buffer memory needs to be non pageable (for example because it contains sensitive data
 * which shouldn't end up in swap unencrypted). */
#define VD_IOBUFMGR_F_REQUIRE_NOT_PAGABLE RT_BIT(0)

/** Pointer to VD I/O buffer callbacks. */
typedef struct VDIOBUFCALLBACKS *PVDIOBUFCALLBACKS;
/** Pointer to const VD I/O buffer callbacks. */
typedef const struct VDIOBUFCALLBACKS *PCVDIOBUFCALLBACKS;

/**
 * VD I/O buffer callbacks.
 */
typedef struct VDIOBUFCALLBACKS
{
    /**
     * Copy data from the memory buffer of the caller to the callees memory buffer for the given request.
     *
     * @returns VBox status code.
     * @retval  VERR_PDM_MEDIAEX_IOBUF_OVERFLOW if there is not enough room to store the data.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoBuf          The I/O request handle.
     * @param   pvIoBufAlloc    The allocator specific memory for this request.
     * @param   offDst          The destination offset from the start to write the data to.
     * @param   pSgBuf          The S/G buffer to read the data from.
     * @param   cbCopy          How many bytes to copy.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoBufCopyFromBuf, (PVDIOBUFCALLBACKS pInterface, VDIOBUF hIoBuf,
                                                    void *pvIoBufAlloc, uint32_t offDst, PRTSGBUF pSgBuf,
                                                    size_t cbCopy));

    /**
     * Copy data to the memory buffer of the caller from the callees memory buffer for the given request.
     *
     * @returns VBox status code.
     * @retval  VERR_PDM_MEDIAEX_IOBUF_UNDERRUN if there is not enough data to copy from the buffer.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoBuf          The I/O request handle.
     * @param   pvIoBufAlloc    The allocator specific memory for this request.
     * @param   offSrc          The offset from the start of the buffer to read the data from.
     * @param   pSgBuf          The S/G buffer to write the data to.
     * @param   cbCopy          How many bytes to copy.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoBufCopyToBuf, (PVDIOBUFCALLBACKS pInterface, VDIOBUF hIoBuf,
                                                  void *pvIoBufAlloc, uint32_t offSrc, PRTSGBUF pSgBuf,
                                                  size_t cbCopy));

    /**
     * Queries a pointer to the memory buffer for the request from the drive/device above.
     *
     * @returns VBox status code.
     * @retval  VERR_NOT_SUPPORTED if this is not supported for this request.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoBuf          The I/O request handle.
     * @param   pvIoBufAlloc    The allocator specific memory for this request.
     * @param   offBuf          The offset from the start of the buffer to get the buffer address.
     * @param   cbBuf           The number of bytes requested.
     * @param   ppvBuf          Where to store the pointer to the guest buffer on success.
     * @param   pcbBuf          Where to store the size of the buffer on success.
     *
     * @note This is an optional feature of the entity implementing this interface to avoid overhead
     *       by copying the data between buffers. If NULL it is not supported at all and the caller
     *       has to resort to VDIOBUFCALLBACKS::pfnIoBufCopyToBuf and VDIOBUFCALLBACKS::pfnIoBufCopyFromBuf.
     *       The same holds when VERR_NOT_SUPPORTED is returned.
     *
     *       On the upside the caller of this interface might not call this method at all and just
     *       use the before mentioned methods to copy the data between the buffers.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoBufQueryBuf, (PVDIOBUFCALLBACKS pInterface, VDIOBUF hIoBuf,
                                                 void *pvIoBufAlloc, uint32_t offBuf, size_t cbBuf,
                                                 void **ppvBuf, size_t *pcbBuf));

} VDIOBUFCALLBACKS;

/**
 * Creates a new I/O buffer manager.
 *
 * @returns VBox status code.
 * @param   phIoBufMgr      Where to store the handle to the I/O buffer manager on success.
 * @param   cbMax           The maximum amount of I/O memory to allow. Trying to allocate more than
 *                          this will lead to out of memory errors. 0 for "unlimited" size (only restriction
 *                          is the available memory on the host).
 * @param   fFlags          Combination of VD_IOBUFMGR_F_*.
 * @param   pIoBufClbks     Memory copy callbacks between source and target memory regions, optional.
 *                          When NULL all I/O buffers must be allocated with a valid S/G buffer laying out the
 *                          memory.
 * @param   cbIoBufAlloc    How much to allocate extra in the I/O buffer for private use.
 */
VBOXDDU_DECL(int) VDIoBufMgrCreate(PVDIOBUFMGR phIoBufMgr, size_t cbMax, uint32_t fFlags,
                                   PVDIOBUFCALLBACKS pIoBufClbks, size_t cbIoBufAlloc);

/**
 * Destroys the given I/O buffer manager.
 *
 * @returns VBox status code.
 * @retval  VERR_INVALID_STATE if there are still buffers allocated by the given manager.
 * @param   hIoBufMgr       The I/O buffer manager.
 */
VBOXDDU_DECL(int) VDIoBufMgrDestroy(VDIOBUFMGR hIoBufMgr);

/**
 * Allocate a new I/O buffer handle.
 *
 * @returns VBox status code.
 * @param   hIoBufMgr       The I/O buffer manager to use.
 * @param   phIoBuf         Where to store the I/O buffer handle on success.
 * @param   ppvIoBufAlloc   Where to store the pointe to the private party on success.
 * @param   pSgBuf          The S/G buffer to use, optional. If NULL the I/O buffer callbacks
 *                          supplied when creating the owning manager are used to transfer the
 *                          data.
 * @param   cbBuf           Size of the buffer in bytes.
 */
VBOXDDU_DECL(int) VDIoBufMgrAllocBuf(VDIOBUFMGR hIoBufMgr, PVDIOBUF phIoBuf, void **ppvIoBufAlloc,
                                     PCRTSGBUF pSgBuf, size_t cbBuf);

/**
 * Retains the I/O buffer reference count.
 *
 * @returns New reference count.
 * @param   hIoBuf          The I/O buffer handle.
 */
VBOXDDU_DECL(uint32_t) VDIoBufRetain(VDIOBUF hIoBuf);

/**
 * Releases the given I/O buffer reference.
 *
 * @returns New reference count, on 0 the I/O buffer is destroyed.
 * @param   hIoBuf          The I/O buffer handle.
 */
VBOXDDU_DECL(uint32_t) VDIoBufRelease(VDIOBUF hIoBuf);

/** @} */


/** @defgroup grp_vd_ioqueue    I/O queues
 * @{
 */

/** VD I/O queue handle. */
typedef struct VDIOQUEUEINT *VDIOQUEUE;
/** Pointer to an VD I/O queue handle. */
typedef VDIOQUEUE *PVDIOQUEUE;

/** VD I/O queue request handle. */
typedef struct VDIOREQINT *VDIOREQ;
/** Pointer to an VD I/O queue request handle. */
typedef VDIOREQ *PVDIOREQ;

/** A I/O request ID. */
typedef uint64_t VDIOREQID;

/**
 * I/O request type.
 */
typedef enum VDIOREQTYPE
{
    /** Invalid request type. */
    VDIOREQTYPE_INVALID = 0,
    /** Read request. */
    VDIOREQTYPE_READ,
    /** Write request. */
    VDIOREQTYPE_WRITE,
    /** Flush request. */
    VDIOREQTYPE_FLUSH,
    /** Discard request. */
    VDIOREQTYPE_DISCARD,
    /** 32bit hack. */
    VDIOREQTYPE_32BIT_HACK = 0x7fffffff
} VDIOREQTYPE;
/** Pointer to a request type. */
typedef VDIOREQTYPE *PVDIOREQTYPE;

/**
 * I/O queue request completion callback.
 *
 * @param   hVdIoQueue      The VD I/O queue handle.
 * @param   pDisk           The disk the queue is attached to.
 * @param   hVdIoReq        The VD I/O request which completed.
 * @param   pvVdIoReq       Pointer to the allocator specific memory for this request.
 * @param   rcReq           The completion status code.
 */
typedef DECLCALLBACKTYPE(void, FNVDIOQUEUEREQCOMPLETE,(VDIOQUEUE hVdIoQueue, PVDISK pDisk,
                                                       VDIOREQ hVdIoReq,  void *pvVdIoReq, int rcReq));
/** Pointer to a VD I/O queue request completion callback. */
typedef FNVDIOQUEUEREQCOMPLETE *PFNVDIOQUEUEREQCOMPLETE;


/**
 * Creates a new I/O queue.
 *
 * @returns VBox status code.
 * @param   phVdIoQueue      Where to store the handle to the I/O queue on success.
 * @param   pfnIoReqComplete The completion handle to call when a request on the specified queue completes.
 * @param   cbIoReqAlloc     The extra amount of memory to allocate and associate with allocated requests
 *                           for use by the caller.
 * @param   iPriority        The priority of the queue from 0..UINT32_MAX. The lower the number the higher
 *                           the priority of the queue.
 */
VBOXDDU_DECL(int) VDIoQueueCreate(PVDIOQUEUE phVdIoQueue, PFNVDIOQUEUEREQCOMPLETE pfnIoReqComplete,
                                  size_t cbIoReqAlloc, uint32_t iPriority);

/**
 * Destroys the given I/O queue.
 *
 * @returns VBox status code.
 * @param   hVdIoQueue       The I/O queue handle.
 */
VBOXDDU_DECL(int) VDIoQueueDestroy(VDIOQUEUE hVdIoQueue);

/**
 * Attaches the given I/O queue to the given virtual disk container.
 *
 * @returns VBox status code.
 * @param   pDisk            The disk container handle.
 * @param   hVdIoQueue       The I/O queue to attach.
 */
VBOXDDU_DECL(int) VDIoQueueAttach(PVDISK pDisk, VDIOQUEUE hVdIoQueue);

/**
 * Detaches the given I/O queue from the currently attached disk container.
 *
 * @returns VBox status code.
 * @param   hVdIoQueue       The I/O queue.
 * @param   fPurge           Flag whether to cancel all active requests on this queue
 *                           before detaching.
 */
VBOXDDU_DECL(int) VDIoQueueDetach(VDIOQUEUE hVdIoQueue, bool fPurge);

/**
 * Purges all requests on the given queue.
 *
 * @returns VBox status code.
 * @param   hVdIoQueue       The I/O queue.
 */
VBOXDDU_DECL(int) VDIoQueuePurge(VDIOQUEUE hVdIoQueue);

/**
 * Allocates a new request from the given queue.
 *
 * @returns VBox status code.
 * @param   hVdIoQueue       The I/O queue.
 * @param   phVdIoReq        Where to store the handle of the request on success.
 * @param   ppvVdIoReq       Where to store the pointer to the allocator usable memory on success.
 * @param   uIoReqId         The request ID to assign to the request for canceling.
 */
VBOXDDU_DECL(int) VDIoQueueReqAlloc(VDIOQUEUE hVdIoQueue, PVDIOREQ phVdIoReq,
                                    void **ppvVdIoReq, VDIOREQID uIoReqId);

/**
 * Frees a given non active request.
 *
 * @returns VBox status code.
 * @param   hVdIoReq         The I/O request to free.
 */
VBOXDDU_DECL(int) VDIoQueueReqFree(VDIOREQ hVdIoReq);

/**
 * Cancels an active request by the given request ID.
 *
 * @returns VBox status code.
 * @param   hVdIoQueue       The I/O queue to cancel the request on.
 * @param   uIoReqId         The request ID.
 */
VBOXDDU_DECL(int) VDIoQueueReqCancelById(VDIOQUEUE hVdIoQueue, VDIOREQID uIoReqId);

/**
 * Cancels an active request by the given handle.
 *
 * @returns VBox status code.
 * @param   hVdIoReq         The I/O request handle to cancel.
 */
VBOXDDU_DECL(int) VDIoQueueReqCancelByHandle(VDIOREQ hVdIoReq);

/**
 * Submit a new request to the queue the request was allocated from.
 *
 * @returns VBox status code.
 * @param   hVdIoReq        The I/O request handle to submit.
 * @param   enmType         The type of the request.
 * @param   hVdIoIter       The iterator to use, NULL for flush requests.
 * @param   hVdIoBuf        The I/O buffer handle to use, NULL for flush and discard requests.
 */
VBOXDDU_DECL(int) VDIoQueueReqSubmit(VDIOREQ hVdIoReq, VDIOREQTYPE enmType,
                                     VDIOITER hVdIoIter, VDIOBUF hVdIoBuf);

/** @} */


RT_C_DECLS_END

/** @} */

#endif /* !VBOX_INCLUDED_vd_h */

