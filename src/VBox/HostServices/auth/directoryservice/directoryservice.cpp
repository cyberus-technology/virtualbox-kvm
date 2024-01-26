/** @file
 *
 * VirtualBox External Authentication Library:
 * Mac OS X Authentication. This is based on
 * http://developer.apple.com/mac/library/samplecode/CryptNoMore/
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
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <iprt/cdefs.h>
#include <iprt/assert.h>

#include <VBox/VBoxAuth.h>

#include <DirectoryService/DirectoryService.h>

/* Globals */
static const size_t s_cBufferSize = 32 * 1024;

tDirStatus defaultSearchNodePath(tDirReference pDirRef, tDataListPtr *pdsNodePath)
{
    tDirStatus dsErr = eDSNoErr;
    /* Create a buffer for the resulting nodes */
    tDataBufferPtr pTmpBuf = NULL;
    pTmpBuf = dsDataBufferAllocate(pDirRef, s_cBufferSize);
    if (pTmpBuf)
    {
        /* Try to find the default search node for local names */
        UInt32 cNodes;
        tContextData hCtx = 0;
        dsErr = dsFindDirNodes(pDirRef, pTmpBuf, NULL, eDSLocalNodeNames, &cNodes, &hCtx);
        /* Any nodes found? */
        if (   dsErr == eDSNoErr
            && cNodes >= 1)
            /* The first path of the node list is what we looking for. */
            dsErr = dsGetDirNodeName(pDirRef, pTmpBuf, 1, pdsNodePath);
        else
            dsErr = eDSNodeNotFound;

        if (hCtx) /* (DSoNodeConfig.m from DSTools-162 does exactly the same free if not-zero-regardless-of-return-code.) */
            dsReleaseContinueData(pDirRef, hCtx);
        dsDataBufferDeAllocate(pDirRef, pTmpBuf);
    }
    else
        dsErr = eDSAllocationFailed;

    return dsErr;
}

tDirStatus userAuthInfo(tDirReference pDirRef, tDirNodeReference pNodeRef, const char *pszUsername, tDataListPtr *ppAuthNodeListOut)
{
    tDirStatus dsErr = eDSNoErr;
    tDirStatus dsCleanErr = eDSNoErr;
    /* Create a buffer for the resulting authentication info */
    tDataBufferPtr pTmpBuf = dsDataBufferAllocate(pDirRef, s_cBufferSize);
    if (pTmpBuf)
    {
        /* Create the necessary lists for kDSNAttrMetaNodeLocation and kDSNAttrRecordName. */
        tDataListPtr pRecordType = dsBuildListFromStrings(pDirRef, kDSStdRecordTypeUsers, NULL);
        tDataListPtr pRecordName = dsBuildListFromStrings(pDirRef, pszUsername, NULL);
        tDataListPtr pRequestedAttributes = dsBuildListFromStrings(pDirRef, kDSNAttrMetaNodeLocation, kDSNAttrRecordName, NULL);
        if (!(   pRecordType == NULL
              || pRecordName == NULL
              || pRequestedAttributes == NULL))
        {
            /* Now search for the first matching record */
            UInt32 cRecords = 1;
            tContextData hCtx = 0;
            dsErr = dsGetRecordList(pNodeRef,
                                    pTmpBuf,
                                    pRecordName,
                                    eDSExact,
                                    pRecordType,
                                    pRequestedAttributes,
                                    false,
                                    &cRecords,
                                    &hCtx);
            if (   dsErr == eDSNoErr
                && cRecords >= 1)
            {
                /* Process the first found record. Look at any attribute one by one. */
                tAttributeListRef hRecAttrListRef = 0;
                tRecordEntryPtr pRecEntry = NULL;
                tDataListPtr pAuthNodeList = NULL;
                dsErr = dsGetRecordEntry(pNodeRef, pTmpBuf, 1, &hRecAttrListRef, &pRecEntry);
                if (dsErr == eDSNoErr)
                {
                    for (size_t i = 1; i <= pRecEntry->fRecordAttributeCount; ++i)
                    {
                        tAttributeValueListRef hAttrValueListRef = 0;
                        tAttributeEntryPtr pAttrEntry = NULL;
                        /* Get the information for this attribute. */
                        dsErr = dsGetAttributeEntry(pNodeRef, pTmpBuf, hRecAttrListRef, i,
                                                    &hAttrValueListRef, &pAttrEntry);
                        if (dsErr == eDSNoErr)
                        {
                            tAttributeValueEntryPtr pValueEntry = NULL;
                            /* Has any value? */
                            if (pAttrEntry->fAttributeValueCount > 0)
                            {
                                dsErr = dsGetAttributeValue(pNodeRef, pTmpBuf, 1, hAttrValueListRef, &pValueEntry);
                                if (dsErr == eDSNoErr)
                                {
                                    /* Check for kDSNAttrMetaNodeLocation */
                                    if (strcmp(pAttrEntry->fAttributeSignature.fBufferData, kDSNAttrMetaNodeLocation) == 0)
                                    {
                                        /* Convert the meta location attribute to a path node list */
                                        pAuthNodeList = dsBuildFromPath(pDirRef,
                                                                        pValueEntry->fAttributeValueData.fBufferData,
                                                                        "/");
                                        if (pAuthNodeList == NULL)
                                            dsErr = eDSAllocationFailed;
                                    }
                                }
                            }

                            if (pValueEntry != NULL)
                                dsDeallocAttributeValueEntry(pDirRef, pValueEntry);
                            if (hAttrValueListRef)
                                dsCloseAttributeValueList(hAttrValueListRef);
                            if (pAttrEntry != NULL)
                                dsDeallocAttributeEntry(pDirRef, pAttrEntry);

                            if (dsErr != eDSNoErr)
                                break;
                        }
                    }
                }
                /* Copy the results */
                if (dsErr == eDSNoErr)
                {
                    if (pAuthNodeList != NULL)
                    {
                        /* Copy out results. */
                        *ppAuthNodeListOut = pAuthNodeList;
                        pAuthNodeList = NULL;
                    }
                    else
                        dsErr = eDSAttributeNotFound;
                }

                if (pAuthNodeList != NULL)
                {
                    dsCleanErr = dsDataListDeallocate(pDirRef, pAuthNodeList);
                    if (dsCleanErr == eDSNoErr)
                        free(pAuthNodeList);
                }
                if (hRecAttrListRef)
                    dsCloseAttributeList(hRecAttrListRef);
                if (pRecEntry != NULL)
                    dsDeallocRecordEntry(pDirRef, pRecEntry);
            }
            else
                dsErr = eDSRecordNotFound;
            if (hCtx)
                dsReleaseContinueData(pDirRef, hCtx);
        }
        else
            dsErr = eDSAllocationFailed;
        if (pRequestedAttributes != NULL)
        {
            dsCleanErr = dsDataListDeallocate(pDirRef, pRequestedAttributes);
            if (dsCleanErr == eDSNoErr)
                free(pRequestedAttributes);
        }
        if (pRecordName != NULL)
        {
            dsCleanErr = dsDataListDeallocate(pDirRef, pRecordName);
            if (dsCleanErr == eDSNoErr)
                free(pRecordName);
        }
        if (pRecordType != NULL)
        {
            dsCleanErr = dsDataListDeallocate(pDirRef, pRecordType);
            if (dsCleanErr == eDSNoErr)
                free(pRecordType);
        }
        dsDataBufferDeAllocate(pDirRef, pTmpBuf);
    }
    else
        dsErr = eDSAllocationFailed;

    return dsErr;
}

tDirStatus authWithNode(tDirReference pDirRef, tDataListPtr pAuthNodeList, const char *pszUsername, const char *pszPassword)
{
    tDirStatus dsErr = eDSNoErr;
    /* Open the authentication node. */
    tDirNodeReference hAuthNodeRef = 0;
    dsErr = dsOpenDirNode(pDirRef, pAuthNodeList, &hAuthNodeRef);
    if (dsErr == eDSNoErr)
    {
        /* How like we to authenticate! */
        tDataNodePtr pAuthMethod = dsDataNodeAllocateString(pDirRef, kDSStdAuthNodeNativeClearTextOK);
        if (pAuthMethod)
        {
            /* Create the memory holding the authentication data. The data
             * structure consists of 4 byte length of the username + zero byte,
             * the username itself, a 4 byte length of the password & the
             * password itself + zero byte. */
            tDataBufferPtr pAuthOutBuf = dsDataBufferAllocate(pDirRef, s_cBufferSize);
            if (pAuthOutBuf)
            {
                size_t cUserName = strlen(pszUsername) + 1;
                size_t cPassword = strlen(pszPassword) + 1;
                unsigned long cLen = 0;
                tDataBufferPtr pAuthInBuf = dsDataBufferAllocate(pDirRef, sizeof(cLen) + cUserName + sizeof(cLen) + cPassword);
                if (pAuthInBuf)
                {
                    /* Move the data into the buffer. */
                    pAuthInBuf->fBufferLength = 0;
                    /* Length of the username */
                    cLen = cUserName;
                    memcpy(&pAuthInBuf->fBufferData[pAuthInBuf->fBufferLength], &cLen, sizeof(cLen));
                    pAuthInBuf->fBufferLength += sizeof(cLen);
                    /* The username itself */
                    memcpy(&pAuthInBuf->fBufferData[pAuthInBuf->fBufferLength], pszUsername, cUserName);
                    pAuthInBuf->fBufferLength += cUserName;
                    /* Length of the password */
                    cLen = cPassword;
                    memcpy(&pAuthInBuf->fBufferData[pAuthInBuf->fBufferLength], &cLen, sizeof(cLen));
                    pAuthInBuf->fBufferLength += sizeof(cLen);
                    /* The password itself */
                    memcpy(&pAuthInBuf->fBufferData[pAuthInBuf->fBufferLength], pszPassword, cPassword);
                    pAuthInBuf->fBufferLength += cPassword;
                    /* Now authenticate */
                    dsErr = dsDoDirNodeAuth(hAuthNodeRef, pAuthMethod, true, pAuthInBuf, pAuthOutBuf, NULL);
                    /* Clean up. */
                    dsDataBufferDeAllocate(pDirRef, pAuthInBuf);
                }
                else
                    dsErr = eDSAllocationFailed;
                dsDataBufferDeAllocate(pDirRef, pAuthOutBuf);
            }
            else
                dsErr = eDSAllocationFailed;
            dsDataNodeDeAllocate(pDirRef, pAuthMethod);
        }
        else
            dsErr = eDSAllocationFailed;
        dsCloseDirNode(hAuthNodeRef);
    }

    return dsErr;
}

RT_C_DECLS_BEGIN
DECLEXPORT(FNAUTHENTRY3) AuthEntry;
RT_C_DECLS_END

DECLEXPORT(AuthResult) AUTHCALL AuthEntry(const char *pszCaller,
                                          PAUTHUUID pUuid,
                                          AuthGuestJudgement guestJudgement,
                                          const char *pszUser,
                                          const char *pszPassword,
                                          const char *pszDomain,
                                          int fLogon,
                                          unsigned clientId)
{
    RT_NOREF(pszCaller, pUuid, guestJudgement, pszDomain, clientId);

    /* Validate input */
    AssertPtrReturn(pszUser, AuthResultAccessDenied);
    AssertPtrReturn(pszPassword, AuthResultAccessDenied);

    /* Result to a default value */
    AuthResult result = AuthResultAccessDenied;

    /* Only process logon requests. */
    if (!fLogon)
        return result; /* Return value is ignored by the caller. */

    tDirStatus dsErr = eDSNoErr;
    tDirStatus dsCleanErr = eDSNoErr;
    tDirReference hDirRef = 0;
    /* Connect to the Directory Service. */
    dsErr = dsOpenDirService(&hDirRef);
    if (dsErr == eDSNoErr)
    {
        /* Fetch the default search node */
        tDataListPtr pSearchNodeList = NULL;
        dsErr = defaultSearchNodePath(hDirRef, &pSearchNodeList);
        if (dsErr == eDSNoErr)
        {
            /* Open the default search node */
            tDirNodeReference hSearchNodeRef = 0;
            dsErr = dsOpenDirNode(hDirRef, pSearchNodeList, &hSearchNodeRef);
            if (dsErr == eDSNoErr)
            {
                /* Search for the user info, fetch the authentication node &
                 * the authentication user name. This allows the client to
                 * specify a long user name even if the name which is used to
                 * authenticate has the short form. */
                tDataListPtr pAuthNodeList = NULL;
                dsErr = userAuthInfo(hDirRef, hSearchNodeRef, pszUser, &pAuthNodeList);
                if (dsErr == eDSNoErr)
                {
                    /* Open the authentication node and do the authentication. */
                    dsErr = authWithNode(hDirRef, pAuthNodeList, pszUser, pszPassword);
                    if (dsErr == eDSNoErr)
                        result = AuthResultAccessGranted;
                    dsCleanErr = dsDataListDeallocate(hDirRef, pAuthNodeList);
                    if (dsCleanErr == eDSNoErr)
                        free(pAuthNodeList);
                }
                dsCloseDirNode(hSearchNodeRef);
            }
            dsCleanErr = dsDataListDeallocate(hDirRef, pSearchNodeList);
            if (dsCleanErr == eDSNoErr)
                free(pSearchNodeList);
        }
        dsCloseDirService(hDirRef);
    }

    return result;
}

