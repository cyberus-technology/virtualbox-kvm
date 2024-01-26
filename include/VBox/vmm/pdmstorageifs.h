/** @file
 * PDM - Pluggable Device Manager, Storage related interfaces.
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

#ifndef VBOX_INCLUDED_vmm_pdmstorageifs_h
#define VBOX_INCLUDED_vmm_pdmstorageifs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/sg.h>
#include <VBox/types.h>
#include <VBox/vdmedia.h>

RT_C_DECLS_BEGIN

struct PDMISECKEY;
struct PDMISECKEYHLP;


/** @defgroup grp_pdm_ifs_storage       PDM Storage Interfaces
 * @ingroup grp_pdm_interfaces
 * @{
 */


/** Pointer to a mount interface. */
typedef struct PDMIMOUNTNOTIFY *PPDMIMOUNTNOTIFY;
/**
 * Block interface (up).
 * Pair with PDMIMOUNT.
 */
typedef struct PDMIMOUNTNOTIFY
{
    /**
     * Called when a media is mounted.
     *
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  The emulation thread.
     */
    DECLR3CALLBACKMEMBER(void, pfnMountNotify,(PPDMIMOUNTNOTIFY pInterface));

    /**
     * Called when a media is unmounted
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  The emulation thread.
     */
    DECLR3CALLBACKMEMBER(void, pfnUnmountNotify,(PPDMIMOUNTNOTIFY pInterface));
} PDMIMOUNTNOTIFY;
/** PDMIMOUNTNOTIFY interface ID. */
#define PDMIMOUNTNOTIFY_IID                     "fa143ac9-9fc6-498e-997f-945380a558f9"


/** Pointer to mount interface. */
typedef struct PDMIMOUNT *PPDMIMOUNT;
/**
 * Mount interface (down).
 * Pair with PDMIMOUNTNOTIFY.
 */
typedef struct PDMIMOUNT
{
    /**
     * Unmount the media.
     *
     * The driver will validate and pass it on. On the rebounce it will decide whether or not to detach it self.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  The emulation thread.
     * @param   fForce          Force the unmount, even for locked media.
     * @param   fEject          Eject the medium. Only relevant for host drives.
     * @thread  The emulation thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnUnmount,(PPDMIMOUNT pInterface, bool fForce, bool fEject));

    /**
     * Checks if a media is mounted.
     *
     * @returns true if mounted.
     * @returns false if not mounted.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(bool, pfnIsMounted,(PPDMIMOUNT pInterface));

    /**
     * Locks the media, preventing any unmounting of it.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  The emulation thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnLock,(PPDMIMOUNT pInterface));

    /**
     * Unlocks the media, canceling previous calls to pfnLock().
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  The emulation thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnUnlock,(PPDMIMOUNT pInterface));

    /**
     * Checks if a media is locked.
     *
     * @returns true if locked.
     * @returns false if not locked.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(bool, pfnIsLocked,(PPDMIMOUNT pInterface));
} PDMIMOUNT;
/** PDMIMOUNT interface ID. */
#define PDMIMOUNT_IID                           "34fc7a4c-623a-4806-a6bf-5be1be33c99f"


/**
 * Callback which provides progress information.
 *
 * @return  VBox status code.
 * @param   pvUser          Opaque user data.
 * @param   uPercentage     Completion percentage.
 */
typedef DECLCALLBACKTYPE(int, FNSIMPLEPROGRESS,(void *pvUser, unsigned uPercentage));
/** Pointer to FNSIMPLEPROGRESS() */
typedef FNSIMPLEPROGRESS *PFNSIMPLEPROGRESS;


/**
 * Media type.
 */
typedef enum PDMMEDIATYPE
{
    /** Error (for the query function). */
    PDMMEDIATYPE_ERROR = 1,
    /** 360KB 5 1/4" floppy drive. */
    PDMMEDIATYPE_FLOPPY_360,
    /** 720KB 3 1/2" floppy drive. */
    PDMMEDIATYPE_FLOPPY_720,
    /** 1.2MB 5 1/4" floppy drive. */
    PDMMEDIATYPE_FLOPPY_1_20,
    /** 1.44MB 3 1/2" floppy drive. */
    PDMMEDIATYPE_FLOPPY_1_44,
    /** 2.88MB 3 1/2" floppy drive. */
    PDMMEDIATYPE_FLOPPY_2_88,
    /** Fake drive that can take up to 15.6 MB images.
     * C=255, H=2, S=63.  */
    PDMMEDIATYPE_FLOPPY_FAKE_15_6,
    /** Fake drive that can take up to 63.5 MB images.
     * C=255, H=2, S=255.  */
    PDMMEDIATYPE_FLOPPY_FAKE_63_5,
    /** CDROM drive. */
    PDMMEDIATYPE_CDROM,
    /** DVD drive. */
    PDMMEDIATYPE_DVD,
    /** Hard disk drive. */
    PDMMEDIATYPE_HARD_DISK
} PDMMEDIATYPE;

/** Check if the given block type is a floppy. */
#define PDMMEDIATYPE_IS_FLOPPY(a_enmType) ( (a_enmType) >= PDMMEDIATYPE_FLOPPY_360 && (a_enmType) <= PDMMEDIATYPE_FLOPPY_2_88 )

/**
 * Raw command data transfer direction.
 */
typedef enum PDMMEDIATXDIR
{
    PDMMEDIATXDIR_NONE = 0,
    PDMMEDIATXDIR_FROM_DEVICE,
    PDMMEDIATXDIR_TO_DEVICE
} PDMMEDIATXDIR;

/**
 * Media geometry structure.
 */
typedef struct PDMMEDIAGEOMETRY
{
    /** Number of cylinders. */
    uint32_t    cCylinders;
    /** Number of heads. */
    uint32_t    cHeads;
    /** Number of sectors. */
    uint32_t    cSectors;
} PDMMEDIAGEOMETRY;

/** Pointer to media geometry structure. */
typedef PDMMEDIAGEOMETRY *PPDMMEDIAGEOMETRY;
/** Pointer to constant media geometry structure. */
typedef const PDMMEDIAGEOMETRY *PCPDMMEDIAGEOMETRY;

/** Pointer to a media port interface. */
typedef struct PDMIMEDIAPORT *PPDMIMEDIAPORT;
/**
 * Media port interface (down).
 */
typedef struct PDMIMEDIAPORT
{
    /**
     * Returns the storage controller name, instance and LUN of the attached medium.
     *
     * @returns VBox status.
     * @param   pInterface      Pointer to this interface.
     * @param   ppcszController Where to store the name of the storage controller.
     * @param   piInstance      Where to store the instance number of the controller.
     * @param   piLUN           Where to store the LUN of the attached device.
     */
    DECLR3CALLBACKMEMBER(int, pfnQueryDeviceLocation, (PPDMIMEDIAPORT pInterface, const char **ppcszController,
                                                       uint32_t *piInstance, uint32_t *piLUN));


    /**
     * Queries the vendor and product ID and revision to report for INQUIRY commands in underlying devices, optional.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to this interface.
     * @param   ppszVendorId    Where to store the pointer to the vendor ID string to report.
     * @param   ppszProductId   Where to store the pointer to the product ID string to report.
     * @param   ppszRevision    Where to store the pointer to the revision string to report.
     *
     * @note The strings for the inquiry data are stored in the storage controller rather than in the device
     *       because if device attachments change (virtual CD/DVD drive versus host drive) there is currently no
     *       way to keep the INQUIRY data in extradata keys without causing trouble when the attachment is changed.
     *       Also Main currently doesn't has any settings for the attachment to store such information in the settings
     *       properly. Last reason (but not the most important one) is to stay compatible with older versions
     *       where the drive emulation was in AHCI but it now uses VSCSI and the settings overwrite should still work.
     */
    DECLR3CALLBACKMEMBER(int, pfnQueryScsiInqStrings, (PPDMIMEDIAPORT pInterface, const char **ppszVendorId,
                                                       const char **ppszProductId, const char **ppszRevision));

} PDMIMEDIAPORT;
/** PDMIMEDIAPORT interface ID. */
#define PDMIMEDIAPORT_IID                           "77180ab8-6485-454f-b440-efca322b7bd7"

/** Pointer to a media interface. */
typedef struct PDMIMEDIA *PPDMIMEDIA;
/**
 * Media interface (up).
 * Pairs with PDMIMEDIAPORT.
 */
typedef struct PDMIMEDIA
{
    /**
     * Read bits.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   off             Offset to start reading from. The offset must be aligned to a sector boundary.
     * @param   pvBuf           Where to store the read bits.
     * @param   cbRead          Number of bytes to read. Must be aligned to a sector boundary.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnRead,(PPDMIMEDIA pInterface, uint64_t off, void *pvBuf, size_t cbRead));

    /**
     * Read bits - version for DevPcBios.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   off             Offset to start reading from. The offset must be aligned to a sector boundary.
     * @param   pvBuf           Where to store the read bits.
     * @param   cbRead          Number of bytes to read. Must be aligned to a sector boundary.
     * @thread  Any thread.
     *
     * @note: Special version of pfnRead which doesn't try to suspend the VM when the DEKs for encrypted disks
     *        are missing but just returns an error.
     */
    DECLR3CALLBACKMEMBER(int, pfnReadPcBios,(PPDMIMEDIA pInterface, uint64_t off, void *pvBuf, size_t cbRead));

    /**
     * Write bits.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   off             Offset to start writing at. The offset must be aligned to a sector boundary.
     * @param   pvBuf           Where to store the write bits.
     * @param   cbWrite         Number of bytes to write. Must be aligned to a sector boundary.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnWrite,(PPDMIMEDIA pInterface, uint64_t off, const void *pvBuf, size_t cbWrite));

    /**
     * Make sure that the bits written are actually on the storage medium.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnFlush,(PPDMIMEDIA pInterface));

    /**
     * Send a raw command to the underlying device (CDROM).
     * This method is optional (i.e. the function pointer may be NULL).
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pbCdb           The command to process.
     * @param   cbCdb           The length of the command in bytes.
     * @param   enmTxDir        Direction of transfer.
     * @param   pvBuf           Pointer tp the transfer buffer.
     * @param   pcbBuf          Size of the transfer buffer.
     * @param   pabSense        Status of the command (when return value is VERR_DEV_IO_ERROR).
     * @param   cbSense         Size of the sense buffer in bytes.
     * @param   cTimeoutMillies Command timeout in milliseconds.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnSendCmd,(PPDMIMEDIA pInterface, const uint8_t *pbCdb, size_t cbCdb,
                                          PDMMEDIATXDIR enmTxDir, void *pvBuf, uint32_t *pcbBuf,
                                          uint8_t *pabSense, size_t cbSense, uint32_t cTimeoutMillies));

    /**
     * Merge medium contents during a live snapshot deletion. All details
     * must have been configured through CFGM or this will fail.
     * This method is optional (i.e. the function pointer may be NULL).
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pfnProgress     Function pointer for progress notification.
     * @param   pvUser          Opaque user data for progress notification.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnMerge,(PPDMIMEDIA pInterface, PFNSIMPLEPROGRESS pfnProgress, void *pvUser));

    /**
     * Sets the secret key retrieval interface to use to get secret keys.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pIfSecKey       The secret key interface to use.
     *                          Use NULL to clear the currently set interface and clear all secret
     *                          keys from the user.
     * @param   pIfSecKeyHlp    The secret key helper interface to use.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnSetSecKeyIf,(PPDMIMEDIA pInterface, struct PDMISECKEY *pIfSecKey,
                                              struct PDMISECKEYHLP *pIfSecKeyHlp));

    /**
     * Get the media size in bytes.
     *
     * @returns Media size in bytes.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(uint64_t, pfnGetSize,(PPDMIMEDIA pInterface));

    /**
     * Gets the media sector size in bytes.
     *
     * @returns Media sector size in bytes.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnGetSectorSize,(PPDMIMEDIA pInterface));

    /**
     * Check if the media is readonly or not.
     *
     * @returns true if readonly.
     * @returns false if read/write.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(bool, pfnIsReadOnly,(PPDMIMEDIA pInterface));

    /**
     * Returns whether the medium should be marked as rotational or not.
     *
     * @returns true if non rotating medium.
     * @returns false if rotating medium.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(bool, pfnIsNonRotational,(PPDMIMEDIA pInterface));

    /**
     * Get stored media geometry (physical CHS, PCHS) - BIOS property.
     * This is an optional feature of a media.
     *
     * @returns VBox status code.
     * @returns VERR_NOT_IMPLEMENTED if the media doesn't support storing the geometry.
     * @returns VERR_PDM_GEOMETRY_NOT_SET if the geometry hasn't been set using pfnBiosSetPCHSGeometry() yet.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pPCHSGeometry   Pointer to PCHS geometry (cylinders/heads/sectors).
     * @remark  This has no influence on the read/write operations.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnBiosGetPCHSGeometry,(PPDMIMEDIA pInterface, PPDMMEDIAGEOMETRY pPCHSGeometry));

    /**
     * Store the media geometry (physical CHS, PCHS) - BIOS property.
     * This is an optional feature of a media.
     *
     * @returns VBox status code.
     * @returns VERR_NOT_IMPLEMENTED if the media doesn't support storing the geometry.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pPCHSGeometry   Pointer to PCHS geometry (cylinders/heads/sectors).
     * @remark  This has no influence on the read/write operations.
     * @thread  The emulation thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnBiosSetPCHSGeometry,(PPDMIMEDIA pInterface, PCPDMMEDIAGEOMETRY pPCHSGeometry));

    /**
     * Get stored media geometry (logical CHS, LCHS) - BIOS property.
     * This is an optional feature of a media.
     *
     * @returns VBox status code.
     * @returns VERR_NOT_IMPLEMENTED if the media doesn't support storing the geometry.
     * @returns VERR_PDM_GEOMETRY_NOT_SET if the geometry hasn't been set using pfnBiosSetLCHSGeometry() yet.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pLCHSGeometry   Pointer to LCHS geometry (cylinders/heads/sectors).
     * @remark  This has no influence on the read/write operations.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnBiosGetLCHSGeometry,(PPDMIMEDIA pInterface, PPDMMEDIAGEOMETRY pLCHSGeometry));

    /**
     * Store the media geometry (logical CHS, LCHS) - BIOS property.
     * This is an optional feature of a media.
     *
     * @returns VBox status code.
     * @returns VERR_NOT_IMPLEMENTED if the media doesn't support storing the geometry.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pLCHSGeometry   Pointer to LCHS geometry (cylinders/heads/sectors).
     * @remark  This has no influence on the read/write operations.
     * @thread  The emulation thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnBiosSetLCHSGeometry,(PPDMIMEDIA pInterface, PCPDMMEDIAGEOMETRY pLCHSGeometry));

    /**
     * Checks if the device should be visible to the BIOS or not.
     *
     * @returns true if the device is visible to the BIOS.
     * @returns false if the device is not visible to the BIOS.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(bool, pfnBiosIsVisible,(PPDMIMEDIA pInterface));

    /**
     * Gets the media type.
     *
     * @returns media type.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(PDMMEDIATYPE, pfnGetType,(PPDMIMEDIA pInterface));

    /**
     * Gets the UUID of the media drive.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pUuid           Where to store the UUID on success.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetUuid,(PPDMIMEDIA pInterface, PRTUUID pUuid));

    /**
     * Discards the given range.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   paRanges        Array of ranges to discard.
     * @param   cRanges         Number of entries in the array.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnDiscard,(PPDMIMEDIA pInterface, PCRTRANGE paRanges, unsigned cRanges));

    /**
     * Returns the number of regions for the medium.
     *
     * @returns Number of regions.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnGetRegionCount,(PPDMIMEDIA pInterface));

    /**
     * Queries the properties for the given region.
     *
     * @returns VBox status code.
     * @retval  VERR_NOT_FOUND if the region index is not known.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   uRegion         The region index to query the properties of.
     * @param   pu64LbaStart    Where to store the starting LBA for the region on success.
     * @param   pcBlocks        Where to store the number of blocks for the region on success.
     * @param   pcbBlock        Where to store the size of one block in bytes on success.
     * @param   penmDataForm    WHere to store the data form for the region on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnQueryRegionProperties,(PPDMIMEDIA pInterface, uint32_t uRegion, uint64_t *pu64LbaStart,
                                                        uint64_t *pcBlocks, uint64_t *pcbBlock,
                                                        PVDREGIONDATAFORM penmDataForm));

    /**
     * Queries the properties for the region covering the given LBA.
     *
     * @returns VBox status code.
     * @retval  VERR_NOT_FOUND if the region index is not known.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   u64LbaStart     Where to store the starting LBA for the region on success.
     * @param   puRegion        Where to store the region number on success.
     * @param   pcBlocks        Where to store the number of blocks left in this region starting from the given LBA.
     * @param   pcbBlock        Where to store the size of one block in bytes on success.
     * @param   penmDataForm    WHere to store the data form for the region on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnQueryRegionPropertiesForLba,(PPDMIMEDIA pInterface, uint64_t u64LbaStart,
                                                              uint32_t *puRegion, uint64_t *pcBlocks,
                                                              uint64_t *pcbBlock, PVDREGIONDATAFORM penmDataForm));

} PDMIMEDIA;
/** PDMIMEDIA interface ID. */
#define PDMIMEDIA_IID                           "8ec68c48-dd20-4430-8386-f0d628a5aca6"


/**
 * Opaque I/O request handle.
 *
 * The specific content depends on the driver implementing this interface.
 */
typedef struct PDMMEDIAEXIOREQINT *PDMMEDIAEXIOREQ;
/** Pointer to an I/O request handle. */
typedef PDMMEDIAEXIOREQ *PPDMMEDIAEXIOREQ;
/** NIL I/O request handle. */
#define NIL_PDMMEDIAEXIOREQ     ((PDMMEDIAEXIOREQ)0)

/** A I/O request ID. */
typedef uint64_t PDMMEDIAEXIOREQID;

/**
 * I/O Request Type.
 */
typedef enum PDMMEDIAEXIOREQTYPE
{
    /** Invalid tpe. */
    PDMMEDIAEXIOREQTYPE_INVALID = 0,
    /** Flush request. */
    PDMMEDIAEXIOREQTYPE_FLUSH,
    /** Write request. */
    PDMMEDIAEXIOREQTYPE_WRITE,
    /** Read request. */
    PDMMEDIAEXIOREQTYPE_READ,
    /** Discard request. */
    PDMMEDIAEXIOREQTYPE_DISCARD,
    /** SCSI command. */
    PDMMEDIAEXIOREQTYPE_SCSI
} PDMMEDIAEXIOREQTYPE;
/** Pointer to a I/O request type. */
typedef PDMMEDIAEXIOREQTYPE *PPDMMEDIAEXIOREQTYPE;

/**
 * Data direction for raw SCSI commands.
 */
typedef enum PDMMEDIAEXIOREQSCSITXDIR
{
    /** Invalid data direction. */
    PDMMEDIAEXIOREQSCSITXDIR_INVALID     = 0,
    /** Direction is unknown. */
    PDMMEDIAEXIOREQSCSITXDIR_UNKNOWN,
    /** Direction is from device to host. */
    PDMMEDIAEXIOREQSCSITXDIR_FROM_DEVICE,
    /** Direction is from host to device. */
    PDMMEDIAEXIOREQSCSITXDIR_TO_DEVICE,
    /** No data transfer associated with this request. */
    PDMMEDIAEXIOREQSCSITXDIR_NONE,
    /** 32bit hack. */
    PDMMEDIAEXIOREQSCSITXDIR_32BIT_HACK  = 0x7fffffff
} PDMMEDIAEXIOREQSCSITXDIR;

/**
 * I/O request state.
 */
typedef enum PDMMEDIAEXIOREQSTATE
{
    /** Invalid state. */
    PDMMEDIAEXIOREQSTATE_INVALID = 0,
    /** The request is active and being processed. */
    PDMMEDIAEXIOREQSTATE_ACTIVE,
    /** The request is suspended due to an error and no processing will take place. */
    PDMMEDIAEXIOREQSTATE_SUSPENDED,
    /** 32bit hack. */
    PDMMEDIAEXIOREQSTATE_32BIT_HACK = 0x7fffffff
} PDMMEDIAEXIOREQSTATE;
/** Pointer to a I/O request state. */
typedef PDMMEDIAEXIOREQSTATE *PPDMMEDIAEXIOREQSTATE;

/** @name Supported feature flags
 * @{ */
/** I/O requests will execute asynchronously by default. */
#define PDMIMEDIAEX_FEATURE_F_ASYNC             RT_BIT_32(0)
/** The discard request is supported. */
#define PDMIMEDIAEX_FEATURE_F_DISCARD           RT_BIT_32(1)
/** The send raw SCSI command request is supported. */
#define PDMIMEDIAEX_FEATURE_F_RAWSCSICMD        RT_BIT_32(2)
/** Mask of valid flags. */
#define PDMIMEDIAEX_FEATURE_F_VALID             (PDMIMEDIAEX_FEATURE_F_ASYNC | PDMIMEDIAEX_FEATURE_F_DISCARD | PDMIMEDIAEX_FEATURE_F_RAWSCSICMD)
/** @} */

/** @name I/O request specific flags
 * @{ */
/** Default behavior (async I/O).*/
#define PDMIMEDIAEX_F_DEFAULT                    (0)
/** The I/O request will be executed synchronously. */
#define PDMIMEDIAEX_F_SYNC                       RT_BIT_32(0)
/** Whether to suspend the VM on a recoverable error with
 * an appropriate error message (disk full, etc.).
 * The request will be retried by the driver implementing the interface
 * when the VM resumes the next time. However before suspending the request
 * the owner of the request will be notified using the PDMMEDIAEXPORT::pfnIoReqStateChanged.
 * The same goes for resuming the request after the VM was resumed.
 */
#define PDMIMEDIAEX_F_SUSPEND_ON_RECOVERABLE_ERR RT_BIT_32(1)
 /** Mask of valid flags. */
#define PDMIMEDIAEX_F_VALID                      (PDMIMEDIAEX_F_SYNC | PDMIMEDIAEX_F_SUSPEND_ON_RECOVERABLE_ERR)
/** @} */

/** Pointer to an extended media notification interface. */
typedef struct PDMIMEDIAEXPORT *PPDMIMEDIAEXPORT;

/**
 * Asynchronous version of the media interface (up).
 * Pair with PDMIMEDIAEXPORT.
 */
typedef struct PDMIMEDIAEXPORT
{
    /**
     * Notify completion of a I/O request.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The I/O request handle.
     * @param   pvIoReqAlloc    The allocator specific memory for this request.
     * @param   rcReq           IPRT Status code of the completed request.
     *                          VERR_PDM_MEDIAEX_IOREQ_CANCELED if the request was canceled by a call to
     *                          PDMIMEDIAEX::pfnIoReqCancel.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqCompleteNotify, (PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                       void *pvIoReqAlloc, int rcReq));

    /**
     * Copy data from the memory buffer of the caller to the callees memory buffer for the given request.
     *
     * @returns VBox status code.
     * @retval  VERR_PDM_MEDIAEX_IOBUF_OVERFLOW if there is not enough room to store the data.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The I/O request handle.
     * @param   pvIoReqAlloc    The allocator specific memory for this request.
     * @param   offDst          The destination offset from the start to write the data to.
     * @param   pSgBuf          The S/G buffer to read the data from.
     * @param   cbCopy          How many bytes to copy.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqCopyFromBuf, (PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                    void *pvIoReqAlloc, uint32_t offDst, PRTSGBUF pSgBuf,
                                                    size_t cbCopy));

    /**
     * Copy data to the memory buffer of the caller from the callees memory buffer for the given request.
     *
     * @returns VBox status code.
     * @retval  VERR_PDM_MEDIAEX_IOBUF_UNDERRUN if there is not enough data to copy from the buffer.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The I/O request handle.
     * @param   pvIoReqAlloc    The allocator specific memory for this request.
     * @param   offSrc          The offset from the start of the buffer to read the data from.
     * @param   pSgBuf          The S/G buffer to write the data to.
     * @param   cbCopy          How many bytes to copy.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqCopyToBuf, (PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                  void *pvIoReqAlloc, uint32_t offSrc, PRTSGBUF pSgBuf,
                                                  size_t cbCopy));

    /**
     * Queries a pointer to the memory buffer for the request from the drive/device above.
     *
     * @returns VBox status code.
     * @retval  VERR_NOT_SUPPORTED if this is not supported for this request.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The I/O request handle.
     * @param   pvIoReqAlloc    The allocator specific memory for this request.
     * @param   ppvBuf          Where to store the pointer to the guest buffer on success.
     * @param   pcbBuf          Where to store the size of the buffer on success.
     *
     * @note This is an optional feature of the entity implementing this interface to avoid overhead
     *       by copying the data between buffers. If NULL it is not supported at all and the caller
     *       has to resort to PDMIMEDIAEXPORT::pfnIoReqCopyToBuf and PDMIMEDIAEXPORT::pfnIoReqCopyFromBuf.
     *       The same holds when VERR_NOT_SUPPORTED is returned.
     *
     *       On the upside the caller of this interface might not call this method at all and just
     *       use the before mentioned methods to copy the data between the buffers.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqQueryBuf, (PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                 void *pvIoReqAlloc, void **ppvBuf, size_t *pcbBuf));

    /**
     * Queries the specified amount of ranges to discard from the callee for the given I/O request.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The I/O request handle.
     * @param   pvIoReqAlloc    The allocator specific memory for this request.
     * @param   idxRangeStart   The range index to start with.
     * @param   cRanges         How man ranges can be stored in the provided array.
     * @param   paRanges        Where to store the ranges on success.
     * @param   *pcRanges       Where to store the number of ranges copied over on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqQueryDiscardRanges, (PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                           void *pvIoReqAlloc, uint32_t idxRangeStart,
                                                           uint32_t cRanges, PRTRANGE paRanges,
                                                           uint32_t *pcRanges));

    /**
     * Notify the request owner about a state change for the request.
     *
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The I/O request handle.
     * @param   pvIoReqAlloc    The allocator specific memory for this request.
     * @param   enmState        The new state of the request.
     */
    DECLR3CALLBACKMEMBER(void, pfnIoReqStateChanged, (PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                      void *pvIoReqAlloc, PDMMEDIAEXIOREQSTATE enmState));

    /**
     * Informs the device that the underlying medium was ejected.
     *
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     */
    DECLR3CALLBACKMEMBER(void, pfnMediumEjected, (PPDMIMEDIAEXPORT pInterface));

} PDMIMEDIAEXPORT;

/** PDMIMEDIAAEXPORT interface ID. */
#define PDMIMEDIAEXPORT_IID                  "0ae2e534-6c28-41d6-9a88-7f88f2cb2ff8"


/** Pointer to an extended media interface. */
typedef struct PDMIMEDIAEX *PPDMIMEDIAEX;

/**
 * Extended version of PDMIMEDIA (down).
 * Pair with PDMIMEDIAEXPORT.
 */
typedef struct PDMIMEDIAEX
{
    /**
     * Queries the features supported by the entity implementing this interface.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pfFeatures      Where to store the supported feature flags on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnQueryFeatures, (PPDMIMEDIAEX pInterface, uint32_t *pfFeatures));

    /**
     * Notifies the driver below that the device received a suspend notification.
     *
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     *
     * @note this is required because the PDM drivers in the storage area usually get their suspend notification
     *       only after the device finished suspending. For some cases it is useful for the driver to know
     *       as early as possible that a suspend is in progress to stop issuing deferred requests or other things.
     */
    DECLR3CALLBACKMEMBER(void, pfnNotifySuspend, (PPDMIMEDIAEX pInterface));

    /**
     * Sets the size of the allocator specific memory for a I/O request.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   cbIoReqAlloc    The size of the allocator specific memory in bytes.
     * @thread  EMT.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqAllocSizeSet, (PPDMIMEDIAEX pInterface, size_t cbIoReqAlloc));

    /**
     * Allocates a new I/O request.
     *
     * @returns VBox status code.
     * @retval  VERR_PDM_MEDIAEX_IOREQID_CONFLICT if the ID belongs to a still active request.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   phIoReq         Where to store the handle to the new I/O request on success.
     * @param   ppvIoReqAlloc   Where to store the pointer to the allocator specific memory on success.
     *                          NULL if the memory size was not set or set to 0.
     * @param   uIoReqId        A custom request ID which can be used to cancel the request.
     * @param   fFlags          A combination of PDMIMEDIAEX_F_* flags.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqAlloc, (PPDMIMEDIAEX pInterface, PPDMMEDIAEXIOREQ phIoReq, void **ppvIoReqAlloc,
                                              PDMMEDIAEXIOREQID uIoReqId, uint32_t fFlags));

    /**
     * Frees a given I/O request.
     *
     * @returns VBox status code.
     * @retval  VERR_PDM_MEDIAEX_IOREQ_INVALID_STATE if the given request is still active.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The I/O request to free.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqFree, (PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq));

    /**
     * Queries the residual amount of data not transfered when the request completed.
     *
     * @returns VBox status code.
     * @retval  VERR_PDM_MEDIAEX_IOREQ_INVALID_STATE has not completed yet.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The I/O request.
     * @param   pcbResidual     Where to store the amount of resdiual data in bytes.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqQueryResidual, (PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq, size_t *pcbResidual));

    /**
     * Queries the residual amount of data not transfered when the request completed.
     *
     * @returns VBox status code.
     * @retval  VERR_PDM_MEDIAEX_IOREQ_INVALID_STATE has not completed yet.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The I/O request.
     * @param   pcbXfer         Where to store the amount of resdiual data in bytes.
     * @thread  Any thread.
     *
     * @note For simple read/write requests this returns the amount to read/write as given to the
     *       PDMIMEDIAEX::pfnIoReqRead or PDMIMEDIAEX::pfnIoReqWrite call.
     *       For SCSI commands this returns the transfer size as given in the provided CDB.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqQueryXferSize, (PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq, size_t *pcbXfer));

    /**
     * Cancels all I/O active requests.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqCancelAll, (PPDMIMEDIAEX pInterface));

    /**
     * Cancels a I/O request identified by the ID.
     *
     * @returns VBox status code.
     * @retval  VERR_PDM_MEDIAEX_IOREQID_NOT_FOUND if the given ID could not be found in the active request list.
     *          (The request has either completed already or an invalid ID was given).
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   uIoReqId        The I/O request ID
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqCancel, (PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQID uIoReqId));

    /**
     * Start a reading request.
     *
     * @returns VBox status code.
     * @retval  VERR_PDM_MEDIAEX_IOREQ_CANCELED if the request was canceled  by a call to
     *          PDMIMEDIAEX::pfnIoReqCancel.
     * @retval  VINF_PDM_MEDIAEX_IOREQ_IN_PROGRESS if the request was successfully submitted but is still in progress.
     *          Completion will be notified through PDMIMEDIAEXPORT::pfnIoReqCompleteNotify with the appropriate status code.
     * @retval  VINF_SUCCESS if the request completed successfully.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The I/O request to associate the read with.
     * @param   off             Offset to start reading from. Must be aligned to a sector boundary.
     * @param   cbRead          Number of bytes to read. Must be aligned to a sector boundary.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqRead, (PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq, uint64_t off, size_t cbRead));

    /**
     * Start a writing request.
     *
     * @returns VBox status code.
     * @retval  VERR_PDM_MEDIAEX_IOREQ_CANCELED if the request was canceled  by a call to
     *          PDMIMEDIAEX::pfnIoReqCancel.
     * @retval  VINF_PDM_MEDIAEX_IOREQ_IN_PROGRESS if the request was successfully submitted but is still in progress.
     *          Completion will be notified through PDMIMEDIAEXPORT::pfnIoReqCompleteNotify with the appropriate status code.
     * @retval  VINF_SUCCESS if the request completed successfully.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The I/O request to associate the write with.
     * @param   off             Offset to start reading from. Must be aligned to a sector boundary.
     * @param   cbWrite         Number of bytes to write. Must be aligned to a sector boundary.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqWrite, (PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq, uint64_t off, size_t cbWrite));

    /**
     * Flush everything to disk.
     *
     * @returns VBox status code.
     * @retval  VERR_PDM_MEDIAEX_IOREQ_CANCELED if the request was canceled  by a call to
     *          PDMIMEDIAEX::pfnIoReqCancel.
     * @retval  VINF_PDM_MEDIAEX_IOREQ_IN_PROGRESS if the request was successfully submitted but is still in progress.
     *          Completion will be notified through PDMIMEDIAEXPORT::pfnIoReqCompleteNotify with the appropriate status code.
     * @retval  VINF_SUCCESS if the request completed successfully.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The I/O request to associate the flush with.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqFlush, (PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq));

    /**
     * Discards the given range.
     *
     * @returns VBox status code.
     * @retval  VERR_PDM_MEDIAEX_IOREQ_CANCELED if the request was canceled  by a call to
     *          PDMIMEDIAEX::pfnIoReqCancel.
     * @retval  VINF_PDM_MEDIAEX_IOREQ_IN_PROGRESS if the request was successfully submitted but is still in progress.
     *          Completion will be notified through PDMIMEDIAEXPORT::pfnIoReqCompleteNotify with the appropriate status code.
     * @retval  VINF_SUCCESS if the request completed successfully.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The I/O request to associate the discard with.
     * @param   cRangesMax      The maximum number of ranges this request has associated, this must not be accurate
     *                          but can actually be bigger than the amount of ranges actually available.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqDiscard, (PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq, unsigned cRangesMax));

    /**
     * Send a raw command to the underlying device (CDROM).
     *
     * @returns VBox status code.
     * @retval  VERR_PDM_MEDIAEX_IOREQ_CANCELED if the request was canceled  by a call to
     *          PDMIMEDIAEX::pfnIoReqCancel.
     * @retval  VINF_PDM_MEDIAEX_IOREQ_IN_PROGRESS if the request was successfully submitted but is still in progress.
     *          Completion will be notified through PDMIMEDIAEXPORT::pfnIoReqCompleteNotify with the appropriate status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The I/O request to associate the command with.
     * @param   uLun            The LUN the command is for.
     * @param   pbCdb           The SCSI CDB containing the command.
     * @param   cbCdb           Size of the CDB in bytes.
     * @param   enmTxDir        Direction of transfer.
     * @param   penmTxDirRet    Where to store the transfer direction as parsed from the CDB, optional.
     * @param   cbBuf           Size of the transfer buffer.
     * @param   pabSense        Where to store the optional sense key.
     * @param   cbSense         Size of the sense key buffer.
     * @param   pcbSenseRet     Where to store the amount of sense data written, optional.
     * @param   pu8ScsiSts      Where to store the SCSI status on success.
     * @param   cTimeoutMillies Command timeout in milliseconds.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqSendScsiCmd,(PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                   uint32_t uLun, const uint8_t *pbCdb, size_t cbCdb,
                                                   PDMMEDIAEXIOREQSCSITXDIR enmTxDir, PDMMEDIAEXIOREQSCSITXDIR *penmTxDirRet,
                                                   size_t cbBuf, uint8_t *pabSense, size_t cbSense, size_t *pcbSenseRet,
                                                   uint8_t *pu8ScsiSts, uint32_t cTimeoutMillies));

    /**
     * Returns the number of active I/O requests.
     *
     * @returns Number of active I/O requests.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnIoReqGetActiveCount, (PPDMIMEDIAEX pInterface));

    /**
     * Returns the number of suspended requests.
     *
     * @returns Number of suspended I/O requests.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @thread  Any thread.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnIoReqGetSuspendedCount, (PPDMIMEDIAEX pInterface));

    /**
     * Gets the first suspended request handle.
     *
     * @returns VBox status code.
     * @retval  VERR_NOT_FOUND if there is no suspended request waiting.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   phIoReq         Where to store the request handle on success.
     * @param   ppvIoReqAlloc   Where to store the pointer to the allocator specific memory on success.
     * @thread  Any thread.
     *
     * @note This should only be called when the VM is suspended to make sure the request doesn't suddenly
     *       changes into the active state again. The only purpose for this method for now is to make saving the state
     *       possible without breaking saved state versions.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqQuerySuspendedStart, (PPDMIMEDIAEX pInterface, PPDMMEDIAEXIOREQ phIoReq, void **ppvIoReqAlloc));

    /**
     * Gets the next suspended request handle.
     *
     * @returns VBox status code.
     * @retval  VERR_NOT_FOUND if there is no suspended request waiting.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   hIoReq          The current request handle.
     * @param   phIoReqNext     Where to store the request handle on success.
     * @param   ppvIoReqAllocNext Where to store the pointer to the allocator specific memory on success.
     * @thread  Any thread.
     *
     * @note This should only be called when the VM is suspended to make sure the request doesn't suddenly
     *       changes into the active state again. The only purpose for this method for now is to make saving the state
     *       possible without breaking saved state versions.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqQuerySuspendedNext, (PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                           PPDMMEDIAEXIOREQ phIoReqNext, void **ppvIoReqAllocNext));

    /**
     * Saves the given I/O request state in the provided saved state unit.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pSSM            The SSM handle.
     * @param   hIoReq          The request handle to save.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqSuspendedSave, (PPDMIMEDIAEX pInterface, PSSMHANDLE pSSM, PDMMEDIAEXIOREQ hIoReq));

    /**
     * Load a suspended request state from the given saved state unit and link it into the suspended list.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the interface structure containing the called function pointer.
     * @param   pSSM            The SSM handle to read the state from.
     * @param   hIoReq          The request handle to load the state into.
     */
    DECLR3CALLBACKMEMBER(int, pfnIoReqSuspendedLoad, (PPDMIMEDIAEX pInterface, PSSMHANDLE pSSM, PDMMEDIAEXIOREQ hIoReq));

} PDMIMEDIAEX;
/** PDMIMEDIAEX interface ID. */
#define PDMIMEDIAEX_IID                      "29c9e82b-934e-45c5-bb84-0d871c3cc9dd"

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_pdmstorageifs_h */
