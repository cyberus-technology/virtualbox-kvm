/** @file
 * IPRT - Crypto - Microsoft SPC / Authenticode.
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

#ifndef IPRT_INCLUDED_crypto_spc_h
#define IPRT_INCLUDED_crypto_spc_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/asn1.h>
#include <iprt/crypto/x509.h>
#include <iprt/crypto/pkcs7.h>
#include <iprt/md5.h>
#include <iprt/sha.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_cr_spc  RTCrSpc - Microsoft Authenticode
 * @ingroup grp_rt_crypto
 * @{
 */

/** Value for RTCR_PKCS9_ID_MS_STATEMENT_TYPE. */
#define RTCRSPC_STMT_TYPE_INDIVIDUAL_CODE_SIGNING   "1.3.6.1.4.1.311.2.1.21"

/**
 * PE Image page hash table, generic union.
 *
 * @remarks This table isn't used by ldrPE.cpp, it walks the table in a generic
 *          fashion using the hash size. So, we can ditch it if we feel like it.
 */
typedef union RTCRSPCPEIMAGEPAGEHASHES
{
    /** MD5 page hashes. */
    struct
    {
        /** The file offset.  */
        uint32_t        offFile;
        /** The hash. */
        uint8_t         abHash[RTSHA1_HASH_SIZE];
    } aMd5[1];

    /** SHA-1 page hashes. */
    struct
    {
        /** The file offset.  */
        uint32_t        offFile;
        /** The hash. */
        uint8_t         abHash[RTSHA1_HASH_SIZE];
    } aSha1[1];

    /** SHA-256 page hashes. */
    struct
    {
        /** The file offset.  */
        uint32_t        offFile;
        /** The hash. */
        uint8_t         abHash[RTSHA256_HASH_SIZE];
    } aSha256[1];

    /** SHA-512 page hashes. */
    struct
    {
        /** The file offset.  */
        uint32_t        offFile;
        /** The hash. */
        uint8_t         abHash[RTSHA512_HASH_SIZE];
    } aSha512[1];

    /** Generic view of ONE hash. */
    struct
    {
        /** The file offset. */
        uint32_t        offFile;
        /** Variable length hash field. */
        uint8_t         abHash[1];
    } Generic;
} RTCRSPCPEIMAGEPAGEHASHES;
/** Pointer to a PE image page hash table union. */
typedef RTCRSPCPEIMAGEPAGEHASHES *PRTCRSPCPEIMAGEPAGEHASHES;
/** Pointer to a const PE image page hash table union. */
typedef RTCRSPCPEIMAGEPAGEHASHES const *PCRTCRSPCPEIMAGEPAGEHASHES;


/**
 * Serialization wrapper for raw RTCRSPCPEIMAGEPAGEHASHES data.
 */
typedef struct RTCRSPCSERIALIZEDPAGEHASHES
{
    /** The page hashes are within a set. Dunno if there could be multiple
     * entries in this set, never seen it yet, so I doubt it. */
    RTASN1SETCORE                   SetCore;
    /** Octet string containing the raw data. */
    RTASN1OCTETSTRING               RawData;

    /** Pointer to the hash data within that string.
     * The hash algorithm is given by the object attribute type in
     * RTCRSPCSERIALIZEDOBJECTATTRIBUTE.  It is generally the same as for the
     * whole image hash. */
    PCRTCRSPCPEIMAGEPAGEHASHES      pData;
    /** Field the user can use to store the number of pages in pData. */
    uint32_t                        cPages;
} RTCRSPCSERIALIZEDPAGEHASHES;
/** Pointer to a serialized wrapper for page hashes.  */
typedef RTCRSPCSERIALIZEDPAGEHASHES *PRTCRSPCSERIALIZEDPAGEHASHES;
/** Pointer to a const serialized wrapper for page hashes.  */
typedef RTCRSPCSERIALIZEDPAGEHASHES const *PCRTCRSPCSERIALIZEDPAGEHASHES;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRSPCSERIALIZEDPAGEHASHES, RTDECL, RTCrSpcSerializedPageHashes, SetCore.Asn1Core);

RTDECL(int) RTCrSpcSerializedPageHashes_UpdateDerivedData(PRTCRSPCSERIALIZEDPAGEHASHES pThis);


/**
 * Data type selection for RTCRSPCSERIALIZEDOBJECTATTRIBUTE.
 */
typedef enum RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE
{
    /** Invalid zero entry. */
    RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE_INVALID = 0,
    /** Not present pro forma. */
    RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE_NOT_PRESENT,
    /** Unknown object. */
    RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE_UNKNOWN,
    /** SHA-1 page hashes (pPageHashes). */
    RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE_PAGE_HASHES_V1,
    /** SHA-256 page hashes (pPageHashes). */
    RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE_PAGE_HASHES_V2,
    /** End of valid values. */
    RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE_END,
    /** Blow up the type to at least 32-bits. */
    RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE_32BIT_HACK
} RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE;

/**
 * One serialized object attribute (PE image data).
 */
typedef struct RTCRSPCSERIALIZEDOBJECTATTRIBUTE
{
    /** Sequence core. */
    RTASN1SEQUENCECORE                  SeqCore;
    /** The attribute type. */
    RTASN1OBJID                         Type;
    /** The allocation of the data type. */
    RTASN1ALLOCATION                    Allocation;
    /** Indicates the valid value in the union. */
    RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE enmType;
    /** Union with data format depending on the Type. */
    union
    {
        /** The unknown value (RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE_UNKNOWN). */
        PRTASN1CORE                         pCore;
        /** Page hashes (RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE_PAGE_HASHES_V1 or
         *  RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE_PAGE_HASHES_V2). */
        PRTCRSPCSERIALIZEDPAGEHASHES        pPageHashes;
    } u;
} RTCRSPCSERIALIZEDOBJECTATTRIBUTE;
/** Pointer to a serialized object attribute.  */
typedef RTCRSPCSERIALIZEDOBJECTATTRIBUTE *PRTCRSPCSERIALIZEDOBJECTATTRIBUTE;
/** Pointer to a const serialized object attribute.  */
typedef RTCRSPCSERIALIZEDOBJECTATTRIBUTE const *PCRTCRSPCSERIALIZEDOBJECTATTRIBUTE;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRSPCSERIALIZEDOBJECTATTRIBUTE, RTDECL, RTCrSpcSerializedObjectAttribute, SeqCore.Asn1Core);

RTDECL(int) RTCrSpcSerializedObjectAttribute_SetV1Hashes(PRTCRSPCSERIALIZEDOBJECTATTRIBUTE pThis,
                                                         PCRTCRSPCSERIALIZEDPAGEHASHES, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrSpcSerializedObjectAttribute_SetV2Hashes(PRTCRSPCSERIALIZEDOBJECTATTRIBUTE pThis,
                                                         PCRTCRSPCSERIALIZEDPAGEHASHES, PCRTASN1ALLOCATORVTABLE pAllocator);

/** @name RTCRSPCSERIALIZEDOBJECTATTRIBUTE::Type values
 * @{ */
/** Serialized object attribute type for page hashes version 1. */
#define RTCRSPC_PE_IMAGE_HASHES_V1_OID  "1.3.6.1.4.1.311.2.3.1"
/** Serialized object attribute type for page hashes version 2. */
#define RTCRSPC_PE_IMAGE_HASHES_V2_OID  "1.3.6.1.4.1.311.2.3.2"
/** @} */


/*
 * Set of serialized object attributes (PE image data).
 */
RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTCRSPCSERIALIZEDOBJECTATTRIBUTES, RTCRSPCSERIALIZEDOBJECTATTRIBUTE, RTDECL,
                                           RTCrSpcSerializedObjectAttributes);

/** The UUID found in RTCRSPCSERIALIZEDOBJECT::Uuid for
 *  RTCRSPCSERIALIZEDOBJECTATTRIBUTES. */
#define RTCRSPCSERIALIZEDOBJECT_UUID_STR "d586b5a6-a1b4-6624-ae05-a217da8e60d6"


/**
 * Decoded encapsulated data type selection in RTCRSPCSERIALIZEDOBJECT.
 */
typedef enum RTCRSPCSERIALIZEDOBJECTTYPE
{
    /** Invalid zero value. */
    RTCRSPCSERIALIZEDOBJECTTYPE_INVALID = 0,
    /** Serialized object attributes (RTCRSPCSERIALIZEDOBJECT_UUID_STR / pAttribs). */
    RTCRSPCSERIALIZEDOBJECTTYPE_ATTRIBUTES,
    /** End of valid values. */
    RTCRSPCSERIALIZEDOBJECTTYPE_END,
    /** MAke sure the type is at least 32-bit wide. */
    RTCRSPCSERIALIZEDOBJECTTYPE_32BIT_HACK = 0x7fffffff
} RTCRSPCSERIALIZEDOBJECTTYPE;

/**
 * A serialized object (PE image data).
 */
typedef struct RTCRSPCSERIALIZEDOBJECT
{
    /** Sequence core. */
    RTASN1SEQUENCECORE                  SeqCore;
    /** The UUID of the data object. */
    RTASN1OCTETSTRING                   Uuid;
    /** Serialized data object. */
    RTASN1OCTETSTRING                   SerializedData;

    /** Indicates the valid pointer in the union. */
    RTCRSPCSERIALIZEDOBJECTTYPE         enmType;
    /** Union of pointers shadowing SerializedData.pEncapsulated.  */
    union
    {
        /** Generic core pointer. */
        PRTASN1CORE                         pCore;
        /** Pointer to decoded data if Uuid is RTCRSPCSERIALIZEDOBJECT_UUID_STR. */
        PRTCRSPCSERIALIZEDOBJECTATTRIBUTES  pData;
    } u;
} RTCRSPCSERIALIZEDOBJECT;
/** Pointer to a serialized object (PE image data). */
typedef RTCRSPCSERIALIZEDOBJECT *PRTCRSPCSERIALIZEDOBJECT;
/** Pointer to a const serialized object (PE image data). */
typedef RTCRSPCSERIALIZEDOBJECT const *PCRTCRSPCSERIALIZEDOBJECT;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRSPCSERIALIZEDOBJECT, RTDECL, RTCrSpcSerializedObject, SeqCore.Asn1Core);


/**
 * RTCRSPCSTRING choices.
 */
typedef enum RTCRSPCSTRINGCHOICE
{
    /** Invalid zero value.  */
    RTCRSPCSTRINGCHOICE_INVALID = 0,
    /** Not present. */
    RTCRSPCSTRINGCHOICE_NOT_PRESENT,
    /** UCS-2 string (pUcs2). */
    RTCRSPCSTRINGCHOICE_UCS2,
    /** ASCII string (pAscii). */
    RTCRSPCSTRINGCHOICE_ASCII,
    /** End of valid values. */
    RTCRSPCSTRINGCHOICE_END,
    /** Blow the type up to 32-bit. */
    RTCRSPCSTRINGCHOICE_32BIT_HACK = 0x7fffffff
} RTCRSPCSTRINGCHOICE;

/**
 * Stupid microsoft choosy string type.
 */
typedef struct RTCRSPCSTRING
{
    /** Dummy core. */
    RTASN1DUMMY                         Dummy;
    /** Allocation of what the pointer below points to. */
    RTASN1ALLOCATION                    Allocation;
    /** Pointer choice.*/
    RTCRSPCSTRINGCHOICE                 enmChoice;
    /** Pointer union. */
    union
    {
        /** Tag 0, implicit: UCS-2 (BMP) string.  */
        PRTASN1STRING                   pUcs2;
        /** Tag 1, implicit: ASCII (IA5) string.  */
        PRTASN1STRING                   pAscii;
    } u;
} RTCRSPCSTRING;
/** Pointer to a stupid microsoft string choice.  */
typedef RTCRSPCSTRING *PRTCRSPCSTRING;
/** Pointer to a const stupid microsoft string choice.  */
typedef RTCRSPCSTRING const *PCRTCRSPCSTRING;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRSPCSTRING, RTDECL, RTCrSpcString, Dummy.Asn1Core);

RTDECL(int) RTCrSpcString_SetUcs2(PRTCRSPCSTRING pThis, PCRTASN1STRING pToClone, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrSpcString_SetAscii(PRTCRSPCSTRING pThis, PCRTASN1STRING pToClone, PCRTASN1ALLOCATORVTABLE pAllocator);


/**
 * RTCRSPCSTRING choices.
 */
typedef enum RTCRSPCLINKCHOICE
{
    /** Invalid zero value.  */
    RTCRSPCLINKCHOICE_INVALID = 0,
    /** Not present. */
    RTCRSPCLINKCHOICE_NOT_PRESENT,
    /** URL (ASCII) string (pUrl). */
    RTCRSPCLINKCHOICE_URL,
    /** Serialized object (pMoniker). */
    RTCRSPCLINKCHOICE_MONIKER,
    /** Filename (pT2). */
    RTCRSPCLINKCHOICE_FILE,
    /** End of valid values. */
    RTCRSPCLINKCHOICE_END,
    /** Blow the type up to 32-bit. */
    RTCRSPCLINKCHOICE_32BIT_HACK = 0x7fffffff
} RTCRSPCLINKCHOICE;

/**
 * PE image data link.
 */
typedef struct RTCRSPCLINK
{
    /** Dummy core. */
    RTASN1DUMMY                         Dummy;
    /** Allocation of what the pointer below points to. */
    RTASN1ALLOCATION                    Allocation;
    /** Pointer choice.*/
    RTCRSPCLINKCHOICE                   enmChoice;
    /** Pointer union. */
    union
    {
        /** Tag 0, implicit: An URL encoded as an IA5 STRING.  */
        PRTASN1STRING                   pUrl;
        /** Tag 1, implicit: A serialized object.  */
        PRTCRSPCSERIALIZEDOBJECT        pMoniker;
        /** Tag 2, explicit: The default, a file name.
         * Documented to be set to "<<<Obsolete>>>" when used. */
        struct
        {
            /** Context tag 2. */
            RTASN1CONTEXTTAG2           CtxTag2;
            /** The file name string. */
            RTCRSPCSTRING               File;
        } *pT2;
    } u;
} RTCRSPCLINK;
/** Poitner to a PE image data link. */
typedef RTCRSPCLINK *PRTCRSPCLINK;
/** Poitner to a const PE image data link. */
typedef RTCRSPCLINK const *PCRTCRSPCLINK;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRSPCLINK, RTDECL, RTCrSpcLink, Dummy.Asn1Core);

RTDECL(int) RTCrSpcLink_SetUrl(PRTCRSPCLINK pThis, PCRTASN1STRING pToClone, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrSpcLink_SetMoniker(PRTCRSPCLINK pThis, PCRTCRSPCSERIALIZEDOBJECT pToClone, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrSpcLink_SetFile(PRTCRSPCLINK pThis, PCRTCRSPCSTRING pToClone, PCRTASN1ALLOCATORVTABLE pAllocator);


#if 0 /** @todo Might not be the correct bit order. */
/**
 * Flag values for RTCRSPCPEIMAGEDATA::Flags and RTCRSPCPEIMAGEDATA::fFlags.
 */
typedef enum RTCRSPCPEIMAGEFLAGS
{
    RTCRSPCPEIMAGEFLAGS_INCLUDE_RESOURCES = 0,
    RTCRSPCPEIMAGEFLAGS_INCLUDE_DEBUG_INFO = 1,
    RTCRSPCPEIMAGEFLAGS_IMPORT_ADDRESS_TABLE = 2
} RTCRSPCPEIMAGEFLAGS;
#endif


/**
 * Authenticode PE Image data.
 */
typedef struct RTCRSPCPEIMAGEDATA
{
    /** Sequence core. */
    RTASN1SEQUENCECORE                  SeqCore;
    /** One of the RTCRSPCPEIMAGEFLAGS value, default is
     * RTCRSPCPEIMAGEFLAGS_INCLUDE_RESOURCES.  Obsolete with v2 page hashes? */
    RTASN1BITSTRING                     Flags;
    /** Tag 0, explicit: Link to the data. */
    struct
    {
        /** Context tag 0. */
        RTASN1CONTEXTTAG0               CtxTag0;
        /** Link to the data. */
        RTCRSPCLINK                     File;
    } T0;
} RTCRSPCPEIMAGEDATA;
/** Pointer to a authenticode PE image data representation. */
typedef RTCRSPCPEIMAGEDATA *PRTCRSPCPEIMAGEDATA;
/** Pointer to a const authenticode PE image data representation. */
typedef RTCRSPCPEIMAGEDATA const *PCRTCRSPCPEIMAGEDATA;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRSPCPEIMAGEDATA, RTDECL, RTCrSpcPeImageData, SeqCore.Asn1Core);

RTDECL(int) RTCrSpcPeImageData_SetFlags(PRTCRSPCPEIMAGEDATA pThis, PCRTASN1BITSTRING pToClone, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTCrSpcPeImageData_SetFile(PRTCRSPCPEIMAGEDATA pThis, PCRTCRSPCLINK pToClone, PCRTASN1ALLOCATORVTABLE pAllocator);

/** The object ID for SpcPeImageData. */
#define RTCRSPCPEIMAGEDATA_OID            "1.3.6.1.4.1.311.2.1.15"


/**
 * Data type selection for RTCRSPCATTRIBUTETYPEANDOPTIONALVALUE.
 */
typedef enum RTCRSPCAAOVTYPE
{
    /** Invalid zero entry. */
    RTCRSPCAAOVTYPE_INVALID = 0,
    /** Not present (pro forma). */
    RTCRSPCAAOVTYPE_NOT_PRESENT,
    /** Unknown object. */
    RTCRSPCAAOVTYPE_UNKNOWN,
    /** PE image data (pPeImage). */
    RTCRSPCAAOVTYPE_PE_IMAGE_DATA,
    /** End of valid values. */
    RTCRSPCAAOVTYPE_END,
    /** Blow up the type to at least 32-bits. */
    RTCRSPCAAOVTYPE_32BIT_HACK
} RTCRSPCAAOVTYPE;

/**
 * Authenticode attribute type and optional value.
 *
 * Note! Spec says the value should be explicitly tagged, but in real life
 *       it isn't.  So, not very optional?
 */
typedef struct RTCRSPCATTRIBUTETYPEANDOPTIONALVALUE
{
    /** Sequence core. */
    RTASN1SEQUENCECORE                  SeqCore;
    /** An object ID indicating the type of the value. */
    RTASN1OBJID                         Type;
    /** Allocation of the optional data value. */
    RTASN1ALLOCATION                    Allocation;
    /** The valid pointer. */
    RTCRSPCAAOVTYPE                     enmType;
    /** The value part depends on the Type. */
    union
    {
        /** RTCRSPCAAOVTYPE_UNKNOWN / Generic.  */
        PRTASN1CORE                     pCore;
        /** RTCRSPCAAOVTYPE_PE_IMAGE_DATA / RTCRSPCPEIMAGEDATA_OID. */
        PRTCRSPCPEIMAGEDATA             pPeImage;
    } uValue;
} RTCRSPCATTRIBUTETYPEANDOPTIONALVALUE;
/** Pointer to a authentication attribute type and optional value
 *  representation. */
typedef RTCRSPCATTRIBUTETYPEANDOPTIONALVALUE *PRTCRSPCATTRIBUTETYPEANDOPTIONALVALUE;
/** Pointer to a const authentication attribute type and optional value
 *  representation. */
typedef RTCRSPCATTRIBUTETYPEANDOPTIONALVALUE const *PCRTCRSPCATTRIBUTETYPEANDOPTIONALVALUE;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRSPCATTRIBUTETYPEANDOPTIONALVALUE, RTDECL, RTCrSpcAttributeTypeAndOptionalValue, SeqCore.Asn1Core);

RTDECL(int) RTCrSpcAttributeTypeAndOptionalValue_SetPeImage(PRTCRSPCATTRIBUTETYPEANDOPTIONALVALUE pThis,
                                                            PCRTCRSPCPEIMAGEDATA pToClone, PCRTASN1ALLOCATORVTABLE pAllocator);

/**
 * Authenticode indirect data content.
 */
typedef struct RTCRSPCINDIRECTDATACONTENT
{
    /** Sequence core. */
    RTASN1SEQUENCECORE                  SeqCore;
    /** Additional data. */
    RTCRSPCATTRIBUTETYPEANDOPTIONALVALUE Data;
    /** The whole image digest. */
    RTCRPKCS7DIGESTINFO                 DigestInfo;
} RTCRSPCINDIRECTDATACONTENT;
/** Pointer to a authenticode indirect data content representation. */
typedef RTCRSPCINDIRECTDATACONTENT *PRTCRSPCINDIRECTDATACONTENT;
/** Pointer to a const authenticode indirect data content representation. */
typedef RTCRSPCINDIRECTDATACONTENT const *PCRTCRSPCINDIRECTDATACONTENT;
RTASN1TYPE_STANDARD_PROTOTYPES(RTCRSPCINDIRECTDATACONTENT, RTDECL, RTCrSpcIndirectDataContent, SeqCore.Asn1Core);

/** The object ID for SpcIndirectDataContent. */
#define RTCRSPCINDIRECTDATACONTENT_OID    "1.3.6.1.4.1.311.2.1.4"

/**
 * Check the sanity of an Authenticode SPCIndirectDataContent object.
 *
 * @returns IPRT status code
 * @param   pIndData            The Authenticode SPCIndirectDataContent to
 *                              check.
 * @param   pSignedData         The related signed data object.
 * @param   fFlags              RTCRSPCINDIRECTDATACONTENT_SANITY_F_XXX.
 * @param   pErrInfo            Optional error info.
 */
RTDECL(int) RTCrSpcIndirectDataContent_CheckSanityEx(PCRTCRSPCINDIRECTDATACONTENT pIndData, PCRTCRPKCS7SIGNEDDATA pSignedData,
                                                     uint32_t fFlags, PRTERRINFO pErrInfo);
/** @name RTCRSPCINDIRECTDATACONTENT_SANITY_F_XXX for RTCrSpcIndirectDataContent_CheckSanityEx.
 * @{  */
/** The digest hash algorithm must be known to IPRT. */
#define RTCRSPCINDIRECTDATACONTENT_SANITY_F_ONLY_KNOWN_HASH   RT_BIT_32(0)
/** PE image signing, check expectations of the spec.  */
#define RTCRSPCINDIRECTDATACONTENT_SANITY_F_PE_IMAGE          RT_BIT_32(1)
/** @} */

/**
 * Gets the first SPC serialized object attribute in a SPC PE image.
 *
 * @returns Pointer to the attribute with the given type, NULL if not found.
 * @param   pThis               The Authenticode SpcIndirectDataContent.
 * @param   enmType             The type of attribute to get.
 */
RTDECL(PCRTCRSPCSERIALIZEDOBJECTATTRIBUTE)
RTCrSpcIndirectDataContent_GetPeImageObjAttrib(PCRTCRSPCINDIRECTDATACONTENT pThis,
                                               RTCRSPCSERIALIZEDOBJECTATTRIBUTETYPE enmType);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crypto_spc_h */

