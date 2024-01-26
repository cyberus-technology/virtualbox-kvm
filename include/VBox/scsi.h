/** @file
 * VirtualBox - SCSI declarations. (DEV,+)
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

#ifndef VBOX_INCLUDED_scsi_h
#define VBOX_INCLUDED_scsi_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/assert.h>

/**
 * @todo: Remove when the splitting code was removed from DevATA.
 *        The limit doesn't belong here but is specific for each host platform.
 */
#ifdef RT_OS_FREEBSD
/* The cam subsystem doesn't allow more */
# define SCSI_MAX_BUFFER_SIZE (64  * _1K)
#else
# define SCSI_MAX_BUFFER_SIZE (100 * _1K)
#endif

/**
 * SCSI command opcode identifiers.
 *
 * SCSI-3, so far for CD/DVD Logical Units, from Table 49 of the MMC-3 draft standard.
 */
typedef enum SCSICMD
{
    SCSI_BLANK                          = 0xa1,
    SCSI_CLOSE_TRACK_SESSION            = 0x5b,
    SCSI_ERASE_10                       = 0x2c,
    SCSI_FORMAT_UNIT                    = 0x04,
    SCSI_GET_CONFIGURATION              = 0x46,
    SCSI_GET_EVENT_STATUS_NOTIFICATION  = 0x4a,
    SCSI_GET_PERFORMANCE                = 0xac,
    /** Inquiry command. */
    SCSI_INQUIRY                        = 0x12,
    SCSI_LOAD_UNLOAD_MEDIUM             = 0xa6,
    SCSI_MECHANISM_STATUS               = 0xbd,
    SCSI_MODE_SELECT_10                 = 0x55,
    SCSI_MODE_SENSE_10                  = 0x5a,
    SCSI_PAUSE_RESUME                   = 0x4b,
    SCSI_PLAY_AUDIO_10                  = 0x45,
    SCSI_PLAY_AUDIO_12                  = 0xa5,
    SCSI_PLAY_AUDIO_MSF                 = 0x47,
    SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL   = 0x1e,
    /** Read(10) command. */
    SCSI_READ_10                        = 0x28,
    SCSI_READ_12                        = 0xa8,
    SCSI_READ_BUFFER                    = 0x3c,
    SCSI_READ_BUFFER_CAPACITY           = 0x5c,
    /** Read Capacity(6) command. */
    SCSI_READ_CAPACITY                  = 0x25,
    SCSI_READ_CD                        = 0xbe,
    SCSI_READ_CD_MSF                    = 0xb9,
    SCSI_READ_DISC_INFORMATION          = 0x51,
    SCSI_READ_DVD_STRUCTURE             = 0xad,
    SCSI_READ_FORMAT_CAPACITIES         = 0x23,
    SCSI_READ_SUBCHANNEL                = 0x42,
    SCSI_READ_TOC_PMA_ATIP              = 0x43,
    SCSI_READ_TRACK_INFORMATION         = 0x52,
    SCSI_REPAIR_TRACK                   = 0x58,
    SCSI_REPORT_KEY                     = 0xa4,
    SCSI_REQUEST_SENSE                  = 0x03,
    SCSI_RESERVE_TRACK                  = 0x53,
    SCSI_SCAN                           = 0xba,
    SCSI_SEEK_10                        = 0x2b,
    SCSI_SEND_CUE_SHEET                 = 0x5d,
    SCSI_SEND_DVD_STRUCTURE             = 0xbf,
    SCSI_SEND_EVENT                     = 0xa2,
    SCSI_SEND_KEY                       = 0xa3,
    SCSI_SEND_OPC_INFORMATION           = 0x54,
    SCSI_SET_CD_SPEED                   = 0xbb,
    SCSI_SET_READ_AHEAD                 = 0xa7,
    SCSI_SET_STREAMING                  = 0xb6,
    SCSI_START_STOP_UNIT                = 0x1b,
    SCSI_LOAD_UNLOAD                    = 0x1b,
    SCSI_STOP_PLAY_SCAN                 = 0x4e,
    /** Synchronize Cache command. */
    SCSI_SYNCHRONIZE_CACHE              = 0x35,
    SCSI_TEST_UNIT_READY                = 0x00,
    SCSI_VERIFY_10                      = 0x2f,
    /** Write(10) command. */
    SCSI_WRITE_10                       = 0x2a,
    SCSI_WRITE_12                       = 0xaa,
    SCSI_WRITE_AND_VERIFY_10            = 0x2e,
    SCSI_WRITE_BUFFER                   = 0x3b,

    /** Mode Select(6) command */
    SCSI_MODE_SELECT_6                  = 0x15,
    /** Mode Sense(6) command */
    SCSI_MODE_SENSE_6                   = 0x1a,
    /** Report LUNs command. */
    SCSI_REPORT_LUNS                    = 0xa0,
    SCSI_REPORT_DENSITY                 = 0x44,
    /** Rezero Unit command. Obsolete for ages now, but used by cdrecord. */
    SCSI_REZERO_UNIT                    = 0x01,
    SCSI_REWIND                         = 0x01,
    SCSI_SERVICE_ACTION_IN_16           = 0x9e,
    SCSI_READ_16                        = 0x88,
    SCSI_WRITE_16                       = 0x8a,
    SCSI_READ_6                         = 0x08,
    SCSI_WRITE_6                        = 0x0a,
    SCSI_LOG_SENSE                      = 0x4d,
    SCSI_UNMAP                          = 0x42,
    SCSI_RESERVE_6                      = 0x16,
    SCSI_RELEASE_6                      = 0x17,
    SCSI_RESERVE_10                     = 0x56,
    SCSI_RELEASE_10                     = 0x57,
    SCSI_READ_BLOCK_LIMITS              = 0x05,
    SCSI_MAINTENANCE_IN                 = 0xa3
} SCSICMD;

/**
 * Service action in opcode identifiers
 */
typedef enum SCSISVCACTIONIN
{
    SCSI_SVC_ACTION_IN_READ_CAPACITY_16 = 0x10
} SCSISVCACTIONIN;

/**
 * Maintenance in opcode identifiers
 */
typedef enum SCSIMAINTENANCEIN
{
    SCSI_MAINTENANCE_IN_REPORT_SUPP_OPC = 0x0c
} SCSIMAINTENANCEIN;

/* Mode page codes for mode sense/select commands. */
#define SCSI_MODEPAGE_ERROR_RECOVERY   0x01
#define SCSI_MODEPAGE_WRITE_PARAMETER  0x05
#define SCSI_MODEPAGE_CD_STATUS        0x2a


/* Page control codes. */
#define SCSI_PAGECONTROL_CURRENT        0x00
#define SCSI_PAGECONTROL_CHANGEABLE     0x01
#define SCSI_PAGECONTROL_DEFAULT        0x02
#define SCSI_PAGECONTROL_SAVED          0x03


/* Status codes */
#define SCSI_STATUS_OK                          0x00
#define SCSI_STATUS_CHECK_CONDITION             0x02
#define SCSI_STATUS_CONDITION_MET               0x04
#define SCSI_STATUS_BUSY                        0x08
#define SCSI_STATUS_INTERMEDIATE                0x10
#define SCSI_STATUS_DATA_UNDEROVER_RUN          0x12
#define SCSI_STATUS_INTERMEDIATE_CONDITION_MET  0x14
#define SCSI_STATUS_RESERVATION_CONFLICT        0x18
#define SCSI_STATUS_COMMAND_TERMINATED          0x22
#define SCSI_STATUS_QUEUE_FULL                  0x28
#define SCSI_STATUS_ACA_ACTIVE                  0x30
#define SCSI_STATUS_TASK_ABORTED                0x40

/* Sense data response codes - This is the first byte in the sense data */
#define SCSI_SENSE_RESPONSE_CODE_CURR_FIXED     0x70
#define SCSI_SENSE_RESPONSE_CODE_DEFERRED_FIXED 0x71
#define SCSI_SENSE_RESPONSE_CODE_CURR_DESC      0x72
#define SCSI_SENSE_RESPONSE_CODE_DEFERRED_DESC  0x73

/* Sense keys */
#define SCSI_SENSE_NONE             0
#define SCSI_SENSE_RECOVERED_ERROR  1
#define SCSI_SENSE_NOT_READY        2
#define SCSI_SENSE_MEDIUM_ERROR     3
#define SCSI_SENSE_HARDWARE_ERROR   4
#define SCSI_SENSE_ILLEGAL_REQUEST  5
#define SCSI_SENSE_UNIT_ATTENTION   6
#define SCSI_SENSE_DATA_PROTECT     7
#define SCSI_SENSE_BLANK_CHECK      8
#define SCSI_SENSE_VENDOR_SPECIFIC  9
#define SCSI_SENSE_COPY_ABORTED     10
#define SCSI_SENSE_ABORTED_COMMAND  11
#define SCSI_SENSE_VOLUME_OVERFLOW  13
#define SCSI_SENSE_MISCOMPARE       14

/* Additional sense bit flags (to be ORed with sense key). */
#define SCSI_SENSE_FLAG_FILEMARK    0x80
#define SCSI_SENSE_FLAG_EOM         0x40
#define SCSI_SENSE_FLAG_ILI         0x20

/* Additional sense keys */
#define SCSI_ASC_NONE                                       0x00
#define SCSI_ASC_WRITE_ERROR                                0x0c
#define SCSI_ASC_READ_ERROR                                 0x11
#define SCSI_ASC_ILLEGAL_OPCODE                             0x20
#define SCSI_ASC_LOGICAL_BLOCK_OOR                          0x21
#define SCSI_ASC_INV_FIELD_IN_CMD_PACKET                    0x24
#define SCSI_ASC_LOGICAL_UNIT_NOT_SUPPORTED                 0x25
#define SCSI_ASC_WRITE_PROTECTED                            0x27
#define SCSI_ASC_MEDIUM_MAY_HAVE_CHANGED                    0x28
#define SCSI_ASC_POWER_ON_RESET_BUS_DEVICE_RESET_OCCURRED   0x29
#define SCSI_ASC_CANNOT_READ_MEDIUM                         0x30
#define SCSI_ASC_MEDIUM_NOT_PRESENT                         0x3a
#define SCSI_ASC_SAVING_PARAMETERS_NOT_SUPPORTED            0x39
#define SCSI_ASC_INTERNAL_TARGET_FAILURE                    0x44
#define SCSI_ASC_INVALID_MESSAGE                            0x49
#define SCSI_ASC_MEDIA_LOAD_OR_EJECT_FAILED                 0x53
#define SCSI_ASC_LOGICAL_UNIT_DOES_NOT_RESPOND_TO_SELECTION 0x00
#define SCSI_ASC_SYSTEM_RESOURCE_FAILURE                    0x55
#define SCSI_ASC_ILLEGAL_MODE_FOR_THIS_TRACK                0x64
#define SCSI_ASC_COMMAND_TO_LOGICAL_UNIT_FAILED             0x6E

/** Additional sense code qualifiers (ASCQ). */
/* NB: The ASC/ASCQ combination determines the full meaning. */
#define SCSI_ASCQ_SYSTEM_BUFFER_FULL                        0x01
#define SCSI_ASCQ_POWER_ON_RESET_BUS_DEVICE_RESET_OCCURRED  0x00
#define SCSI_ASCQ_END_OF_DATA_DETECTED                      0x05
#define SCSI_ASCQ_FILEMARK_DETECTED                         0x01
#define SCSI_ASCQ_EOP_EOM_DETECTED                          0x02
#define SCSI_ASCQ_SETMARK_DETECTED                          0x03
#define SCSI_ASCQ_BOP_BOM_DETECTED                          0x04
#define SCSI_ASCQ_UNKNOWN_FORMAT                            0x01
#define SCSI_ASCQ_INCOMPATIBLE_FORMAT                       0x02
#define SCSI_ASCQ_COPY_TARGET_DEVICE_DATA_OVERRUN           0x0d

/** @name SCSI_INQUIRY
 * @{
 */

/** Length of the SCSI INQUIRY vendor identifier (without termination). */
#define SCSI_INQUIRY_VENDOR_ID_LENGTH   8
/** Length of the SCSI INQUIRY product identifier (without termination). */
#define SCSI_INQUIRY_PRODUCT_ID_LENGTH 16
/** Length of the SCSI INQUIRY revision identifier (without termination). */
#define SCSI_INQUIRY_REVISION_LENGTH    4

#pragma pack(1)
typedef struct SCSIINQUIRYCDB
{
    unsigned u8Cmd : 8;
    unsigned fEVPD : 1;
    unsigned u4Reserved : 4;
    unsigned u3LUN : 3;
    unsigned u8PageCode : 8;
    unsigned u8Reserved : 8;
    uint8_t cbAlloc;
    uint8_t u8Control;
} SCSIINQUIRYCDB;
#pragma pack()
AssertCompileSize(SCSIINQUIRYCDB, 6);
typedef SCSIINQUIRYCDB *PSCSIINQUIRYCDB;
typedef const SCSIINQUIRYCDB *PCSCSIINQUIRYCDB;

#pragma pack(1)
typedef struct SCSIINQUIRYDATA
{
    unsigned u5PeripheralDeviceType : 5;                    /**< 0x00 / 00 */
    unsigned u3PeripheralQualifier : 3;
    unsigned u6DeviceTypeModifier : 7;                      /**< 0x01 */
    unsigned fRMB : 1;
    unsigned u3AnsiVersion : 3;                             /**< 0x02 */
    unsigned u3EcmaVersion : 3;
    unsigned u2IsoVersion : 2;
    unsigned u4ResponseDataFormat : 4;                      /**< 0x03 */
    unsigned u2Reserved0 : 2;
    unsigned fTrmlOP : 1;
    unsigned fAEC : 1;
    unsigned cbAdditional : 8;                              /**< 0x04 */
    unsigned u8Reserved1 : 8;                               /**< 0x05 */
    unsigned u8Reserved2 : 8;                               /**< 0x06 */
    unsigned fSftRe : 1;                                    /**< 0x07 */
    unsigned fCmdQue : 1;
    unsigned fReserved3 : 1;
    unsigned fLinked : 1;
    unsigned fSync : 1;
    unsigned fWBus16 : 1;
    unsigned fWBus32 : 1;
    unsigned fRelAdr : 1;
    int8_t   achVendorId[SCSI_INQUIRY_VENDOR_ID_LENGTH];    /**< 0x08 */
    int8_t   achProductId[SCSI_INQUIRY_PRODUCT_ID_LENGTH];  /**< 0x10 */
    int8_t   achProductLevel[SCSI_INQUIRY_REVISION_LENGTH]; /**< 0x20 */
    uint8_t  abVendorSpecific[20];                          /**< 0x24/36 - Optional it seems. */
    uint8_t  abReserved4[40];
    uint8_t  abVendorSpecificParameters[1];                 /**< 0x60/96 - Variable size. */
} SCSIINQUIRYDATA;
#pragma pack()
AssertCompileSize(SCSIINQUIRYDATA, 97);
typedef SCSIINQUIRYDATA *PSCSIINQUIRYDATA;
typedef const SCSIINQUIRYDATA *PCSCSIINQUIRYDATA;

#define SCSI_INQUIRY_DATA_PERIPHERAL_QUALIFIER_CONNECTED                   0x00
#define SCSI_INQUIRY_DATA_PERIPHERAL_QUALIFIER_NOT_CONNECTED_BUT_SUPPORTED 0x01
#define SCSI_INQUIRY_DATA_PERIPHERAL_QUALIFIER_NOT_CONNECTED_NOT_SUPPORTED 0x03

#define SCSI_INQUIRY_DATA_PERIPHERAL_DEVICE_TYPE_DIRECT_ACCESS     0x00
#define SCSI_INQUIRY_DATA_PERIPHERAL_DEVICE_TYPE_SEQUENTIAL_ACCESS 0x01
#define SCSI_INQUIRY_DATA_PERIPHERAL_DEVICE_TYPE_CD_DVD            0x05
#define SCSI_INQUIRY_DATA_PERIPHERAL_DEVICE_TYPE_UNKNOWN           0x1f

/** @} */

#if defined(IN_RING3) && (defined(LOG_ENABLED) || defined(RT_STRICT))
const char * SCSICmdText(uint8_t uCmd);
const char * SCSIStatusText(uint8_t uStatus);
const char * SCSISenseText(uint8_t uSense);
const char * SCSISenseExtText(uint8_t uASC, uint8_t uASCQ);
int SCSILogModePage(char *pszBuf, size_t cchBuffer, uint8_t *pbModePage,
                    size_t cbModePage);
int SCSILogCueSheet(char *pszBuf, size_t cchBuffer, uint8_t *pbCueSheet,
                    size_t cbCueSheet);
#endif

#endif /* !VBOX_INCLUDED_scsi_h */
