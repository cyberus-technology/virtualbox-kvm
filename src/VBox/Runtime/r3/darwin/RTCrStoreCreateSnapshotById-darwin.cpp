/* $Id: RTCrStoreCreateSnapshotById-darwin.cpp $ */
/** @file
 * IPRT - RTCrStoreCreateSnapshotById, Darwin.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/crypto/store.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/file.h>

/* HACK ALERT! Shut up those deprecated messages on SecKeychainSearchCreateFromAttributes and SecKeychainSearchCopyNext. */
#include <CoreFoundation/CoreFoundation.h>
#undef  DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER
#define DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER

#include <Security/Security.h>


/**
 * Checks the trust settings of the certificate.
 *
 * @returns true if not out-right distructed, otherwise false.
 * @param   hCert           The certificate.
 * @param   enmTrustDomain  The trust settings domain to check relative to.
 */
static bool rtCrStoreIsDarwinCertTrustworthy(SecCertificateRef hCert, SecTrustSettingsDomain enmTrustDomain)
{
    bool fResult = true;
    CFArrayRef hTrustSettings;
    OSStatus orc = SecTrustSettingsCopyTrustSettings(hCert, enmTrustDomain, &hTrustSettings);
    if (orc == noErr)
    {
        CFIndex const cTrustSettings = CFArrayGetCount(hTrustSettings);
        for (CFIndex i = 0; i < cTrustSettings; i++)
        {
            CFDictionaryRef hDict = (CFDictionaryRef)CFArrayGetValueAtIndex(hTrustSettings, i);
            AssertContinue(CFGetTypeID(hDict) == CFDictionaryGetTypeID());

            CFNumberRef hNum = (CFNumberRef)CFDictionaryGetValue(hDict, kSecTrustSettingsResult);
            if (hNum)
            {
                AssertContinue(CFGetTypeID(hNum) == CFNumberGetTypeID());
                SInt32 iNum;
                if (CFNumberGetValue(hNum, kCFNumberSInt32Type, &iNum))
                {
                    if (iNum == kSecTrustSettingsResultDeny)
                    {
                        fResult = false;
                        break;
                    }
                }
                /* No need to release hNum (get rule). */
            }
            /* No need to release hDict (get rule). */
        }
        CFRelease(hTrustSettings);
    }
    else if (orc != errSecItemNotFound)
    {
        AssertFailed();
        fResult = false;
    }
    return fResult;
}


static int rtCrStoreAddCertsFromNativeKeychain(RTCRSTORE hStore, SecKeychainRef hKeychain, SecTrustSettingsDomain enmTrustDomain,
                                               int rc, PRTERRINFO pErrInfo)
{
    /** @todo The SecKeychainSearchCreateFromAttributes and
     * SecKeychainSearchCopyNext APIs have been officially deprecated since 10.7
     * according to the header files.  However, the perferred API,
     * SecItemCopyMatching (and possibly SecTrustCopyAnchorCertificates) would
     * require a larger rewrite here and that's just not worth it right now.  We can
     * do that should these APIs be removed (unlikely given the amount of grep hits
     * in the public 10.15.3 sources). */

    /*
     * Enumerate the certificates in the keychain.
     */
    RT_GCC_NO_WARN_DEPRECATED_BEGIN
    SecKeychainSearchRef hSearch;
    OSStatus orc = SecKeychainSearchCreateFromAttributes(hKeychain, kSecCertificateItemClass, NULL, &hSearch);
    if (orc == noErr)
    {
        SecKeychainItemRef hItem;
        while ((orc = SecKeychainSearchCopyNext(hSearch, &hItem)) == noErr)
        {
            Assert(CFGetTypeID(hItem) == SecCertificateGetTypeID());
            SecCertificateRef hCert = (SecCertificateRef)hItem;

            /*
             * Check if the current certificate is at all trusted, skip it if it's isn't.
             */
            if (rtCrStoreIsDarwinCertTrustworthy(hCert, enmTrustDomain))
            {
                /*
                 * Get the certificate data.
                 */
                CFDataRef hEncodedCert = SecCertificateCopyData(hCert);
                Assert(hEncodedCert);
                if (hEncodedCert)
                {
                    CFIndex         cbEncoded = CFDataGetLength(hEncodedCert);
                    const uint8_t  *pbEncoded = CFDataGetBytePtr(hEncodedCert);

                    RTERRINFOSTATIC StaticErrInfo;
                    int rc2 = RTCrStoreCertAddEncoded(hStore, RTCRCERTCTX_F_ENC_X509_DER | RTCRCERTCTX_F_ADD_IF_NOT_FOUND,
                                                      pbEncoded, cbEncoded, RTErrInfoInitStatic(&StaticErrInfo));
                    if (RT_FAILURE(rc2))
                    {
                        if (RTErrInfoIsSet(&StaticErrInfo.Core))
                            RTErrInfoAddF(pErrInfo, -rc2, "  %s", StaticErrInfo.Core.pszMsg);
                        else
                            RTErrInfoAddF(pErrInfo, -rc2, "  %Rrc adding cert", rc2);
                        rc = -rc2;
                    }

                    CFRelease(hEncodedCert);
                }
            }

            CFRelease(hItem);
        }
        if (orc != errSecItemNotFound)
            rc = RTErrInfoAddF(pErrInfo, -VERR_SEARCH_ERROR,
                               "  SecKeychainSearchCopyNext failed with %#x", orc);
        CFRelease(hSearch);
    }
    else
        rc = RTErrInfoAddF(pErrInfo, -VERR_SEARCH_ERROR,
                           "  SecKeychainSearchCreateFromAttributes failed with %#x", orc);
    RT_GCC_NO_WARN_DEPRECATED_END
    return rc;
}


static int rtCrStoreAddCertsFromNativeKeychainFile(RTCRSTORE hStore, const char *pszKeychain,
                                                   SecTrustSettingsDomain enmTrustDomain,
                                                   int rc, PRTERRINFO pErrInfo)
{
    /*
     * Open the keychain and call common worker to do the job.
     */
    SecKeychainRef hKeychain;
    OSStatus orc = SecKeychainOpen(pszKeychain, &hKeychain);
    if (orc == noErr)
    {
        rc = rtCrStoreAddCertsFromNativeKeychain(hStore, hKeychain, enmTrustDomain, rc, pErrInfo);

        CFRelease(hKeychain);
    }
    else if (RTFileExists(pszKeychain))
        rc = RTErrInfoAddF(pErrInfo, -VERR_OPEN_FAILED, "  SecKeychainOpen failed with %#x on '%s'", orc, pszKeychain);
    return rc;
}


static int rtCrStoreAddCertsFromNativeKeystoreDomain(RTCRSTORE hStore, SecPreferencesDomain enmDomain,
                                                     SecTrustSettingsDomain enmTrustDomain,
                                                     int rc, PRTERRINFO pErrInfo)
{
    /*
     * Get a list of keystores for this domain and call common worker on each.
     */
    CFArrayRef hKeychains;
    OSStatus orc = SecKeychainCopyDomainSearchList(enmDomain, &hKeychains);
    if (orc == noErr)
    {
        CFIndex const cEntries = CFArrayGetCount(hKeychains);
        for (CFIndex i = 0; i < cEntries; i++)
        {
            SecKeychainRef hKeychain = (SecKeychainRef)CFArrayGetValueAtIndex(hKeychains, i);
            Assert(CFGetTypeID(hKeychain) == SecKeychainGetTypeID());
            CFRetain(hKeychain);

            rc = rtCrStoreAddCertsFromNativeKeychain(hStore, hKeychain, enmTrustDomain, rc, pErrInfo);

            CFRelease(hKeychain);
        }

        CFRelease(hKeychains);
    }
    else
        rc = RTErrInfoAddF(pErrInfo, -VERR_SEARCH_ERROR,
                           " SecKeychainCopyDomainSearchList failed with %#x on %d", orc, enmDomain);
    return rc;
}


RTDECL(int) RTCrStoreCreateSnapshotById(PRTCRSTORE phStore, RTCRSTOREID enmStoreId, PRTERRINFO pErrInfo)
{
    AssertReturn(enmStoreId > RTCRSTOREID_INVALID && enmStoreId < RTCRSTOREID_END, VERR_INVALID_PARAMETER);

    /*
     * Create an empty in-memory store.
     */
    RTCRSTORE hStore;
    int rc = RTCrStoreCreateInMem(&hStore, 128);
    if (RT_SUCCESS(rc))
    {
        *phStore = hStore;

        /*
         * Load the certificates corresponding to the given virtual store ID.
         */
        switch (enmStoreId)
        {
            case RTCRSTOREID_USER_TRUSTED_CAS_AND_CERTIFICATES:
                rc = rtCrStoreAddCertsFromNativeKeystoreDomain(hStore, kSecPreferencesDomainUser,
                                                               kSecTrustSettingsDomainUser, rc, pErrInfo);
                break;

            case RTCRSTOREID_SYSTEM_TRUSTED_CAS_AND_CERTIFICATES:
                rc = rtCrStoreAddCertsFromNativeKeystoreDomain(hStore, kSecPreferencesDomainSystem,
                                                               kSecTrustSettingsDomainSystem, rc, pErrInfo);
                rc = rtCrStoreAddCertsFromNativeKeychainFile(hStore,
                                                             "/System/Library/Keychains/SystemRootCertificates.keychain",
                                                             kSecTrustSettingsDomainSystem, rc, pErrInfo);
                break;

            default:
                AssertFailed(); /* implement me */
        }
    }
    else
        RTErrInfoSet(pErrInfo, rc, "RTCrStoreCreateInMem failed");
    return rc;
}
RT_EXPORT_SYMBOL(RTCrStoreCreateSnapshotById);

