/** @file
 * IPRT - Hierarchical File System (HFS).
 */

/*
 * Copyright (C) 2009-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_formats_hfs_h
#define IPRT_INCLUDED_formats_hfs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


#include <iprt/types.h>
#include <iprt/assertcompile.h>


/** @defgroup grp_rt_fmt_hfs    HFS - Hierarchical File System.
 * @{
 */


/** @name HFS signature words (HFSPlusVolumeHeader::signature)
 * @{ */
#define kHFSSigWord                     UINT16_C(0x4244)
#define kHFSPlusSigWord                 UINT16_C(0x482b)
#define kHFSXSigWord                    UINT16_C(0x4858)
/** @}  */

/** @name HFS version numbers (HFSPlusVolumeHeader::version).
 * @{  */
#define kHFSPlusVersion                 UINT16_C(4)
#define kHFSXVersion                    UINT16_C(5)
/** @} */

/** @name HFS mount version numbers (HFSPlusVolumeHeader::lastMountedVersion).
 * @{ */
#define kHFSPlusMountVersion            UINT32_C(0x31302e30)
#define kHFSJMountVersion               UINT32_C(0x4846534a)
#define kFSKMountVersion                UINT32_C(0x46534b21)
/** @}  */

/** @name Hard link file creators & types.
 * @{ */
#define kHardLinkFileType               UINT32_C(0x686c6e6b)
#define kHFSPlusCreator                 UINT32_C(0x6866732b)
/** @} */

/** @name Symlink file creators & types.
 * @{ */
#define kSymLinkFileType                UINT32_C(0x736c6e6b)
#define kSymLinkCreator                 UINT32_C(0x72686170)
/** @} */

/** @name Name limits.
 * @{ */
#define kHFSMaxVolumeNameChars          UINT8_C(0x1b)
#define kHFSMaxFileNameChars            UINT8_C(0x1f)
#define kHFSPlusMaxFileNameChars        UINT8_C(0xff)
#define kHFSMaxAttrNameLen              UINT8_C(0x7f)
/** @} */

/** @name Extent descriptor record densities
 * @{ */
#define kHFSExtentDensity               UINT8_C(3)
#define kHFSPlusExtentDensity           UINT8_C(8)
/** @} */


/** @name File IDs (various fileID members).
 * @{ */
#define kHFSRootParentID               UINT32_C(0x00000001)
#define kHFSRootFolderID               UINT32_C(0x00000002)
#define kHFSExtentsFileID              UINT32_C(0x00000003)
#define kHFSCatalogFileID              UINT32_C(0x00000004)
#define kHFSBadBlockFileID             UINT32_C(0x00000005)
#define kHFSAllocationFileID           UINT32_C(0x00000006)
#define kHFSStartupFileID              UINT32_C(0x00000007)
#define kHFSAttributesFileID           UINT32_C(0x00000008)
#define kHFSAttributeDataFileID        UINT32_C(0x0000000c)
#define kHFSRepairCatalogFileID        UINT32_C(0x0000000e)
#define kHFSBogusExtentFileID          UINT32_C(0x0000000f)
#define kHFSFirstUserCatalogNodeID     UINT32_C(0x00000010)
/** @} */

/** @name Catalog record types.
 * @{ */
#define kHFSFolderRecord                UINT16_C(0x0100)
#define kHFSFileRecord                  UINT16_C(0x0200)
#define kHFSFolderThreadRecord          UINT16_C(0x0300)
#define kHFSFileThreadRecord            UINT16_C(0x0400)
#define kHFSPlusFolderRecord            UINT16_C(0x0001)
#define kHFSPlusFileRecord              UINT16_C(0x0002)
#define kHFSPlusFolderThreadRecord      UINT16_C(0x0003)
#define kHFSPlusFileThreadRecord        UINT16_C(0x0004)
/** @} */

/** @name File record bits and masks.
 * @{ */
#define kHFSFileLockedBit               0
#define kHFSThreadExistsBit             1
#define kHFSHasAttributesBit            2
#define kHFSHasSecurityBit              3
#define kHFSHasFolderCountBit           4
#define kHFSHasLinkChainBit             5
#define kHFSHasChildLinkBit             6
#define kHFSHasDateAddedBit             7

#define kHFSFileLockedMask              RT_BIT(kHFSFileLockedBit)
#define kHFSThreadExistsMask            RT_BIT(kHFSThreadExistsBit)
#define kHFSHasAttributesMask           RT_BIT(kHFSHasAttributesBit)
#define kHFSHasSecurityMask             RT_BIT(kHFSHasSecurityBit)
#define kHFSHasFolderCountMask          RT_BIT(kHFSHasFolderCountBit)
#define kHFSHasLinkChainMask            RT_BIT(kHFSHasLinkChainBit)
#define kHFSHasChildLinkMask            RT_BIT(kHFSHasChildLinkBit)
#define kHFSHasDateAddedMask            RT_BIT(kHFSHasDateAddedBit)
/** @} */

/** @name Key and node lengths.
 * @{ */
#define kHFSPlusAttrKeyMaximumLength    ( sizeof(HFSPlusAttrKey) - sizeof(uint16_t) )
#define kHFSPlusAttrKeyMinimumLength    ( kHFSPlusAttrKeyMaximumLength - (kHFSMaxAttrNameLen * sizeof(uint16_t)) )
#define kHFSPlusExtentKeyMaximumLength  ( sizeof(HFSPlusExtentKey) - sizeof(uint16_t),
#define kHFSExtentKeyMaximumLength      ( sizeof(HFSExtentKey) - sizeof(uint8_t) )
#define kHFSPlusCatalogKeyMaximumLength ( sizeof(HFSPlusCatalogKey) - sizeof(uint16_t) )
#define kHFSPlusCatalogKeyMinimumLength ( kHFSPlusCatalogKeyMaximumLength - sizeof(HFSUniStr255) + sizeof(uint16_t) )
#define kHFSCatalogKeyMaximumLength     ( sizeof(HFSCatalogKey) - sizeof(uint8_t) )
#define kHFSCatalogKeyMinimumLength     ( kHFSCatalogKeyMaximumLength - kHFSMaxFileNameChars - 1 + sizeof(uint8_t) )
#define kHFSPlusCatalogMinNodeSize      UINT16_C(0x1000)
#define kHFSPlusExtentMinNodeSize       UINT16_C(0x0200)
#define kHFSPlusAttrMinNodeSize         UINT16_C(0x1000)
/** @} */

/** @name Volume Attribute bits and masks.
 * @remarks HFS has only 16-bit wide field, HFS+ has 32-bit.
 * @{ */
#define kHFSVolumeHardwareLockBit       7
#define kHFSVolumeUnmountedBit          8
#define kHFSVolumeSparedBlocksBit       9
#define kHFSVolumeNoCacheRequiredBit    10
#define kHFSBootVolumeInconsistentBit   11
#define kHFSCatalogNodeIDsReusedBit     12
#define kHFSVolumeJournaledBit          13
#define kHFSVolumeInconsistentBit       14
#define kHFSVolumeSoftwareLockBit       15
#define kHFSUnusedNodeFixBit            31
#define kHFSContentProtectionBit        30

#define kHFSVolumeHardwareLockMask      RT_BIT(kHFSVolumeHardwareLockBit)
#define kHFSVolumeUnmountedMask         RT_BIT(kHFSVolumeUnmountedBit)
#define kHFSVolumeSparedBlocksMask      RT_BIT(kHFSVolumeSparedBlocksBit)
#define kHFSVolumeNoCacheRequiredMask   RT_BIT(kHFSVolumeNoCacheRequiredBit)
#define kHFSBootVolumeInconsistentMask  RT_BIT(kHFSBootVolumeInconsistentBit)
#define kHFSCatalogNodeIDsReusedMask    RT_BIT(kHFSCatalogNodeIDsReusedBit)
#define kHFSVolumeJournaledMask         RT_BIT(kHFSVolumeJournaledBit)
#define kHFSVolumeInconsistentMask      RT_BIT(kHFSVolumeInconsistentBit)
#define kHFSVolumeSoftwareLockMask      RT_BIT(kHFSVolumeSoftwareLockBit)
#define kHFSUnusedNodeFixMask           RT_BIT(kHFSUnusedNodeFixBit)
#define kHFSContentProtectionMask       RT_BIT(kHFSContentProtectionBit)

#define kHFSMDBAttributesMask           UINT16_C(0x8380)
/** @} */

/** @name Misc
 * @{ */
#define kHFSUnusedNodesFixDate          UINT32_C(0xc5ef2480)

#define HFSPLUSMETADATAFOLDER           "\xE2\x90\x80\xE2\x90\x80\xE2\x90\x80\xE2\x90\x80HFS+ Private Data"
#define HFSPLUS_DIR_METADATA_FOLDER     ".HFS+ Private Directory Data\xd"
#define HFS_INODE_PREFIX                "iNode"
#define HFS_DELETE_PREFIX               "temp"
#define HFS_DIRINODE_PREFIX             "dir_"
#define FIRST_LINK_XATTR_NAME           "com.apple.system.hfs.firstlink"
#define FIRST_LINK_XATTR_REC_SIZE       ( sizeof(HFSPlusAttrData) + 10 )

/* {b3e20f39-f292-11d6-97a4-00306543ecac} */
#define HFS_UUID_NAMESPACE_ID           "\xB3\xE2\x0F\x39\xF2\x92\x11\xD6\x97\xA4\x00\x30\x65\x43\xEC\xAC"

#define SET_HFS_TEXT_ENCODING(a_uHint)  (UINT32_C(0x656e6300) | (uint8_t)(a_uHint))
#define GET_HFS_TEXT_ENCODING(a_uHint)  ( ((a_uHint) & UINT32_C(0xffffff00)) == UINT32_C(0x656e6300) \
                                         ? UINT32_C(0x000000ff)(a_uHint) : UINT32_MAX)
/** @} */

/** @name B-tree stuff.
 * @{ */
#define kMaxKeyLength                   520

#define kBTLeafNode                     (-1)
#define kBTIndexNode                    0
#define kBTHeaderNode                   1
#define kBTMapNode                      2

#define kBTBadCloseMask                 RT_BIT_32(0)
#define kBTBigKeysMask                  RT_BIT_32(1)
#define kBTVariableIndexKeysMask        RT_BIT_32(2)

/** @} */

/** @name B-tree compare types (BTHeaderRec::keyCompareType)
 * @{ */
#define kHFSCaseFolding                 UINT8_C(0xcf)
#define kHFSBinaryCompare               UINT8_C(0xbc)
/** @} */

/** @name Journal stuff.
 * @{ */
#define JIB_RESERVED_SIZE               ( sizeof(uint32_t) * 32 - 85 )

#define kJIJournalInFSMask              RT_BIT_32(0)
#define kJIJournalOnOtherDeviceMask     RT_BIT_32(1)
#define kJIJournalNeedInitMask          RT_BIT_32(2)

#define EXTJNL_CONTENT_TYPE_UUID        "4a6f7572-6e61-11aa-aa11-00306543ecac"
/** @} */



typedef struct HFSUniStr255
{
    uint16_t                length;
    RTUTF16                 unicode[255];
} HFSUniStr255;
AssertCompileSize(HFSUniStr255, 0x200);
typedef const HFSUniStr255 * ConstHFSUniStr255Param;

#pragma pack(1)
typedef struct HFSExtentKey
{
    uint8_t                 keyLength;
    uint8_t                 forkType;
    uint32_t                fileID;         /**< Misaligned. */
    uint16_t                startBLock;
} HFSExtentKey;
#pragma pack()
AssertCompileSize(HFSExtentKey, 8);

typedef struct HFSPlusExtentKey
{
    uint16_t                keyLength;
    uint8_t                 forkType;
    uint8_t                 pad;
    uint32_t                fileID;
    uint32_t                startBlock;
} HFSPlusExtentKey;
AssertCompileSize(HFSPlusExtentKey, 12);

typedef struct HFSExtentDescriptor
{
    uint16_t                startBlock;
    uint16_t                blockCount;
} HFSExtentDescriptor;
AssertCompileSize(HFSExtentDescriptor, 4);

typedef struct HFSPlusExtentDescriptor
{
    uint32_t                startBlock;
    uint32_t                blockCount;
} HFSPlusExtentDescriptor;
AssertCompileSize(HFSPlusExtentDescriptor, 8);

typedef HFSExtentDescriptor     HFSExtentRecord[3];
typedef HFSPlusExtentDescriptor HFSPlusExtentRecord[8];

typedef struct FndrFileInfo
{
    uint32_t                fdType;
    uint32_t                fdCreator;
    uint16_t                fdFlags;
    struct
    {
        int16_t             v;
        int16_t             h;
    }                       fdLocation;
    uint16_t                opaque;
} FndrFileInfo;
AssertCompileSize(FndrFileInfo, 16);

typedef struct FndrDirInfo
{
    struct
    {
        int16_t             top;
        int16_t             left;
        int16_t             bottom;
        int16_t             right;
    }                       frRect;
    uint16_t                frFlags;
    struct
    {
        int16_t             v;
        int16_t             h;
    }                       fdLocation;
    uint16_t                opaque;
} FndrDirInfo;
AssertCompileSize(FndrDirInfo, 16);

typedef struct FndrOpaqueInfo
{
    int8_t                  opaque[16];
} FndrOpaqueInfo;
AssertCompileSize(FndrOpaqueInfo, 16);

typedef struct FndrExtendedFileInfo
{
    uint32_t                reserved1;
    uint32_t                date_added;
    uint16_t                extended_flags;
    uint16_t                reserved2;
    uint32_t                reserved3;
} FndrExtendedFileInfo;
AssertCompileSize(FndrExtendedFileInfo, 16);

typedef struct FndrExtendedDirInfo
{
    uint32_t                point;
    uint32_t                date_added;
    uint16_t                extended_flags;
    uint16_t                reserved3;
    uint32_t                reserved4;
} FndrExtendedDirInfo;
AssertCompileSize(FndrExtendedDirInfo, 16);

typedef struct HFSPlusForkData
{
    uint64_t                logicalSize;
    uint32_t                clumpSize;
    uint32_t                totalBlocks;
    HFSPlusExtentRecord     extents;
} HFSPlusForkData;
AssertCompileSize(HFSPlusForkData, 80);

typedef struct HFSPlusBSDInfo
{
    uint32_t                ownerID;
    uint32_t                groupID;
    uint8_t                 adminFlags;
    uint8_t                 ownerFlags;
    uint16_t                fileMode;
    union
    {
        uint32_t            iNodeNum;
        uint32_t            linkCount;
        uint32_t            rawDevice;
    }                       special;
} HFSPlusBSDInfo;
AssertCompileSize(HFSPlusBSDInfo, 16);

#pragma pack(1)
typedef struct HFSCatalogKey
{
    uint8_t                 keyLength;
    uint8_t                 reserved;
    uint32_t                parentID;       /**< Misaligned. */
    uint8_t                 nodeName[kHFSMaxFileNameChars + 1];
} HFSCatalogKey;
#pragma pack()
AssertCompileSize(HFSCatalogKey, 0x26);

#pragma pack(1)
typedef struct HFSPlusCatalogKey
{
    uint16_t                keyLength;
    uint32_t                parentID;       /**< Misaligned. */
    HFSUniStr255            nodeName;
} HFSPlusCatalogKey;
#pragma pack()
AssertCompileSize(HFSPlusCatalogKey, 0x206);

#pragma pack(1)
typedef struct HFSCatalogFolder
{
    int16_t                 recordType;
    uint16_t                flags;
    uint16_t                valence;
    uint32_t                folderID;       /**< Misaligned. */
    uint32_t                createDate;     /**< Misaligned. */
    uint32_t                modifyDate;     /**< Misaligned. */
    uint32_t                backupDate;     /**< Misaligned. */
    FndrDirInfo             userInfo;
    FndrOpaqueInfo          finderInfo;
    uint32_t                reserved[4];    /**< Misaligned. */
} HFSCatalogFolder;
#pragma pack()
AssertCompileSize(HFSCatalogFolder, 70);

typedef struct HFSPlusCatalogFolder
{
    int16_t                 recordType;
    uint16_t                flags;
    uint32_t                valence;
    uint32_t                folderID;
    uint32_t                createDate;
    uint32_t                contentModDate;
    uint32_t                attributeModDate;
    uint32_t                accessDate;
    uint32_t                backupDate;
    HFSPlusBSDInfo          bsdInfo;
    FndrDirInfo             userInfo;
    FndrOpaqueInfo          finderInfo;
    uint32_t                textEncoding;
    uint32_t                folderCount;
} HFSPlusCatalogFolder;
AssertCompileSize(HFSPlusCatalogFolder, 88);

#pragma pack(1)
typedef struct HFSCatalogFile
{
    int16_t                 recordType;
    uint8_t                 flags;
    uint8_t                 fileType;
    FndrFileInfo            userInfo;
    uint32_t                fileID;
    uint16_t                dataStartBlock;
    int32_t                 dataLogicalSize;  /**< Misaligned. */
    int32_t                 dataPhysicalSize; /**< Misaligned. */
    uint16_t                rsrcStartBlock;
    int32_t                 rsrcLogicalSize;
    int32_t                 rsrcPhysicalSize;
    uint32_t                createDate;
    uint32_t                modifyDate;
    uint32_t                backupDate;
    FndrOpaqueInfo          finderInfo;
    uint16_t                clumpSize;
    HFSExtentRecord         dataExtents;    /**< Misaligned. */
    HFSExtentRecord         rsrcExtents;    /**< Misaligned. */
    uint32_t                reserved;       /**< Misaligned. */
} HFSCatalogFile;
#pragma pack()
AssertCompileSize(HFSCatalogFile, 102);

#pragma pack(1)
typedef struct HFSPlusCatalogFile
{
    int16_t                 recordType;
    uint16_t                flags;
    uint32_t                reserved1;
    uint32_t                fileID;
    uint32_t                createDate;
    uint32_t                contentModDate;
    uint32_t                attributeModDate;
    uint32_t                accessDate;
    uint32_t                backupDate;
    HFSPlusBSDInfo          bsdInfo;
    FndrFileInfo            userInfo;
    FndrOpaqueInfo          finderInfo;
    uint32_t                textEncoding;
    uint32_t                reserved2;
    HFSPlusForkData         dataFork;
    HFSPlusForkData         resourceFork;
} HFSPlusCatalogFile;
#pragma pack()
AssertCompileMemberAlignment(HFSPlusCatalogFile, dataFork, 8);
AssertCompileSize(HFSPlusCatalogFile, 248);

#pragma pack(1)
typedef struct HFSCatalogThread
{
    int16_t                 recordType;
    int32_t                 reserved[2];
    uint32_t                parentID;
    uint8_t                 nodeName[kHFSMaxFileNameChars + 1];
} HFSCatalogThread;
#pragma pack()
AssertCompileSize(HFSCatalogThread, 46);

typedef struct HFSPlusCatalogThread
{
    int16_t                 recordType;
    int16_t                 reserved;
    uint32_t                parentID;
    HFSUniStr255            nodeName;
} HFSPlusCatalogThread;
AssertCompileSize(HFSPlusCatalogThread, 0x208);

typedef struct HFSPlusAttrForkData
{
    uint32_t                recordType;
    uint32_t                reserved;
    HFSPlusForkData         theFork;
} HFSPlusAttrForkData;
AssertCompileSize(HFSPlusAttrForkData, 88);

typedef struct HFSPlusAttrExtents
{
    uint32_t                recordType;
    uint32_t                reserved;
    HFSPlusExtentRecord     extents;
} HFSPlusAttrExtents;
AssertCompileSize(HFSPlusAttrExtents, 72);

#pragma pack(1)
typedef struct HFSPlusAttrData
{
    uint32_t                recordType;
    uint32_t                reserved[2];
    uint32_t                attrSize;
    uint8_t                 attrData[2];    /**< Causes misaligned struct size. */
} HFSPlusAttrData;
#pragma pack()
AssertCompileSize(HFSPlusAttrData, 18);

#pragma pack(1)
typedef struct HFSPlusAttrInlineData
{
    uint32_t                recordType;
    uint32_t                reserved;
    uint32_t                logicalSize;
    uint8_t                 userData[2];    /**< Causes misaligned struct size. */
} HFSPlusAttrInlineData;
#pragma pack()
AssertCompileSize(HFSPlusAttrInlineData, 14);

typedef union HFSPlusAttrRecord
{
    uint32_t                recordType;
    HFSPlusAttrInlineData   inlineData;
    HFSPlusAttrData         attrData;
    HFSPlusAttrForkData     forkData;
    HFSPlusAttrExtents      overflowExtents;
} HFSPlusAttrRecord;
AssertCompileSize(HFSPlusAttrRecord, 88);

typedef struct HFSPlusAttrKey
{
    uint16_t                keyLength;
    uint16_t                pad;
    uint32_t                fileID;
    uint32_t                startBlock;
    uint16_t                attrNameLen;
    RTUTF16                 attrName[kHFSMaxAttrNameLen];
} HFSPlusAttrKey;
AssertCompileSize(HFSPlusAttrKey, 268);

#pragma pack(1)
typedef struct HFSMasterDirectoryBlock
{
    uint16_t                drSigWord;
    uint32_t                drCrDate;       /**< Misaligned. */
    uint32_t                drLsMod;        /**< Misaligned. */
    uint16_t                drAtrb;
    uint16_t                drNmFls;
    uint16_t                drVBMSt;
    uint16_t                drAllocPtr;
    uint16_t                drNmAlBlks;
    uint32_t                drAlBlkSiz;
    uint32_t                drClpSiz;
    uint16_t                drAlBlSt;
    uint32_t                drNxCNID;       /**< Misaligned. */
    uint16_t                drFreeBks;
    uint8_t                 drVN[kHFSMaxVolumeNameChars + 1];
    uint32_t                drVolBkUp;
    uint16_t                drVSeqNum;
    uint32_t                drWrCnt;        /**< Misaligned. */
    uint32_t                drXTClpSiz;     /**< Misaligned. */
    uint32_t                drCTClpSiz;     /**< Misaligned. */
    uint16_t                drNmRtDirs;
    uint32_t                drFilCnt;
    uint32_t                drDirCnt;
    uint32_t                drFndrInfo[8];
    uint16_t                drEmbedSigWord;
    HFSExtentDescriptor     drEmbedExtent;
    uint32_t                drXTFlSize;     /**< Misaligned. */
    HFSExtentRecord         drXTExtRec;
    uint32_t                drCTFlSize;     /**< Misaligned. */
    HFSExtentRecord         drCTExtRec;
} HFSMasterDirectoryBlock;
#pragma pack()
AssertCompileSize(HFSMasterDirectoryBlock, 162);

typedef struct HFSPlusVolumeHeader
{
    uint16_t                signature;
    uint16_t                version;
    uint32_t                attributes;
    uint32_t                lastMountedVersion;
    uint32_t                journalInfoBlock;
    uint32_t                createDate;
    uint32_t                modifyDate;
    uint32_t                backupDate;
    uint32_t                checkedDate;
    uint32_t                fileCount;
    uint32_t                folderCount;
    uint32_t                blockSize;
    uint32_t                totalBlocks;
    uint32_t                freeBlocks;
    uint32_t                nextAllocation;
    uint32_t                rsrcClumpSize;
    uint32_t                dataClumpSize;
    uint32_t                nextCatalogID;
    uint32_t                writeCount;
    uint64_t                encodingsBitmap;
    uint8_t                 finderInfo[32];
    HFSPlusForkData         allocationFile;
    HFSPlusForkData         extentsFile;
    HFSPlusForkData         catalogFile;
    HFSPlusForkData         attributesFile;
    HFSPlusForkData         startupFile;
} HFSPlusVolumeHeader;
AssertCompileMemberAlignment(HFSPlusVolumeHeader, nextCatalogID, 8);
AssertCompileSize(HFSPlusVolumeHeader, 512);

typedef union BTreeKey
{
    uint8_t                 length8;
    uint16_t                length16;
    uint8_t                 rawData[kMaxKeyLength + 2];
} BTreeKey;
AssertCompileSize(BTreeKey, 522);

#pragma pack(1)
typedef struct BTNodeDescriptor
{
    uint32_t                fLink;
    uint32_t                bLink;
    int8_t                  kind;
    uint8_t                 height;
    uint16_t                numRecords;
    uint16_t                reserved;       /**< Causes struct size misalignment. */
} BTNodeDescriptor;
#pragma pack()
AssertCompileSize(BTNodeDescriptor, 14);

#pragma pack(1)
typedef struct BTHeaderRec
{
    uint16_t                treeDepth;
    uint32_t                rootNode;       /**< Misaligned. */
    uint32_t                leafRecords;    /**< Misaligned. */
    uint32_t                firstLeafNode;  /**< Misaligned. */
    uint32_t                lastLeafNode;   /**< Misaligned. */
    uint16_t                nodeSize;
    uint16_t                maxKeyLength;
    uint32_t                totalNodes;     /**< Misaligned. */
    uint32_t                freeNodes;      /**< Misaligned. */
    uint16_t                reserved1;
    uint32_t                clumpSize;
    uint8_t                 btreeType;
    uint8_t                 keyCompareType;
    uint32_t                attributes;     /**< Misaligned. */
    uint32_t                reserved3[16];  /**< Misaligned. */
} BTHeaderRec;
#pragma pack()
AssertCompileSize(BTHeaderRec, 106);

#pragma pack(1)
typedef struct JournalInfoBlock
{
    uint32_t                flags;
    uint32_t                devices_signature[8];
    uint64_t                offset;         /**< Misaligned (morons). */
    uint64_t                size;           /**< Misaligned. */
    char                    ext_jnl_uuid[37];
    char                    machine_serial_num[48];
    char                    reserved[JIB_RESERVED_SIZE];
} JournalInfoBlock;
#pragma pack()
AssertCompileSize(JournalInfoBlock, 180);

/** @}  */

#endif /* !IPRT_INCLUDED_formats_hfs_h */

