/** @file
 * VirtualBox - Cryptographic support functions Interface.
 */

/*
 * Copyright (C) 2022-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_VBoxCryptoIf_h
#define VBOX_INCLUDED_VBoxCryptoIf_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/vfs.h>
#include <VBox/types.h>

/** An opaque VBox cryptographic context handle. */
typedef struct VBOXCRYPTOCTXINT *VBOXCRYPTOCTX;
/**Pointer to an opaque VBox cryptographic context handle. */
typedef VBOXCRYPTOCTX *PVBOXCRYPTOCTX;

/** Magic identifying the cryptographic interface (Charles Babbage). */
#define VBOXCRYPTOIF_MAGIC UINT32_C(0x17911226)

/** Pointer to const cryptographic interface. */
typedef const struct VBOXCRYPTOIF *PCVBOXCRYPTOIF;
/**
 * The main cryptographic callbacks interface table.
 */
typedef struct VBOXCRYPTOIF
{
    /** Interface magic, set to VBOXCRYPTOIF_MAGIC. */
    uint32_t                    u32Magic;
    /** Interface version.
     * This is set to VBOXCRYPTOIF_VERSION. */
    uint32_t                    u32Version;
    /** Description string. */
    const char                 *pszDesc;

    /** @name Generic crytographic context operations.
     * @{ */

    /**
     * Creates a new cryptographic context for encryption.
     *
     * @returns VBox status code.
     * @param   pszCipher           The identifier of the cipher to use.
     * @param   pszPassword         Password for encrypting the context.
     * @param   phCryptoCtx         Where to store the handle to the crypto context on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnCryptoCtxCreate, (const char *pszCipher, const char *pszPassword,
                                                   PVBOXCRYPTOCTX phCryptoCtx));

    /**
     * Creates a new cryptographic context for decryption from the given base-64 encoded context.
     *
     * @returns VBox status code.
     * @param   pszStoredCtx        The base-64 encoded context to decrypt with the given password.
     * @param   pszPassword         Password for encrypting the context.
     * @param   phCryptoCtx         Where to store the handle to the crypto context on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnCryptoCtxLoad, (const char *pszStoredCtx, const char *pszPassword,
                                                 PVBOXCRYPTOCTX phCryptoCtx));

    /**
     * Destroys a previously created cryptographic context.
     *
     * @returns VBox status code.
     * @param   hCryptoCtx          Handle of crpytographic context to destroy.
     */
    DECLR3CALLBACKMEMBER(int, pfnCryptoCtxDestroy, (VBOXCRYPTOCTX hCryptoCtx));

    /**
     * Returns the given cryptographic context as a base-64 encoded string.
     *
     * @returns VBox status code.
     * @param   hCryptoCtx          Handle of crpytographic context.
     * @param   ppszStoredCtx       Where to store the base-64 encoded cryptographic context on success.
     *                              Must be freed with RTMemFree() when not required anymore.
     */
    DECLR3CALLBACKMEMBER(int, pfnCryptoCtxSave, (VBOXCRYPTOCTX hCryptoCtx, char **ppszStoredCtx));

    /**
     * Changes the encryption password for the given context.
     *
     * @returns VBox status code.
     * @param   hCryptoCtx          Handle of crpytographic context.
     * @param   pszPassword         New password used for encrypting the DEK.
     */
    DECLR3CALLBACKMEMBER(int, pfnCryptoCtxPasswordChange, (VBOXCRYPTOCTX hCryptoCtx, const char *pszPassword));

    /**
     * Queries the required size of the output buffer for encrypted data. Depends on the cipher.
     *
     * @returns VBox status code.
     * @param   hCryptoCtx          Handle of crpytographic context.
     * @param   cbPlainText         The size of the data to be encrypted.
     * @param   pcbEncrypted        Where to store the size in bytes of the encrypted data on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnCryptoCtxQueryEncryptedSize, (VBOXCRYPTOCTX hCryptoCtx, size_t cbPlaintext,
                                                               size_t *pcbEncrypted));

    /**
     * Queries the required size of the output buffer for decrypted data. Depends on the cipher.
     *
     * @returns VBox status code.
     * @param   hCryptoCtx          Handle of crpytographic context.
     * @param   cbEncrypted         The size of the encrypted chunk before decryption.
     * @param   pcbPlaintext        Where to store the size in bytes of the decrypted data on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnCryptoCtxQueryDecryptedSize, (VBOXCRYPTOCTX hCryptoCtx, size_t cbEncrypted,
                                                               size_t *pcbPlaintext));

    /**
     * Encrypts data.
     *
     * @returns VBox status code.
     * @param   hCryptoCtx          Handle of crpytographic context.
     * @param   fPartial            Only part of data to be encrypted is specified. The encryption
     *                              cipher context will not be closed. Set to false for last piece
     *                              of data, or if data is specified completely.
     *                              Only CTR mode supports partial encryption.
     * @param   pvIV                Pointer to IV. If null it will be generated.
     * @param   cbIV                Size of the IV.
     * @param   pvPlainText         Data to encrypt.
     * @param   cbPlainText         Size of the data in the pvPlainText.
     * @param   pvAuthData          Data used for authenticate the pvPlainText
     * @param   cbAuthData          Size of the pvAuthData
     * @param   pvEncrypted         Buffer to store encrypted data
     * @param   cbEncrypted         Size of the buffer in pvEncrypted
     * @param   pcbEncrypted        Placeholder where the size of the encrypted data returned.
     */
    DECLR3CALLBACKMEMBER(int, pfnCryptoCtxEncrypt, (VBOXCRYPTOCTX hCryptoCtx, bool fPartial, void const *pvIV, size_t cbIV,
                                                    void const *pvPlainText, size_t cbPlainText,
                                                    void const *pvAuthData, size_t cbAuthData,
                                                    void *pvEncrypted, size_t cbEncrypted,
                                                    size_t *pcbEncrypted));

    /**
     * Decrypts data.
     *
     * @returns VBox status code.
     * @param   hCryptoCtx          Handle of crpytographic context.
     * @param   fPartial            Only part of data to be encrypted is specified. The encryption
     *                              cipher context will not be closed. Set to false for last piece
     *                              of data, or if data is specified completely.
     *                              Only CTR mode supports partial encryption.
     * @param   pvEncrypted         Data to decrypt.
     * @param   cbEncrypted         Size of the data in the pvEncrypted.
     * @param   pvAuthData          Data used for authenticate the pvEncrypted
     * @param   cbAuthData          Size of the pvAuthData
     * @param   pvPlainText         Buffer to store decrypted data
     * @param   cbPlainText         Size of the buffer in pvPlainText
     * @param   pcbPlainText        Placeholder where the size of the decrypted data returned.
     */
    DECLR3CALLBACKMEMBER(int, pfnCryptoCtxDecrypt, (VBOXCRYPTOCTX hCryptoCtx, bool fPartial,
                                                    void const *pvEncrypted, size_t cbEncrypted,
                                                    void const *pvAuthData, size_t cbAuthData,
                                                    void *pvPlainText, size_t cbPlainText, size_t *pcbPlainText));
    /** @} */

    /** @name File based cryptographic operations.
     * @{ */
    /**
     * Creates a new VFS file handle for an encrypted or to be encrypted file handle.
     *
     * @returns VBox status code.
     * @param   hVfsFile        The input file handle, a new reference is retained upon success.
     * @param   pszKeyStore     The key store containing the DEK used for encryption.
     * @param   pszPassword     Password encrypting the DEK.
     * @param   phVfsFile       Where to store the handle to the VFS file on success.
     */
    DECLR3CALLBACKMEMBER(int, pfnCryptoFileFromVfsFile, (RTVFSFILE hVfsFile, const char *pszKeyStore, const char *pszPassword,
                                                         PRTVFSFILE phVfsFile));

    /**
     * Opens a new encryption I/O stream.
     *
     * @returns VBox status code.
     * @param   hVfsIosDst      The encrypted output stream (must be writeable).
     *                          The reference is not consumed, instead another
     *                          one is retained.
     * @param   pszKeyStore     The key store containing the DEK used for encryption.
     * @param   pszPassword     Password encrypting the DEK.
     * @param   phVfsIosCrypt   Where to return the crypting input I/O stream handle
     *                          (you write to this).
     */
    DECLR3CALLBACKMEMBER(int, pfnCryptoIoStrmFromVfsIoStrmEncrypt, (RTVFSIOSTREAM hVfsIosDst, const char *pszKeyStore,
                                                                    const char *pszPassword, PRTVFSIOSTREAM phVfsIosCrypt));

    /**
     * Opens a new decryption I/O stream.
     *
     * @returns VBox status code.
     * @param   hVfsIosIn       The encrypted input stream (must be readable).
     *                          The reference is not consumed, instead another
     *                          one is retained.
     * @param   pszKeyStore     The key store containing the DEK used for encryption.
     * @param   pszPassword     Password encrypting the DEK.
     * @param   phVfsIosOut     Where to return the handle to the decrypted I/O stream (read).
     */
    DECLR3CALLBACKMEMBER(int, pfnCryptoIoStrmFromVfsIoStrmDecrypt, (RTVFSIOSTREAM hVfsIosIn, const char *pszKeyStore,
                                                                    const char *pszPassword, PRTVFSIOSTREAM phVfsIosOut));
    /** @} */

    /** @name Keystore related functions.
     * @{ */
    /**
     * Return the encryption parameters and DEK from the base64 encoded key store data.
     *
     * @returns VBox status code.
     * @param   pszEnc         The base64 encoded key store data.
     * @param   pszPassword    The password to use for key decryption.
     *                         If the password is NULL only the cipher is returned.
     * @param   ppbKey         Where to store the DEK on success.
     *                         Must be freed with RTMemSaferFree().
     * @param   pcbKey         Where to store the DEK size in bytes on success.
     * @param   ppszCipher     Where to store the used cipher for the decrypted DEK.
     *                         Must be freed with RTStrFree().
     */
    DECLR3CALLBACKMEMBER(int, pfnCryptoKeyStoreGetDekFromEncoded, (const char *pszEnc, const char *pszPassword,
                                                                   uint8_t **ppbKey, size_t *pcbKey, char **ppszCipher));

    /**
     * Stores the given DEK in a key store protected by the given password.
     *
     * @returns VBox status code.
     * @param   pszPassword    The password to protect the DEK.
     * @param   pbKey          The DEK to protect.
     * @param   cbKey          Size of the DEK to protect.
     * @param   pszCipher      The cipher string associated with the DEK.
     * @param   ppszEnc        Where to store the base64 encoded key store data on success.
     *                         Must be freed with RTMemFree().
     */
    DECLR3CALLBACKMEMBER(int, pfnCryptoKeyStoreCreate, (const char *pszPassword, const uint8_t *pbKey, size_t cbKey,
                                                        const char *pszCipher, char **ppszEnc));
    /** @} */

    DECLR3CALLBACKMEMBER(int, pfnReserved1,(void)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved2,(void)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved3,(void)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved4,(void)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved5,(void)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved6,(void)); /**< Reserved for minor structure revisions. */

    /** Reserved for minor structure revisions. */
    uint32_t                    uReserved7;

    /** End of structure marker (VBOXCRYPTOIF_VERSION). */
    uint32_t                    u32EndMarker;
} VBOXCRYPTOIF;
/** Current version of the VBOXCRYPTOIF structure.  */
#define VBOXCRYPTOIF_VERSION            RT_MAKE_U32(0, 1)


/**
 * The VBoxCrypto entry callback function.
 *
 * @returns VBox status code.
 * @param   ppCryptoIf          Where to store the pointer to the crypto module interface callback table
 *                              on success.
 */
typedef DECLCALLBACKTYPE(int, FNVBOXCRYPTOENTRY,(PCVBOXCRYPTOIF *ppCryptoIf));
/** Pointer to a FNVBOXCRYPTOENTRY. */
typedef FNVBOXCRYPTOENTRY *PFNVBOXCRYPTOENTRY;

/** The name of the crypto module entry point. */
#define VBOX_CRYPTO_MOD_ENTRY_POINT   "VBoxCryptoEntry"


/**
 * Checks if cryptographic interface version is compatible.
 *
 * @returns true if the do, false if they don't.
 * @param   u32Provider     The provider version.
 * @param   u32User         The user version.
 */
#define VBOXCRYPTO_IS_VER_COMPAT(u32Provider, u32User) \
    (    VBOXCRYPTO_IS_MAJOR_VER_EQUAL(u32Provider, u32User) \
      && (int32_t)RT_LOWORD(u32Provider) >= (int32_t)RT_LOWORD(u32User) ) /* stupid casts to shut up gcc */

/**
 * Check if two cryptographic interface versions have the same major version.
 *
 * @returns true if the do, false if they don't.
 * @param   u32Ver1         The first version number.
 * @param   u32Ver2         The second version number.
 */
#define VBOXCRYPTO_IS_MAJOR_VER_EQUAL(u32Ver1, u32Ver2)  (RT_HIWORD(u32Ver1) == RT_HIWORD(u32Ver2))

#endif /* !VBOX_INCLUDED_VBoxCryptoIf_h */

