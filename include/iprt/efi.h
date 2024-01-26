/** @file
 * IPRT - EFI related utilities.
 */

/*
 * Copyright (C) 2021-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_efi_h
#define IPRT_INCLUDED_efi_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/time.h>
#include <iprt/vfs.h>

#include <iprt/formats/efi-common.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_efi    RTEfi - EFI utilities
 * @ingroup grp_rt
 * @{
 */


#ifdef IN_RING3

/**
 * Converts an EFI time to a time spec (UTC).
 *
 * @returns pTimeSpec on success.
 * @returns NULL if the pEfiTime data is invalid.
 * @param   pTimeSpec   Where to store the converted time.
 * @param   pEfiTime    Pointer to the EFI time struct.
 */
RTDECL(PRTTIMESPEC) RTEfiTimeToTimeSpec(PRTTIMESPEC pTimeSpec, PCEFI_TIME pEfiTime);


/**
 * Converts a time spec (UTC) to an EFI time.
 *
 * @returns pEfiTime on success.
 * @returns NULL if the pTimeSpec data is invalid.
 * @param   pEfiTime    Pointer to the EFI time struct.
 * @param   pTimeSpec   The time spec to convert.
 */
RTDECL(PEFI_TIME) RTEfiTimeFromTimeSpec(PEFI_TIME pEfiTime, PCRTTIMESPEC pTimeSpec);


/**
 * Converts the given EFI GUID to the IPRT UUID representation.
 *
 * @returns pUuid.
 * @param   pUuid       Where to store the converted GUID.
 * @param   pEfiGuid    The EFI GUID to convert.
 */
RTDECL(PRTUUID) RTEfiGuidToUuid(PRTUUID pUuid, PCEFI_GUID pEfiGuid);


/**
 * Converts the given EFI GUID to the IPRT UUID representation.
 *
 * @returns pEfiGuid.
 * @param   pEfiGuid    Where to store the converted UUID.
 * @param   pUuid       The UUID to convert.
 */
RTDECL(PEFI_GUID) RTEfiGuidFromUuid(PEFI_GUID pEfiGuid, PCRTUUID pUuid);


/**
 * Compares two EFI GUID values.
 *
 * @returns 0 if eq, < 0 or > 0.
 * @param   pGuid1          First value to compare.
 * @param   pGuid2          Second value to compare.
 */
RTDECL(int)  RTEfiGuidCompare(PCEFI_GUID pGuid1, PCEFI_GUID pGuid2);


/**
 * Opens an EFI variable store.
 *
 * @returns IPRT status code.
 * @param   hVfsFileIn      The file or device backing the store.
 * @param   fMntFlags       RTVFSMNT_F_XXX.
 * @param   fVarStoreFlags  Reserved, MBZ.
 * @param   phVfs           Where to return the virtual file system handle.
 * @param   pErrInfo        Where to return additional error information.
 */
RTDECL(int) RTEfiVarStoreOpenAsVfs(RTVFSFILE hVfsFileIn, uint32_t fMntFlags, uint32_t fVarStoreFlags, PRTVFS phVfs, PRTERRINFO pErrInfo);


/** @name RTEFIVARSTORE_CREATE_F_XXX - RTEfiVarStoreCreate flags
 * @{ */
/** Use default options. */
#define RTEFIVARSTORE_CREATE_F_DEFAULT                  UINT32_C(0)
/** Don't create a fault tolerant write working space.
 * The default is to create one reducing the size of the variable store. */
#define RTEFIVARSTORE_CREATE_F_NO_FTW_WORKING_SPACE     RT_BIT_32(0)
/** Mask containing all valid flags.   */
#define RTEFIVARSTORE_CREATE_F_VALID_MASK               UINT32_C(0x00000001)
/** @} */

/**
 * Creates a new EFI variable store.
 *
 * @returns IRPT status code.
 * @param   hVfsFile            The store file.
 * @param   offStore            The offset into @a hVfsFile of the file.
 *                              Typically 0.
 * @param   cbStore             The size of the variable store.  Pass 0 if the rest of
 *                              hVfsFile should be used. The remaining space for variables
 *                              will be less because of some metadata overhead.
 * @param   fFlags              See RTEFIVARSTORE_F_XXX.
 * @param   cbBlock             The logical block size.
 * @param   pErrInfo            Additional error information, maybe.  Optional.
 */
RTDECL(int) RTEfiVarStoreCreate(RTVFSFILE hVfsFile, uint64_t offStore, uint64_t cbStore, uint32_t fFlags, uint32_t cbBlock,
                                PRTERRINFO pErrInfo);


/**
 * EFI signature type.
 */
typedef enum RTEFISIGTYPE
{
    /** Invalid type, do not use. */
    RTEFISIGTYPE_INVALID = 0,
    /** First valid signature type. */
    RTEFISIGTYPE_FIRST_VALID,
    /** Signature contains a SHA256 hash. */
    RTEFISIGTYPE_SHA256 = RTEFISIGTYPE_FIRST_VALID,
    /** Signature contains a RSA2048 key (only the modulus in big endian form,
     * the exponent is always 65537/0x10001). */
    RTEFISIGTYPE_RSA2048,
    /** Signature contains a RSA2048 signature of a SHA256 hash. */
    RTEFISIGTYPE_RSA2048_SHA256,
    /** Signature contains a SHA1 hash. */
    RTEFISIGTYPE_SHA1,
    /** Signature contains a RSA2048 signature of a SHA1 hash. */
    RTEFISIGTYPE_RSA2048_SHA1,
    /** Signature contains a DER encoded X.509 certificate. */
    RTEFISIGTYPE_X509,
    /** First invalid type (do not use). */
    RTEFISIGTYPE_FIRST_INVALID,
    /** 32bit blowup hack.*/
    RTEFISIGTYPE_32BIT_HACK = 0x7fffffff
} RTEFISIGTYPE;


/**
 * EFI signature database enumeration callback.
 *
 * @returns IPRT status code, any status code other than VINF_SUCCESS will abort the enumeration.
 * @param   hEfiSigDb           Handle to the EFI signature database this callback is called on.
 * @param   enmSigType          The signature type.
 * @param   pUuidOwner          Signature owner UUID.
 * @param   pvSig               The signature data (dependent on the type).
 * @param   cbSig               Size of the signature in bytes.
 * @param   pvUser              Opaque user data passed in RTEfiSigDbEnum().
 */
typedef DECLCALLBACKTYPE(int, FNRTEFISIGDBENUMSIG,(RTEFISIGDB hEfiSigDb, RTEFISIGTYPE enmSigType, PCRTUUID pUuidOwner,
                                                   const void *pvSig, size_t cbSig, void *pvUser));
/** Pointer to a EFI signature database enumeration callback. */
typedef FNRTEFISIGDBENUMSIG *PFNRTEFISIGDBENUMSIG;


/**
 * Creates an empty EFI signature database.
 *
 * @returns IPRT status code.
 * @param   phEfiSigDb          Where to store the handle to the empty EFI signature database on success.
 */
RTDECL(int) RTEfiSigDbCreate(PRTEFISIGDB phEfiSigDb);


/**
 * Destroys the given EFI signature database handle.
 *
 * @returns IPRT status code.
 * @param   hEfiSigDb           The EFI signature database handle to destroy.
 */
RTDECL(int) RTEfiSigDbDestroy(RTEFISIGDB hEfiSigDb);


/**
 * Adds the signatures from an existing signature database contained in the given file.
 *
 * @returns IPRT status code.
 * @param   hEfiSigDb           The EFI signature database handle.
 * @param   hVfsFileIn          The file handle containing the existing signature database.
 */
RTDECL(int) RTEfiSigDbAddFromExistingDb(RTEFISIGDB hEfiSigDb, RTVFSFILE hVfsFileIn);


/**
 * Adds a new signature to the given signature database from the given file.
 *
 * @returns IPRT status code.
 * @param   hEfiSigDb           The EFI signature database handle.
 * @param   enmSigType          Type of the signature.
 * @param   pUuidOwner          The UUID of the signature owner.
 * @param   hVfsFileIn          File handle containing the signature data.
 */
RTDECL(int) RTEfiSigDbAddSignatureFromFile(RTEFISIGDB hEfiSigDb, RTEFISIGTYPE enmSigType, PCRTUUID pUuidOwner, RTVFSFILE hVfsFileIn);


/**
 * Adds a new signature to the given signature database from the given buffer.
 *
 * @returns IPRT status code.
 * @param   hEfiSigDb           The EFI signature database handle.
 * @param   enmSigType          Type of the signature.
 * @param   pUuidOwner          The UUID of the signature owner.
 * @param   pvBuf               Pointer to the signature data.
 * @param   cbBuf               Size of the signature data in bytes.
 */
RTDECL(int) RTEfiSigDbAddSignatureFromBuf(RTEFISIGDB hEfiSigDb, RTEFISIGTYPE enmSigType, PCRTUUID pUuidOwner,
                                          const void *pvBuf, size_t cbBuf);


/**
 * Writes the given EFI signature database to the given file.
 *
 * @returns IPRT status code.
 * @param   hEfiSigDb           The EFI signature database handle.
 * @param   hVfsFileOut         The file handle to write the signature database to.
 */
RTDECL(int) RTEfiSigDbWriteToFile(RTEFISIGDB hEfiSigDb, RTVFSFILE hVfsFileOut);


/**
 * Enumerate all signatures in the given EFI signature database.
 *
 * @returns IPRT status code.
 * @param   hEfiSigDb           The EFI signature database handle.
 * @param   pfnEnumSig          The callback to call for each signature.
 * @param   pvUser              Opaque user data to pass to the callback.
 */
RTDECL(int) RTEfiSigDbEnum(RTEFISIGDB hEfiSigDb, PFNRTEFISIGDBENUMSIG pfnEnumSig, void *pvUser);


/**
 * Returns a human readable string of the given signature type.
 *
 * @returns Human readable string.
 * @param   enmSigType          The signature type.
 */
RTDECL(const char *) RTEfiSigDbTypeStringify(RTEFISIGTYPE enmSigType);


/**
 * Returns a pointer to the EFI GUID identifying the given signature type.
 *
 * @returns Pointer to the EFI GUID.
 * @param   enmSigType          The signature type.
 */
RTDECL(PCEFI_GUID) RTEfiSigDbTypeGetGuid(RTEFISIGTYPE enmSigType);

#endif /* IN_RING3 */

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_efi_h */

