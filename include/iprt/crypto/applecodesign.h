/** @file
 * IPRT - Apple Code Signing Structures and APIs.
 */

/*
 * Copyright (C) 2018-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_crypto_applecodesign_h
#define IPRT_INCLUDED_crypto_applecodesign_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#include <iprt/assertcompile.h>
#include <iprt/crypto/pkcs7.h>

/** @defgroup grp_rt_craplcs RTCrAppleCs - Apple Code Signing
 * @ingroup grp_rt_crypto
 * @{
 */

/** Apple developer ID for iPhone application software development signing. */
#define RTCR_APPLE_CS_DEVID_IPHONE_SW_DEV_OID           "1.2.840.113635.100.6.1.2"
/** Apple developer ID for Mac application software development signing. */
#define RTCR_APPLE_CS_DEVID_MAC_SW_DEV_OID              "1.2.840.113635.100.6.1.12"
/** Apple developer ID for application signing. */
#define RTCR_APPLE_CS_DEVID_APPLICATION_OID             "1.2.840.113635.100.6.1.13"
/** Apple developer ID for installer signing. */
#define RTCR_APPLE_CS_DEVID_INSTALLER_OID               "1.2.840.113635.100.6.1.14"
/** Apple developer ID for kernel extension signing. */
#define RTCR_APPLE_CS_DEVID_KEXT_OID                    "1.2.840.113635.100.6.1.18"
/** Apple certificate policy OID.   */
#define RTCR_APPLE_CS_CERTIFICATE_POLICY_OID            "1.2.840.113635.100.5.1"


/** @name RTCRAPLCS_MAGIC_XXX - Apple code signing magic values for identifying blobs
 * @note No byte order conversion required.
 * @{ */
#define RTCRAPLCS_MAGIC_BLOBWRAPPER                     RT_N2H_U32_C(UINT32_C(0xfade0b01))
#define RTCRAPLCS_MAGIC_EMBEDDED_SIGNATURE_OLD          RT_N2H_U32_C(UINT32_C(0xfade0b02))
#define RTCRAPLCS_MAGIC_REQUIREMENT                     RT_N2H_U32_C(UINT32_C(0xfade0c00))
#define RTCRAPLCS_MAGIC_REQUIREMENTS                    RT_N2H_U32_C(UINT32_C(0xfade0c01))
#define RTCRAPLCS_MAGIC_CODEDIRECTORY                   RT_N2H_U32_C(UINT32_C(0xfade0c02))
#define RTCRAPLCS_MAGIC_EMBEDDED_SIGNATURE              RT_N2H_U32_C(UINT32_C(0xfade0cc0))
#define RTCRAPLCS_MAGIC_DETACHED_SIGNATURE              RT_N2H_U32_C(UINT32_C(0xfade0cc1))
/** @} */

/** @name Apple code signing versions.
 * @note Requires byte order conversion of the field value.  That way
 *       greater-than and less-than comparisons works correctly.
 * @{  */
#define RTCRAPLCS_VER_2_0                               UINT32_C(0x00020000)
#define RTCRAPLCS_VER_SUPPORTS_SCATTER                  UINT32_C(0x00020100)
#define RTCRAPLCS_VER_SUPPORTS_TEAMID                   UINT32_C(0x00020200)
#define RTCRAPLCS_VER_SUPPORTS_CODE_LIMIT_64            UINT32_C(0x00020300)
#define RTCRAPLCS_VER_SUPPORTS_EXEC_SEG                 UINT32_C(0x00020400)
/** @} */

/** @name RTCRAPLCS_SLOT_XXX - Apple code signing slots.
 * @note No byte order conversion required.
 * @{ */
#define RTCRAPLCS_SLOT_CODEDIRECTORY                    RT_N2H_U32_C(UINT32_C(0x00000000))
#define RTCRAPLCS_SLOT_INFO                             RT_N2H_U32_C(UINT32_C(0x00000001))
#define RTCRAPLCS_SLOT_REQUIREMENTS                     RT_N2H_U32_C(UINT32_C(0x00000002))
#define RTCRAPLCS_SLOT_RESOURCEDIR                      RT_N2H_U32_C(UINT32_C(0x00000003))
#define RTCRAPLCS_SLOT_APPLICATION                      RT_N2H_U32_C(UINT32_C(0x00000004))
#define RTCRAPLCS_SLOT_ENTITLEMENTS                     RT_N2H_U32_C(UINT32_C(0x00000005))
#define RTCRAPLCS_SLOT_ALTERNATE_CODEDIRECTORIES        RT_N2H_U32_C(UINT32_C(0x00001000))
#define RTCRAPLCS_SLOT_ALTERNATE_CODEDIRECTORIES_END    RT_N2H_U32_C(UINT32_C(0x00001005))
#define RTCRAPLCS_SLOT_ALTERNATE_CODEDIRECTORIES_COUNT  UINT32_C(0x00000005)
#define RTCRAPLCS_SLOT_ALTERNATE_CODEDIRECTORY_INC      RT_N2H_U32_C(UINT32_C(0x00000001))
/** The signature.
 * This is simply a RTCRAPLCSHDR/RTCRAPLCS_MAGIC_BLOBWRAPPER followed by a DER
 * encoded \#PKCS7 ContentInfo structure containing signedData.  The inner
 * signedData structure signs external data, so its ContentInfo member is set
 * to 1.2.840.113549.1.7.1 and has no data. */
#define RTCRAPLCS_SLOT_SIGNATURE                        RT_N2H_U32_C(UINT32_C(0x00010000))
/** @} */

/** @name RTCRAPLCS_HASHTYPE_XXX - Apple code signing hash types
 * @note Byte sized field, so no byte order concerns.
 * @{ */
#define RTCRAPLCS_HASHTYPE_SHA1                         UINT8_C(1)
#define RTCRAPLCS_HASHTYPE_SHA256                       UINT8_C(2)
#define RTCRAPLCS_HASHTYPE_SHA256_TRUNCATED             UINT8_C(3) /**< Truncated to 20 bytes (SHA1 size). */
#define RTCRAPLCS_HASHTYPE_SHA384                       UINT8_C(4)
/** @} */


/**
 * Apple code signing blob header.
 */
typedef struct RTCRAPLCSHDR
{
    /** The magic value (RTCRAPLCS_MAGIC_XXX).
     * (Big endian, but constant are big endian already.) */
    uint32_t            uMagic;
    /** The total length of the blob.  Big endian. */
    uint32_t            cb;
} RTCRAPLCSHDR;
AssertCompileSize(RTCRAPLCSHDR, 8);
/** Pointer to a CS blob header. */
typedef RTCRAPLCSHDR *PRTCRAPLCSHDR;
/** Pointer to a const CS blob header. */
typedef RTCRAPLCSHDR const *PCRTCRAPLCSHDR;

/**
 * Apple code signing super blob slot.
 */
typedef struct RTCRAPLCSBLOBSLOT
{
    /** Slot type, RTCRAPLCS_SLOT_XXX.
     * (Big endian, but so are the constants too). */
    uint32_t            uType;
    /** Data offset.  Big endian. */
    uint32_t            offData;
} RTCRAPLCSBLOBSLOT;
AssertCompileSize(RTCRAPLCSBLOBSLOT, 8);
/** Pointer to a super blob slot. */
typedef RTCRAPLCSBLOBSLOT *PRTCRAPLCSBLOBSLOT;
/** Pointer to a const super blob slot. */
typedef RTCRAPLCSBLOBSLOT const *PCRTCRAPLCSBLOBSLOT;

/**
 * Apple code signing super blob.
 */
typedef struct RTCRAPLCSSUPERBLOB
{
    /** Header (uMagic = RTCRAPLCS_MAGIC_EMBEDDED_SIGNATURE?
     *  or RTCRAPLCS_MAGIC_EMBEDDED_SIGNATURE_OLD? ). */
    RTCRAPLCSHDR        Hdr;
    /** Number of slots.  Big endian. */
    uint32_t            cSlots;
    /** Slots. */
    RT_FLEXIBLE_ARRAY_EXTENSION
    RTCRAPLCSBLOBSLOT   aSlots[RT_FLEXIBLE_ARRAY];
} RTCRAPLCSSUPERBLOB;
AssertCompileMemberOffset(RTCRAPLCSSUPERBLOB, aSlots, 12);
/** Pointer to a CS super blob.   */
typedef RTCRAPLCSSUPERBLOB *PRTCRAPLCSSUPERBLOB;
/** Pointer to a const CS super blob.   */
typedef RTCRAPLCSSUPERBLOB const *PCRTCRAPLCSSUPERBLOB;

/**
 * Code directory (RTCRAPLCS_MAGIC_CODEDIRECTORY).
 */
typedef struct RTCRAPLCSCODEDIRECTORY
{
    /** 0x00: Header (uMagic = RTCRAPLCS_MAGIC_CODEDIRECTORY). */
    RTCRAPLCSHDR    Hdr;
    /** 0x08: The version number (RTCRAPLCS_VER_XXX).
     * @note Big endian, host order constants. */
    uint32_t        uVersion;
    /** 0x0c: Flags & mode, RTCRAPLCS_???.  (Big endian. ) */
    uint32_t        fFlags;
    /** 0x10: Offset of the hash slots.  Big endian.
     * Special slots found below this offset, code slots at and after.  */
    uint32_t        offHashSlots;
    /** 0x14: Offset of the identifier string.  Big endian. */
    uint32_t        offIdentifier;
    /** 0x18: Number of special hash slots.  Hubertus Bigend style. */
    uint32_t        cSpecialSlots;
    /** 0x1c: Number of code hash slots.  Big endian. */
    uint32_t        cCodeSlots;
    /** 0x20: Number of bytes of code that's covered, 32-bit wide.  Big endian. */
    uint32_t        cbCodeLimit32;
    /** 0x24: The hash size. */
    uint8_t         cbHash;
    /** 0x25: The hash type (RTCRAPLCS_HASHTYPE_XXX). */
    uint8_t         bHashType;
    /** 0x26: Platform identifier or zero. */
    uint8_t         idPlatform;
    /** 0x27: The page shift value.  zero if infinite page size. */
    uint8_t         cPageShift;
    /** 0x28: Spare field, MBZ. */
    uint32_t        uUnused1;
    /** 0x2c: Offset of scatter vector (optional).  Big endian.
     * @since RTCRAPLCS_VER_SUPPORTS_SCATTER */
    uint32_t        offScatter;
    /** 0x30: Offset of team identifier (optional).  Big endian.
     * @since RTCRAPLCS_VER_SUPPORTS_TEAMID */
    uint32_t        offTeamId;
    /** 0x34: Unused field, MBZ.
     * @since RTCRAPLCS_VER_SUPPORTS_CODE_LIMIT_64 */
    uint32_t        uUnused2;
    /** 0x38: Number of bytes of code that's covered, 64-bit wide.  Big endian.
     * @since RTCRAPLCS_VER_SUPPORTS_CODE_LIMIT_64 */
    uint64_t        cbCodeLimit64;
    /** 0x40: File offset of the first segment.  Big endian.
     * @since RTCRAPLCS_VER_SUPPORTS_EXEC_SEG */
    uint64_t        offExecSeg;
    /** 0x48: The size of the first segment.  Big endian.
     * @since RTCRAPLCS_VER_SUPPORTS_EXEC_SEG */
    uint64_t        cbExecSeg;
    /** 0x50: Flags for the first segment.  Big endian.
     * @since RTCRAPLCS_VER_SUPPORTS_EXEC_SEG */
    uint64_t        fExecSeg;
} RTCRAPLCSCODEDIRECTORY;
AssertCompileSize(RTCRAPLCSCODEDIRECTORY, 0x58);
/** Pointer to a CS code directory. */
typedef RTCRAPLCSCODEDIRECTORY *PRTCRAPLCSCODEDIRECTORY;
/** Pointer to a const CS code directory. */
typedef RTCRAPLCSCODEDIRECTORY const *PCRTCRAPLCSCODEDIRECTORY;


/**
 * IPRT structure for working with an Apple code signing blob.
 */
typedef struct RTCRAPLCS
{
    uint8_t const  *pbBlob;
    size_t          cbBlob;
    size_t          auReserved[4];
} RTCRAPLCS;
/** Pointer to an IPRT CS blob descriptor. */
typedef RTCRAPLCS *PRTCRAPLCS;

/**
 * Initialize a RTCRAPLCS descriptor and validate the blob data.
 *
 * @returns IPRT status code.
 * @param   pDesc       The descirptor to initialize.
 * @param   pvBlob      The blob bytes.
 * @param   cbBlob      The number of bytes in the blob.
 * @param   fFlags      Future validation flags, MBZ.
 * @param   pErrInfo    Where to return additional error details.  Optional.
 */
RTDECL(int) RTCrAppleCsInit(PRTCRAPLCS pDesc, void const *pvBlob, size_t cbBlob, uint32_t fFlags, PRTERRINFO pErrInfo);

/**
 * Callback used by RTCrAppleCsVerifyImage to digest a section of the image.
 *
 * @return IPRT status code.
 * @param   hDigest     The digest to feed the bytes to.
 * @param   off         The RVA of the bytes to digest.
 * @param   cb          Number of bytes to digest.
 * @param   pvUser      User argument.
 */
typedef DECLCALLBACKTYPE(int, FNRTCRAPPLECSDIGESTAREA,(RTCRDIGEST hDigest, size_t off, size_t cb, void *pvUser));
/** Pointer to a image digest callback. */
typedef FNRTCRAPPLECSDIGESTAREA *PFNRTCRAPPLECSDIGESTAREA;

/**
 * Verifies an image against the given signature blob.
 *
 * @return IPRT status code.
 * @param   pDesc       The apple code signing blob to verify against.
 * @param   fFlags      Future verification flags, MBZ.
 * @param   pfnCallback Image digest callback.
 * @param   pvUser      User argument for the callback.
 * @param   pErrInfo    Where to return additional error details.  Optional.
 */
RTDECL(int) RTCrAppleCsVerifyImage(PRTCRAPLCS pDesc, uint32_t fFlags, PFNRTCRAPPLECSDIGESTAREA pfnCallback,
                                   void *pvUser, PRTERRINFO pErrInfo);

RTDECL(int) RTCrAppleCsQuerySigneddData(PRTCRAPLCS pDesc, PRTCRPKCS7SIGNEDDATA pSignedData, PRTERRINFO pErrInfo);

/** @} */

#endif /* !IPRT_INCLUDED_crypto_applecodesign_h */

