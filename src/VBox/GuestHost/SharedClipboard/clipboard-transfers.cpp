/* $Id: clipboard-transfers.cpp $ */
/** @file
 * Shared Clipboard: Common Shared Clipboard transfer handling code.
 */

/*
 * Copyright (C) 2019-2023 Oracle and/or its affiliates.
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

#define LOG_GROUP LOG_GROUP_SHARED_CLIPBOARD
#include <VBox/log.h>

#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/list.h>
#include <iprt/path.h>
#include <iprt/rand.h>
#include <iprt/semaphore.h>

#include <VBox/err.h>
#include <VBox/HostServices/VBoxClipboardSvc.h>
#include <VBox/GuestHost/SharedClipboard-transfers.h>


static int shClTransferThreadCreate(PSHCLTRANSFER pTransfer, PFNRTTHREAD pfnThreadFunc, void *pvUser);
static int shClTransferThreadDestroy(PSHCLTRANSFER pTransfer, RTMSINTERVAL uTimeoutMs);

static void shclTransferCtxTransferRemoveAndUnregister(PSHCLTRANSFERCTX pTransferCtx, PSHCLTRANSFER pTransfer);
static PSHCLTRANSFER shClTransferCtxGetTransferByIdInternal(PSHCLTRANSFERCTX pTransferCtx, uint32_t uId);
static PSHCLTRANSFER shClTransferCtxGetTransferByIndexInternal(PSHCLTRANSFERCTX pTransferCtx, uint32_t uIdx);
static int shClConvertFileCreateFlags(uint32_t fShClFlags, uint64_t *pfOpen);
static int shClTransferResolvePathAbs(PSHCLTRANSFER pTransfer, const char *pszPath, uint32_t fFlags, char **ppszResolved);

/** @todo Split this file up in different modules. */

/**
 * Allocates a new transfer root list.
 *
 * @returns Allocated transfer root list on success, or NULL on failure.
 */
PSHCLROOTLIST ShClTransferRootListAlloc(void)
{
    PSHCLROOTLIST pRootList = (PSHCLROOTLIST)RTMemAllocZ(sizeof(SHCLROOTLIST));

    return pRootList;
}

/**
 * Frees a transfer root list.
 *
 * @param   pRootList           transfer root list to free. The pointer will be
 *                              invalid after returning from this function.
 */
void ShClTransferRootListFree(PSHCLROOTLIST pRootList)
{
    if (!pRootList)
        return;

    for (uint32_t i = 0; i < pRootList->Hdr.cRoots; i++)
        ShClTransferListEntryInit(&pRootList->paEntries[i]);

    RTMemFree(pRootList);
    pRootList = NULL;
}

/**
 * Initializes a transfer root list header.
 *
 * @returns VBox status code.
 * @param   pRootLstHdr         Root list header to initialize.
 */
int ShClTransferRootListHdrInit(PSHCLROOTLISTHDR pRootLstHdr)
{
    AssertPtrReturn(pRootLstHdr, VERR_INVALID_POINTER);

    RT_BZERO(pRootLstHdr, sizeof(SHCLROOTLISTHDR));

    return VINF_SUCCESS;
}

/**
 * Destroys a transfer root list header.
 *
 * @param   pRootLstHdr         Root list header to destroy.
 */
void ShClTransferRootListHdrDestroy(PSHCLROOTLISTHDR pRootLstHdr)
{
    if (!pRootLstHdr)
        return;

    pRootLstHdr->fRoots = 0;
    pRootLstHdr->cRoots = 0;
}

/**
 * Duplicates a transfer list header.
 *
 * @returns Duplicated transfer list header on success, or NULL on failure.
 * @param   pRootLstHdr         Root list header to duplicate.
 */
PSHCLROOTLISTHDR ShClTransferRootListHdrDup(PSHCLROOTLISTHDR pRootLstHdr)
{
    AssertPtrReturn(pRootLstHdr, NULL);

    int rc = VINF_SUCCESS;

    PSHCLROOTLISTHDR pRootsDup = (PSHCLROOTLISTHDR)RTMemAllocZ(sizeof(SHCLROOTLISTHDR));
    if (pRootsDup)
    {
        *pRootsDup = *pRootLstHdr;
    }
    else
        rc = VERR_NO_MEMORY;

    if (RT_FAILURE(rc))
    {
        ShClTransferRootListHdrDestroy(pRootsDup);
        pRootsDup = NULL;
    }

    return pRootsDup;
}

/**
 * (Deep) Copies a clipboard root list entry structure.
 *
 * @returns VBox status code.
 * @param   pDst                Where to copy the source root list entry to.
 * @param   pSrc                Source root list entry to copy.
 */
int ShClTransferRootListEntryCopy(PSHCLROOTLISTENTRY pDst, PSHCLROOTLISTENTRY pSrc)
{
    return ShClTransferListEntryCopy(pDst, pSrc);
}

/**
 * Initializes a clipboard root list entry structure.
 *
 * @param   pRootListEntry      Clipboard root list entry structure to destroy.
 */
int ShClTransferRootListEntryInit(PSHCLROOTLISTENTRY pRootListEntry)
{
    return ShClTransferListEntryInit(pRootListEntry);
}

/**
 * Destroys a clipboard root list entry structure.
 *
 * @param   pRootListEntry      Clipboard root list entry structure to destroy.
 */
void ShClTransferRootListEntryDestroy(PSHCLROOTLISTENTRY pRootListEntry)
{
    return ShClTransferListEntryDestroy(pRootListEntry);
}

/**
 * Duplicates (allocates) a clipboard root list entry structure.
 *
 * @returns Duplicated clipboard root list entry structure on success.
 * @param   pRootListEntry      Clipboard root list entry to duplicate.
 */
PSHCLROOTLISTENTRY ShClTransferRootListEntryDup(PSHCLROOTLISTENTRY pRootListEntry)
{
    return ShClTransferListEntryDup(pRootListEntry);
}

/**
 * Initializes an list handle info structure.
 *
 * @returns VBox status code.
 * @param   pInfo               List handle info structure to initialize.
 */
int ShClTransferListHandleInfoInit(PSHCLLISTHANDLEINFO pInfo)
{
    AssertPtrReturn(pInfo, VERR_INVALID_POINTER);

    pInfo->hList   = SHCLLISTHANDLE_INVALID;
    pInfo->enmType = SHCLOBJTYPE_INVALID;

    pInfo->pszPathLocalAbs = NULL;

    RT_ZERO(pInfo->u);

    return VINF_SUCCESS;
}

/**
 * Destroys a list handle info structure.
 *
 * @param   pInfo               List handle info structure to destroy.
 */
void ShClTransferListHandleInfoDestroy(PSHCLLISTHANDLEINFO pInfo)
{
    if (!pInfo)
        return;

    if (pInfo->pszPathLocalAbs)
    {
        RTStrFree(pInfo->pszPathLocalAbs);
        pInfo->pszPathLocalAbs = NULL;
    }
}

/**
 * Allocates a transfer list header structure.
 *
 * @returns VBox status code.
 * @param   ppListHdr           Where to store the allocated transfer list header structure on success.
 */
int ShClTransferListHdrAlloc(PSHCLLISTHDR *ppListHdr)
{
    int rc;

    PSHCLLISTHDR pListHdr = (PSHCLLISTHDR)RTMemAllocZ(sizeof(SHCLLISTHDR));
    if (pListHdr)
    {
        *ppListHdr = pListHdr;
        rc = VINF_SUCCESS;
    }
    else
        rc = VERR_NO_MEMORY;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Frees a transfer list header structure.
 *
 * @param   pListEntry          Transfer list header structure to free.
 */
void ShClTransferListHdrFree(PSHCLLISTHDR pListHdr)
{
    if (!pListHdr)
        return;

    LogFlowFuncEnter();

    ShClTransferListHdrDestroy(pListHdr);

    RTMemFree(pListHdr);
    pListHdr = NULL;
}

/**
 * Duplicates (allocates) a transfer list header structure.
 *
 * @returns Duplicated transfer list header structure on success.
 * @param   pListHdr            Transfer list header to duplicate.
 */
PSHCLLISTHDR ShClTransferListHdrDup(PSHCLLISTHDR pListHdr)
{
    AssertPtrReturn(pListHdr, NULL);

    PSHCLLISTHDR pListHdrDup = (PSHCLLISTHDR)RTMemAlloc(sizeof(SHCLLISTHDR));
    if (pListHdrDup)
    {
        *pListHdrDup = *pListHdr;
    }

    return pListHdrDup;
}

/**
 * Initializes a transfer list header structure.
 *
 * @returns VBox status code.
 * @param   pListHdr            Transfer list header struct to initialize.
 */
int ShClTransferListHdrInit(PSHCLLISTHDR pListHdr)
{
    AssertPtrReturn(pListHdr, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    ShClTransferListHdrReset(pListHdr);

    return VINF_SUCCESS;
}

/**
 * Destroys a transfer list header structure.
 *
 * @param   pListHdr            Transfer list header struct to destroy.
 */
void ShClTransferListHdrDestroy(PSHCLLISTHDR pListHdr)
{
    if (!pListHdr)
        return;

    LogFlowFuncEnter();
}

/**
 * Resets a transfer list header structure.
 *
 * @returns VBox status code.
 * @param   pListHdr            Transfer list header struct to reset.
 */
void ShClTransferListHdrReset(PSHCLLISTHDR pListHdr)
{
    AssertPtrReturnVoid(pListHdr);

    LogFlowFuncEnter();

    RT_BZERO(pListHdr, sizeof(SHCLLISTHDR));
}

/**
 * Returns whether a given transfer list header is valid or not.
 *
 * @returns \c true if valid, \c false if not.
 * @param   pListHdr            Transfer list header to validate.
 */
bool ShClTransferListHdrIsValid(PSHCLLISTHDR pListHdr)
{
    RT_NOREF(pListHdr);
    return true; /** @todo Implement this. */
}

int ShClTransferListOpenParmsCopy(PSHCLLISTOPENPARMS pDst, PSHCLLISTOPENPARMS pSrc)
{
    AssertPtrReturn(pDst, VERR_INVALID_POINTER);
    AssertPtrReturn(pSrc, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    if (pSrc->pszFilter)
    {
        pDst->pszFilter = RTStrDup(pSrc->pszFilter);
        if (!pDst->pszFilter)
            rc = VERR_NO_MEMORY;
    }

    if (   RT_SUCCESS(rc)
        && pSrc->pszPath)
    {
        pDst->pszPath = RTStrDup(pSrc->pszPath);
        if (!pDst->pszPath)
            rc = VERR_NO_MEMORY;
    }

    if (RT_SUCCESS(rc))
    {
        pDst->fList    = pDst->fList;
        pDst->cbFilter = pSrc->cbFilter;
        pDst->cbPath   = pSrc->cbPath;
    }

    return rc;
}

/**
 * Duplicates a transfer list open parameters structure.
 *
 * @returns Duplicated transfer list open parameters structure on success, or NULL on failure.
 * @param   pParms              Transfer list open parameters structure to duplicate.
 */
PSHCLLISTOPENPARMS ShClTransferListOpenParmsDup(PSHCLLISTOPENPARMS pParms)
{
    AssertPtrReturn(pParms, NULL);

    PSHCLLISTOPENPARMS pParmsDup = (PSHCLLISTOPENPARMS)RTMemAllocZ(sizeof(SHCLLISTOPENPARMS));
    if (!pParmsDup)
        return NULL;

    int rc = ShClTransferListOpenParmsCopy(pParmsDup, pParms);
    if (RT_FAILURE(rc))
    {
        ShClTransferListOpenParmsDestroy(pParmsDup);

        RTMemFree(pParmsDup);
        pParmsDup = NULL;
    }

    return pParmsDup;
}

/**
 * Initializes a transfer list open parameters structure.
 *
 * @returns VBox status code.
 * @param   pParms              Transfer list open parameters structure to initialize.
 */
int ShClTransferListOpenParmsInit(PSHCLLISTOPENPARMS pParms)
{
    AssertPtrReturn(pParms, VERR_INVALID_POINTER);

    RT_BZERO(pParms, sizeof(SHCLLISTOPENPARMS));

    pParms->cbFilter  = SHCL_TRANSFER_PATH_MAX; /** @todo Make this dynamic. */
    pParms->pszFilter = RTStrAlloc(pParms->cbFilter);

    pParms->cbPath    = SHCL_TRANSFER_PATH_MAX; /** @todo Make this dynamic. */
    pParms->pszPath   = RTStrAlloc(pParms->cbPath);

    LogFlowFuncLeave();
    return VINF_SUCCESS;
}

/**
 * Destroys a transfer list open parameters structure.
 *
 * @param   pParms              Transfer list open parameters structure to destroy.
 */
void ShClTransferListOpenParmsDestroy(PSHCLLISTOPENPARMS pParms)
{
    if (!pParms)
        return;

    if (pParms->pszFilter)
    {
        RTStrFree(pParms->pszFilter);
        pParms->pszFilter = NULL;
    }

    if (pParms->pszPath)
    {
        RTStrFree(pParms->pszPath);
        pParms->pszPath = NULL;
    }
}

/**
 * Creates (allocates) and initializes a clipboard list entry structure.
 *
 * @param   ppDirData           Where to return the created clipboard list entry structure on success.
 */
int ShClTransferListEntryAlloc(PSHCLLISTENTRY *ppListEntry)
{
    PSHCLLISTENTRY pListEntry = (PSHCLLISTENTRY)RTMemAlloc(sizeof(SHCLLISTENTRY));
    if (!pListEntry)
        return VERR_NO_MEMORY;

    int rc = ShClTransferListEntryInit(pListEntry);
    if (RT_SUCCESS(rc))
        *ppListEntry = pListEntry;

    return rc;
}

/**
 * Frees a clipboard list entry structure.
 *
 * @param   pListEntry         Clipboard list entry structure to free.
 */
void ShClTransferListEntryFree(PSHCLLISTENTRY pListEntry)
{
    if (!pListEntry)
        return;

    ShClTransferListEntryDestroy(pListEntry);
    RTMemFree(pListEntry);
}

/**
 * (Deep) Copies a clipboard list entry structure.
 *
 * @returns VBox status code.
 * @param   pListEntry          Clipboard list entry to copy.
 */
int ShClTransferListEntryCopy(PSHCLLISTENTRY pDst, PSHCLLISTENTRY pSrc)
{
    AssertPtrReturn(pDst, VERR_INVALID_POINTER);
    AssertPtrReturn(pSrc, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    *pDst = *pSrc;

    if (pSrc->pszName)
    {
        pDst->pszName = RTStrDup(pSrc->pszName);
        if (!pDst->pszName)
            rc = VERR_NO_MEMORY;
    }

    if (   RT_SUCCESS(rc)
        && pSrc->pvInfo)
    {
        pDst->pvInfo = RTMemDup(pSrc->pvInfo, pSrc->cbInfo);
        if (pDst->pvInfo)
        {
            pDst->cbInfo = pSrc->cbInfo;
        }
        else
            rc = VERR_NO_MEMORY;
    }

    if (RT_FAILURE(rc))
    {
        if (pDst->pvInfo)
        {
            RTMemFree(pDst->pvInfo);
            pDst->pvInfo = NULL;
            pDst->cbInfo = 0;
        }
    }

    return rc;
}

/**
 * Duplicates (allocates) a clipboard list entry structure.
 *
 * @returns Duplicated clipboard list entry structure on success.
 * @param   pListEntry          Clipboard list entry to duplicate.
 */
PSHCLLISTENTRY ShClTransferListEntryDup(PSHCLLISTENTRY pListEntry)
{
    AssertPtrReturn(pListEntry, NULL);

    int rc = VINF_SUCCESS;

    PSHCLLISTENTRY pListEntryDup = (PSHCLLISTENTRY)RTMemAllocZ(sizeof(SHCLLISTENTRY));
    if (pListEntryDup)
        rc = ShClTransferListEntryCopy(pListEntryDup, pListEntry);

    if (RT_FAILURE(rc))
    {
        ShClTransferListEntryDestroy(pListEntryDup);

        RTMemFree(pListEntryDup);
        pListEntryDup = NULL;
    }

    return pListEntryDup;
}

/**
 * Initializes a clipboard list entry structure.
 *
 * @returns VBox status code.
 * @param   pListEntry          Clipboard list entry structure to initialize.
 */
int ShClTransferListEntryInit(PSHCLLISTENTRY pListEntry)
{
    AssertPtrReturn(pListEntry, VERR_INVALID_POINTER);

    RT_BZERO(pListEntry, sizeof(SHCLLISTENTRY));

    pListEntry->pszName = RTStrAlloc(SHCLLISTENTRY_MAX_NAME);
    if (!pListEntry->pszName)
        return VERR_NO_MEMORY;

    pListEntry->cbName = SHCLLISTENTRY_MAX_NAME;

    pListEntry->pvInfo = (PSHCLFSOBJINFO)RTMemAlloc(sizeof(SHCLFSOBJINFO));
    if (pListEntry->pvInfo)
    {
        pListEntry->cbInfo = sizeof(SHCLFSOBJINFO);
        pListEntry->fInfo  = VBOX_SHCL_INFO_FLAG_FSOBJINFO;

        return VINF_SUCCESS;
    }

    return VERR_NO_MEMORY;
}

/**
 * Destroys a clipboard list entry structure.
 *
 * @param   pListEntry          Clipboard list entry structure to destroy.
 */
void ShClTransferListEntryDestroy(PSHCLLISTENTRY pListEntry)
{
    if (!pListEntry)
        return;

    if (pListEntry->pszName)
    {
        RTStrFree(pListEntry->pszName);

        pListEntry->pszName = NULL;
        pListEntry->cbName  = 0;
    }

    if (pListEntry->pvInfo)
    {
        RTMemFree(pListEntry->pvInfo);
        pListEntry->pvInfo = NULL;
        pListEntry->cbInfo = 0;
    }
}

/**
 * Returns whether a given clipboard list entry is valid or not.
 *
 * @returns \c true if valid, \c false if not.
 * @param   pListEntry          Clipboard list entry to validate.
 */
bool ShClTransferListEntryIsValid(PSHCLLISTENTRY pListEntry)
{
    AssertPtrReturn(pListEntry, false);

    if (   !pListEntry->pszName
        || !pListEntry->cbName
        || strlen(pListEntry->pszName) == 0
        || strlen(pListEntry->pszName) > pListEntry->cbName /* Includes zero termination */ - 1)
    {
        return false;
    }

    if (pListEntry->cbInfo) /* cbInfo / pvInfo is optional. */
    {
        if (!pListEntry->pvInfo)
            return false;
    }

    return true;
}

/**
 * Initializes a transfer object context.
 *
 * @returns VBox status code.
 * @param   pObjCtx             transfer object context to initialize.
 */
int ShClTransferObjCtxInit(PSHCLCLIENTTRANSFEROBJCTX pObjCtx)
{
    AssertPtrReturn(pObjCtx, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    pObjCtx->uHandle  = SHCLOBJHANDLE_INVALID;

    return VINF_SUCCESS;
}

/**
 * Destroys a transfer object context.
 *
 * @param   pObjCtx             transfer object context to destroy.
 */
void ShClTransferObjCtxDestroy(PSHCLCLIENTTRANSFEROBJCTX pObjCtx)
{
    AssertPtrReturnVoid(pObjCtx);

    LogFlowFuncEnter();
}

/**
 * Returns if a transfer object context is valid or not.
 *
 * @returns \c true if valid, \c false if not.
 * @param   pObjCtx             transfer object context to check.
 */
bool ShClTransferObjCtxIsValid(PSHCLCLIENTTRANSFEROBJCTX pObjCtx)
{
    return (   pObjCtx
            && pObjCtx->uHandle != SHCLOBJHANDLE_INVALID);
}

/**
 * Initializes an object handle info structure.
 *
 * @returns VBox status code.
 * @param   pInfo               Object handle info structure to initialize.
 */
int ShClTransferObjHandleInfoInit(PSHCLOBJHANDLEINFO pInfo)
{
    AssertPtrReturn(pInfo, VERR_INVALID_POINTER);

    pInfo->hObj    = SHCLOBJHANDLE_INVALID;
    pInfo->enmType = SHCLOBJTYPE_INVALID;

    pInfo->pszPathLocalAbs = NULL;

    RT_ZERO(pInfo->u);

    return VINF_SUCCESS;
}

/**
 * Destroys an object handle info structure.
 *
 * @param   pInfo               Object handle info structure to destroy.
 */
void ShClTransferObjHandleInfoDestroy(PSHCLOBJHANDLEINFO pInfo)
{
    if (!pInfo)
        return;

    if (pInfo->pszPathLocalAbs)
    {
        RTStrFree(pInfo->pszPathLocalAbs);
        pInfo->pszPathLocalAbs = NULL;
    }
}

/**
 * Initializes a transfer object open parameters structure.
 *
 * @returns VBox status code.
 * @param   pParms              transfer object open parameters structure to initialize.
 */
int ShClTransferObjOpenParmsInit(PSHCLOBJOPENCREATEPARMS pParms)
{
    AssertPtrReturn(pParms, VERR_INVALID_POINTER);

    int rc;

    RT_BZERO(pParms, sizeof(SHCLOBJOPENCREATEPARMS));

    pParms->cbPath    = RTPATH_MAX; /** @todo Make this dynamic. */
    pParms->pszPath   = RTStrAlloc(pParms->cbPath);
    if (pParms->pszPath)
    {
        rc = VINF_SUCCESS;
    }
    else
        rc = VERR_NO_MEMORY;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Copies a transfer object open parameters structure from source to destination.
 *
 * @returns VBox status code.
 * @param   pParmsDst           Where to copy the source transfer object open parameters to.
 * @param   pParmsSrc           Which source transfer object open parameters to copy.
 */
int ShClTransferObjOpenParmsCopy(PSHCLOBJOPENCREATEPARMS pParmsDst, PSHCLOBJOPENCREATEPARMS pParmsSrc)
{
    int rc;

    *pParmsDst = *pParmsSrc;

    if (pParmsSrc->pszPath)
    {
        Assert(pParmsSrc->cbPath);
        pParmsDst->pszPath = RTStrDup(pParmsSrc->pszPath);
        if (pParmsDst->pszPath)
        {
            rc = VINF_SUCCESS;
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else
        rc = VINF_SUCCESS;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Destroys a transfer object open parameters structure.
 *
 * @param   pParms              transfer object open parameters structure to destroy.
 */
void ShClTransferObjOpenParmsDestroy(PSHCLOBJOPENCREATEPARMS pParms)
{
    if (!pParms)
        return;

    if (pParms->pszPath)
    {
        RTStrFree(pParms->pszPath);
        pParms->pszPath = NULL;
    }
}

/**
 * Returns a specific object handle info of a transfer.
 *
 * @returns Pointer to object handle info if found, or NULL if not found.
 * @param   pTransfer           Clipboard transfer to get object handle info from.
 * @param   hObj                Object handle of the object to get handle info for.
 */
DECLINLINE(PSHCLOBJHANDLEINFO) shClTransferObjGet(PSHCLTRANSFER pTransfer, SHCLOBJHANDLE hObj)
{
    PSHCLOBJHANDLEINFO pIt;
    RTListForEach(&pTransfer->lstObj, pIt, SHCLOBJHANDLEINFO, Node) /** @todo Slooow ...but works for now. */
    {
        if (pIt->hObj == hObj)
            return pIt;
    }

    return NULL;
}

/**
 * Opens a transfer object.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer to open the object for.
 * @param   pOpenCreateParms    Open / create parameters of transfer object to open / create.
 * @param   phObj               Where to store the handle of transfer object opened on success.
 */
int ShClTransferObjOpen(PSHCLTRANSFER pTransfer, PSHCLOBJOPENCREATEPARMS pOpenCreateParms, PSHCLOBJHANDLE phObj)
{
    AssertPtrReturn(pTransfer,        VERR_INVALID_POINTER);
    AssertPtrReturn(pOpenCreateParms, VERR_INVALID_POINTER);
    AssertPtrReturn(phObj,            VERR_INVALID_POINTER);
    AssertMsgReturn(pTransfer->pszPathRootAbs, ("Transfer has no root path set\n"), VERR_INVALID_PARAMETER);
    AssertMsgReturn(pOpenCreateParms->pszPath, ("No path in open/create params set\n"), VERR_INVALID_PARAMETER);

    if (pTransfer->cObjHandles >= pTransfer->cMaxObjHandles)
        return VERR_SHCLPB_MAX_OBJECTS_REACHED;

    LogFlowFunc(("pszPath=%s, fCreate=0x%x\n", pOpenCreateParms->pszPath, pOpenCreateParms->fCreate));

    int rc;
    if (pTransfer->State.enmSource == SHCLSOURCE_LOCAL)
    {
        PSHCLOBJHANDLEINFO pInfo = (PSHCLOBJHANDLEINFO)RTMemAllocZ(sizeof(SHCLOBJHANDLEINFO));
        if (pInfo)
        {
            rc = ShClTransferObjHandleInfoInit(pInfo);
            if (RT_SUCCESS(rc))
            {
                uint64_t fOpen;
                rc = shClConvertFileCreateFlags(pOpenCreateParms->fCreate, &fOpen);
                if (RT_SUCCESS(rc))
                {
                    rc = shClTransferResolvePathAbs(pTransfer, pOpenCreateParms->pszPath, 0 /* fFlags */,
                                                    &pInfo->pszPathLocalAbs);
                    if (RT_SUCCESS(rc))
                    {
                        rc = RTFileOpen(&pInfo->u.Local.hFile, pInfo->pszPathLocalAbs, fOpen);
                        if (RT_SUCCESS(rc))
                            LogRel2(("Shared Clipboard: Opened file '%s'\n", pInfo->pszPathLocalAbs));
                        else
                            LogRel(("Shared Clipboard: Error opening file '%s': rc=%Rrc\n", pInfo->pszPathLocalAbs, rc));
                    }
                }
            }

            if (RT_SUCCESS(rc))
            {
                pInfo->hObj    = pTransfer->uObjHandleNext++;
                pInfo->enmType = SHCLOBJTYPE_FILE;

                RTListAppend(&pTransfer->lstObj, &pInfo->Node);
                pTransfer->cObjHandles++;

                LogFlowFunc(("cObjHandles=%RU32\n", pTransfer->cObjHandles));

                *phObj = pInfo->hObj;
            }
            else
            {
                ShClTransferObjHandleInfoDestroy(pInfo);
                RTMemFree(pInfo);
            }
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else if (pTransfer->State.enmSource == SHCLSOURCE_REMOTE)
    {
        if (pTransfer->ProviderIface.pfnObjOpen)
            rc = pTransfer->ProviderIface.pfnObjOpen(&pTransfer->ProviderCtx, pOpenCreateParms, phObj);
        else
            rc = VERR_NOT_SUPPORTED;
    }
    else
        AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Closes a transfer object.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer that contains the object to close.
 * @param   hObj                Handle of transfer object to close.
 */
int ShClTransferObjClose(PSHCLTRANSFER pTransfer, SHCLOBJHANDLE hObj)
{
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    if (pTransfer->State.enmSource == SHCLSOURCE_LOCAL)
    {
        PSHCLOBJHANDLEINFO pInfo = shClTransferObjGet(pTransfer, hObj);
        if (pInfo)
        {
            switch (pInfo->enmType)
            {
                case SHCLOBJTYPE_DIRECTORY:
                {
                    rc = RTDirClose(pInfo->u.Local.hDir);
                    if (RT_SUCCESS(rc))
                    {
                        pInfo->u.Local.hDir = NIL_RTDIR;

                        LogRel2(("Shared Clipboard: Closed directory '%s'\n", pInfo->pszPathLocalAbs));
                    }
                    else
                        LogRel(("Shared Clipboard: Closing directory '%s' failed with %Rrc\n", pInfo->pszPathLocalAbs, rc));
                    break;
                }

                case SHCLOBJTYPE_FILE:
                {
                    rc = RTFileClose(pInfo->u.Local.hFile);
                    if (RT_SUCCESS(rc))
                    {
                        pInfo->u.Local.hFile = NIL_RTFILE;

                        LogRel2(("Shared Clipboard: Closed file '%s'\n", pInfo->pszPathLocalAbs));
                    }
                    else
                        LogRel(("Shared Clipboard: Closing file '%s' failed with %Rrc\n", pInfo->pszPathLocalAbs, rc));
                    break;
                }

                default:
                    rc = VERR_NOT_IMPLEMENTED;
                    break;
            }

            RTListNodeRemove(&pInfo->Node);

            Assert(pTransfer->cObjHandles);
            pTransfer->cObjHandles--;

            ShClTransferObjHandleInfoDestroy(pInfo);

            RTMemFree(pInfo);
            pInfo = NULL;
        }
        else
            rc = VERR_NOT_FOUND;
    }
    else if (pTransfer->State.enmSource == SHCLSOURCE_REMOTE)
    {
        if (pTransfer->ProviderIface.pfnObjClose)
        {
            rc = pTransfer->ProviderIface.pfnObjClose(&pTransfer->ProviderCtx, hObj);
        }
        else
            rc = VERR_NOT_SUPPORTED;
    }
    else
        AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Reads from a transfer object.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer that contains the object to read from.
 * @param   hObj                Handle of transfer object to read from.
 * @param   pvBuf               Buffer for where to store the read data.
 * @param   cbBuf               Size (in bytes) of buffer.
 * @param   fFlags              Read flags. Optional.
 * @param   pcbRead             Where to return how much bytes were read on success. Optional.
 */
int ShClTransferObjRead(PSHCLTRANSFER pTransfer,
                        SHCLOBJHANDLE hObj, void *pvBuf, uint32_t cbBuf, uint32_t fFlags, uint32_t *pcbRead)
{
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf,     VERR_INVALID_POINTER);
    AssertReturn   (cbBuf,     VERR_INVALID_PARAMETER);
    /* pcbRead is optional. */
    /** @todo Validate fFlags. */

    int rc = VINF_SUCCESS;

    if (pTransfer->State.enmSource == SHCLSOURCE_LOCAL)
    {
        PSHCLOBJHANDLEINFO pInfo = shClTransferObjGet(pTransfer, hObj);
        if (pInfo)
        {
            switch (pInfo->enmType)
            {
                case SHCLOBJTYPE_FILE:
                {
                    size_t cbRead;
                    rc = RTFileRead(pInfo->u.Local.hFile, pvBuf, cbBuf, &cbRead);
                    if (RT_SUCCESS(rc))
                    {
                        if (pcbRead)
                            *pcbRead = (uint32_t)cbRead;
                    }
                    break;
                }

                default:
                    rc = VERR_NOT_SUPPORTED;
                    break;
            }
        }
        else
            rc = VERR_NOT_FOUND;
    }
    else if (pTransfer->State.enmSource == SHCLSOURCE_REMOTE)
    {
        if (pTransfer->ProviderIface.pfnObjRead)
        {
            rc = pTransfer->ProviderIface.pfnObjRead(&pTransfer->ProviderCtx, hObj, pvBuf, cbBuf, fFlags, pcbRead);
        }
        else
            rc = VERR_NOT_SUPPORTED;
    }
    else
        AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Writes to a transfer object.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer that contains the object to write to.
 * @param   hObj                Handle of transfer object to write to.
 * @param   pvBuf               Buffer of data to write.
 * @param   cbBuf               Size (in bytes) of buffer to write.
 * @param   fFlags              Write flags. Optional.
 * @param   pcbWritten          How much bytes were writtenon success. Optional.
 */
int ShClTransferObjWrite(PSHCLTRANSFER pTransfer,
                         SHCLOBJHANDLE hObj, void *pvBuf, uint32_t cbBuf, uint32_t fFlags, uint32_t *pcbWritten)
{
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf,     VERR_INVALID_POINTER);
    AssertReturn   (cbBuf,     VERR_INVALID_PARAMETER);
    /* pcbWritten is optional. */

    int rc = VINF_SUCCESS;

    if (pTransfer->State.enmSource == SHCLSOURCE_LOCAL)
    {
        PSHCLOBJHANDLEINFO pInfo = shClTransferObjGet(pTransfer, hObj);
        if (pInfo)
        {
            switch (pInfo->enmType)
            {
                case SHCLOBJTYPE_FILE:
                {
                    rc = RTFileWrite(pInfo->u.Local.hFile, pvBuf, cbBuf, (size_t *)pcbWritten);
                    break;
                }

                default:
                    rc = VERR_NOT_SUPPORTED;
                    break;
            }
        }
        else
            rc = VERR_NOT_FOUND;
    }
    else if (pTransfer->State.enmSource == SHCLSOURCE_REMOTE)
    {
        if (pTransfer->ProviderIface.pfnObjWrite)
        {
            rc = pTransfer->ProviderIface.pfnObjWrite(&pTransfer->ProviderCtx, hObj, pvBuf, cbBuf, fFlags, pcbWritten);
        }
        else
            rc = VERR_NOT_SUPPORTED;
    }
    else
        AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Duplicates a transfer object data chunk.
 *
 * @returns Duplicated object data chunk on success, or NULL on failure.
 * @param   pDataChunk          transfer object data chunk to duplicate.
 */
PSHCLOBJDATACHUNK ShClTransferObjDataChunkDup(PSHCLOBJDATACHUNK pDataChunk)
{
    if (!pDataChunk)
        return NULL;

    PSHCLOBJDATACHUNK pDataChunkDup = (PSHCLOBJDATACHUNK)RTMemAllocZ(sizeof(SHCLOBJDATACHUNK));
    if (!pDataChunkDup)
        return NULL;

    if (pDataChunk->pvData)
    {
        Assert(pDataChunk->cbData);

        pDataChunkDup->uHandle = pDataChunk->uHandle;
        pDataChunkDup->pvData  = RTMemDup(pDataChunk->pvData, pDataChunk->cbData);
        pDataChunkDup->cbData  = pDataChunk->cbData;
    }

    return pDataChunkDup;
}

/**
 * Destroys a transfer object data chunk.
 *
 * @param   pDataChunk          transfer object data chunk to destroy.
 */
void ShClTransferObjDataChunkDestroy(PSHCLOBJDATACHUNK pDataChunk)
{
    if (!pDataChunk)
        return;

    if (pDataChunk->pvData)
    {
        Assert(pDataChunk->cbData);

        RTMemFree(pDataChunk->pvData);

        pDataChunk->pvData = NULL;
        pDataChunk->cbData = 0;
    }

    pDataChunk->uHandle = 0;
}

/**
 * Frees a transfer object data chunk.
 *
 * @param   pDataChunk          transfer object data chunk to free. The handed-in pointer will
 *                              be invalid after calling this function.
 */
void ShClTransferObjDataChunkFree(PSHCLOBJDATACHUNK pDataChunk)
{
    if (!pDataChunk)
        return;

    ShClTransferObjDataChunkDestroy(pDataChunk);

    RTMemFree(pDataChunk);
    pDataChunk = NULL;
}

/**
 * Creates a clipboard transfer.
 *
 * @returns VBox status code.
 * @param   ppTransfer          Where to return the created Shared Clipboard transfer struct.
 *                              Must be destroyed by ShClTransferDestroy().
 */
int ShClTransferCreate(PSHCLTRANSFER *ppTransfer)
{
    AssertPtrReturn(ppTransfer, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    PSHCLTRANSFER pTransfer = (PSHCLTRANSFER)RTMemAllocZ(sizeof(SHCLTRANSFER));
    AssertPtrReturn(pTransfer, VERR_NO_MEMORY);

    pTransfer->State.uID       = 0;
    pTransfer->State.enmStatus = SHCLTRANSFERSTATUS_NONE;
    pTransfer->State.enmDir    = SHCLTRANSFERDIR_UNKNOWN;
    pTransfer->State.enmSource = SHCLSOURCE_INVALID;

    pTransfer->Thread.hThread    = NIL_RTTHREAD;
    pTransfer->Thread.fCancelled = false;
    pTransfer->Thread.fStarted   = false;
    pTransfer->Thread.fStop      = false;

    pTransfer->pszPathRootAbs    = NULL;

#ifdef DEBUG_andy
    pTransfer->uTimeoutMs     = RT_MS_5SEC;
#else
    pTransfer->uTimeoutMs     = RT_MS_30SEC;
#endif
    pTransfer->cbMaxChunkSize  = _64K; /** @todo Make this configurable. */
    pTransfer->cMaxListHandles = _4K;  /** @todo Ditto. */
    pTransfer->cMaxObjHandles  = _4K;  /** @todo Ditto. */

    pTransfer->pvUser = NULL;
    pTransfer->cbUser = 0;

    RTListInit(&pTransfer->lstList);
    RTListInit(&pTransfer->lstObj);

    pTransfer->cRoots = 0;
    RTListInit(&pTransfer->lstRoots);

    int rc = ShClEventSourceCreate(&pTransfer->Events, 0 /* uID */);
    if (RT_SUCCESS(rc))
    {
        *ppTransfer = pTransfer;
    }
    else
    {
        if (pTransfer)
        {
            ShClTransferDestroy(pTransfer);
            RTMemFree(pTransfer);
        }
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Destroys a clipboard transfer context struct.
 *
 * @returns VBox status code.
 * @param   pTransferCtx                Clipboard transfer to destroy.
 */
int ShClTransferDestroy(PSHCLTRANSFER pTransfer)
{
    if (!pTransfer)
        return VINF_SUCCESS;

    LogFlowFuncEnter();

    int rc = shClTransferThreadDestroy(pTransfer, 30 * 1000 /* Timeout in ms */);
    if (RT_FAILURE(rc))
        return rc;

    ShClTransferReset(pTransfer);

    ShClEventSourceDestroy(&pTransfer->Events);

    LogFlowFuncLeave();
    return VINF_SUCCESS;
}

/**
 * Initializes a Shared Clipboard transfer object.
 *
 * @returns VBox status code.
 * @param   pTransfer           Transfer to initialize.
 * @param   enmDir              Specifies the transfer direction of this transfer.
 * @param   enmSource           Specifies the data source of the transfer.
 */
int ShClTransferInit(PSHCLTRANSFER pTransfer, SHCLTRANSFERDIR enmDir, SHCLSOURCE enmSource)
{
    pTransfer->State.enmDir    = enmDir;
    pTransfer->State.enmSource = enmSource;

    LogFlowFunc(("uID=%RU32, enmDir=%RU32, enmSource=%RU32\n",
                 pTransfer->State.uID, pTransfer->State.enmDir, pTransfer->State.enmSource));

    pTransfer->State.enmStatus = SHCLTRANSFERSTATUS_INITIALIZED; /* Now we're ready to run. */

    pTransfer->cListHandles    = 0;
    pTransfer->uListHandleNext = 1;

    pTransfer->cObjHandles     = 0;
    pTransfer->uObjHandleNext  = 1;

    int rc = VINF_SUCCESS;

    if (pTransfer->Callbacks.pfnOnInitialize)
        rc = pTransfer->Callbacks.pfnOnInitialize(&pTransfer->CallbackCtx);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Returns a specific list handle info of a transfer.
 *
 * @returns Pointer to list handle info if found, or NULL if not found.
 * @param   pTransfer           Clipboard transfer to get list handle info from.
 * @param   hList               List handle of the list to get handle info for.
 */
DECLINLINE(PSHCLLISTHANDLEINFO) shClTransferListGetByHandle(PSHCLTRANSFER pTransfer, SHCLLISTHANDLE hList)
{
    PSHCLLISTHANDLEINFO pIt;
    RTListForEach(&pTransfer->lstList, pIt, SHCLLISTHANDLEINFO, Node) /** @todo Sloooow ... improve this. */
    {
        if (pIt->hList == hList)
            return pIt;
    }

    return NULL;
}

/**
 * Creates a new list handle (local only).
 *
 * @returns New List handle on success, or SHCLLISTHANDLE_INVALID on error.
 * @param   pTransfer           Clipboard transfer to create new list handle for.
 */
DECLINLINE(SHCLLISTHANDLE) shClTransferListHandleNew(PSHCLTRANSFER pTransfer)
{
    return pTransfer->uListHandleNext++; /** @todo Good enough for now. Improve this later. */
}

/**
 * Validates whether a given path matches our set of rules or not.
 *
 * @returns VBox status code.
 * @param   pcszPath            Path to validate.
 * @param   fMustExist          Whether the path to validate also must exist.
 */
static int shClTransferValidatePath(const char *pcszPath, bool fMustExist)
{
    int rc = VINF_SUCCESS;

    if (!strlen(pcszPath))
        rc = VERR_INVALID_PARAMETER;

    if (   RT_SUCCESS(rc)
        && !RTStrIsValidEncoding(pcszPath))
    {
        rc = VERR_INVALID_UTF8_ENCODING;
    }

    if (   RT_SUCCESS(rc)
        && RTStrStr(pcszPath, ".."))
    {
        rc = VERR_INVALID_PARAMETER;
    }

    if (   RT_SUCCESS(rc)
        && fMustExist)
    {
        RTFSOBJINFO objInfo;
        rc = RTPathQueryInfo(pcszPath, &objInfo, RTFSOBJATTRADD_NOTHING);
        if (RT_SUCCESS(rc))
        {
            if (RTFS_IS_DIRECTORY(objInfo.Attr.fMode))
            {
                if (!RTDirExists(pcszPath)) /* Path must exist. */
                    rc = VERR_PATH_NOT_FOUND;
            }
            else if (RTFS_IS_FILE(objInfo.Attr.fMode))
            {
                if (!RTFileExists(pcszPath)) /* File must exist. */
                    rc = VERR_FILE_NOT_FOUND;
            }
            else /* Everything else (e.g. symbolic links) are not supported. */
            {
                LogRel2(("Shared Clipboard: Path '%s' contains a symbolic link or junktion, which are not supported\n", pcszPath));
                rc = VERR_NOT_SUPPORTED;
            }
        }
    }

    if (RT_FAILURE(rc))
        LogRel2(("Shared Clipboard: Validating path '%s' failed: %Rrc\n", pcszPath, rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Resolves a relative path of a specific transfer to its absolute path.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer to resolve path for.
 * @param   pszPath             Path to resolve.
 * @param   fFlags              Resolve flags. Currently not used and must be 0.
 * @param   ppszResolved        Where to store the allocated resolved path. Must be free'd by the called using RTStrFree().
 */
static int shClTransferResolvePathAbs(PSHCLTRANSFER pTransfer, const char *pszPath, uint32_t fFlags,
                                      char **ppszResolved)
{
    AssertPtrReturn(pTransfer,   VERR_INVALID_POINTER);
    AssertPtrReturn(pszPath,     VERR_INVALID_POINTER);
    AssertReturn   (fFlags == 0, VERR_INVALID_PARAMETER);

    LogFlowFunc(("pszPathRootAbs=%s, pszPath=%s\n", pTransfer->pszPathRootAbs, pszPath));

    int rc = shClTransferValidatePath(pszPath, false /* fMustExist */);
    if (RT_SUCCESS(rc))
    {
        char *pszPathAbs = RTPathJoinA(pTransfer->pszPathRootAbs, pszPath);
        if (pszPathAbs)
        {
            char   szResolved[RTPATH_MAX];
            size_t cbResolved = sizeof(szResolved);
            rc = RTPathAbsEx(pTransfer->pszPathRootAbs, pszPathAbs, RTPATH_STR_F_STYLE_HOST, szResolved, &cbResolved);

            RTStrFree(pszPathAbs);

            if (RT_SUCCESS(rc))
            {
                LogFlowFunc(("pszResolved=%s\n", szResolved));

                rc = VERR_PATH_NOT_FOUND; /* Play safe by default. */

                /* Make sure the resolved path is part of the set of root entries. */
                PSHCLLISTROOT pListRoot;
                RTListForEach(&pTransfer->lstRoots, pListRoot, SHCLLISTROOT, Node)
                {
                    if (RTPathStartsWith(szResolved, pListRoot->pszPathAbs))
                    {
                        rc = VINF_SUCCESS;
                        break;
                    }
                }

                if (RT_SUCCESS(rc))
                    *ppszResolved = RTStrDup(szResolved);
            }
        }
        else
            rc = VERR_NO_MEMORY;
    }

    if (RT_FAILURE(rc))
        LogRel(("Shared Clipboard: Resolving absolute path '%s' failed, rc=%Rrc\n", pszPath, rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Opens a list.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer to handle.
 * @param   pOpenParms          List open parameters to use for opening.
 * @param   phList              Where to store the List handle of opened list on success.
 */
int ShClTransferListOpen(PSHCLTRANSFER pTransfer, PSHCLLISTOPENPARMS pOpenParms,
                         PSHCLLISTHANDLE phList)
{
    AssertPtrReturn(pTransfer,  VERR_INVALID_POINTER);
    AssertPtrReturn(pOpenParms, VERR_INVALID_POINTER);
    AssertPtrReturn(phList,     VERR_INVALID_POINTER);

    int rc;

    if (pTransfer->cListHandles == pTransfer->cMaxListHandles)
        return VERR_SHCLPB_MAX_LISTS_REACHED;

    if (pTransfer->State.enmSource == SHCLSOURCE_LOCAL)
    {
        LogFlowFunc(("pszPath=%s\n", pOpenParms->pszPath));

        PSHCLLISTHANDLEINFO pInfo
            = (PSHCLLISTHANDLEINFO)RTMemAllocZ(sizeof(SHCLLISTHANDLEINFO));
        if (pInfo)
        {
            rc = ShClTransferListHandleInfoInit(pInfo);
            if (RT_SUCCESS(rc))
            {
                rc = shClTransferResolvePathAbs(pTransfer, pOpenParms->pszPath, 0 /* fFlags */, &pInfo->pszPathLocalAbs);
                if (RT_SUCCESS(rc))
                {
                    RTFSOBJINFO objInfo;
                    rc = RTPathQueryInfo(pInfo->pszPathLocalAbs, &objInfo, RTFSOBJATTRADD_NOTHING);
                    if (RT_SUCCESS(rc))
                    {
                        if (RTFS_IS_DIRECTORY(objInfo.Attr.fMode))
                        {
                            rc = RTDirOpen(&pInfo->u.Local.hDir, pInfo->pszPathLocalAbs);
                            if (RT_SUCCESS(rc))
                            {
                                pInfo->enmType = SHCLOBJTYPE_DIRECTORY;

                                LogRel2(("Shared Clipboard: Opening directory '%s'\n", pInfo->pszPathLocalAbs));
                            }
                            else
                                LogRel(("Shared Clipboard: Opening directory '%s' failed with %Rrc\n", pInfo->pszPathLocalAbs, rc));

                        }
                        else if (RTFS_IS_FILE(objInfo.Attr.fMode))
                        {
                            rc = RTFileOpen(&pInfo->u.Local.hFile, pInfo->pszPathLocalAbs,
                                            RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_WRITE);
                            if (RT_SUCCESS(rc))
                            {
                                pInfo->enmType = SHCLOBJTYPE_FILE;

                                LogRel2(("Shared Clipboard: Opening file '%s'\n", pInfo->pszPathLocalAbs));
                            }
                            else
                                LogRel(("Shared Clipboard: Opening file '%s' failed with %Rrc\n", pInfo->pszPathLocalAbs, rc));
                        }
                        else
                            rc = VERR_NOT_SUPPORTED;

                        if (RT_SUCCESS(rc))
                        {
                            pInfo->hList = shClTransferListHandleNew(pTransfer);

                            RTListAppend(&pTransfer->lstList, &pInfo->Node);
                            pTransfer->cListHandles++;

                            if (phList)
                                *phList = pInfo->hList;

                            LogFlowFunc(("pszPathLocalAbs=%s, hList=%RU64, cListHandles=%RU32\n",
                                         pInfo->pszPathLocalAbs, pInfo->hList, pTransfer->cListHandles));
                        }
                        else
                        {
                            if (RTFS_IS_DIRECTORY(objInfo.Attr.fMode))
                            {
                                if (RTDirIsValid(pInfo->u.Local.hDir))
                                    RTDirClose(pInfo->u.Local.hDir);
                            }
                            else if (RTFS_IS_FILE(objInfo.Attr.fMode))
                            {
                                if (RTFileIsValid(pInfo->u.Local.hFile))
                                    RTFileClose(pInfo->u.Local.hFile);
                            }
                        }
                    }
                }
            }

            if (RT_FAILURE(rc))
            {
                ShClTransferListHandleInfoDestroy(pInfo);

                RTMemFree(pInfo);
                pInfo = NULL;
            }
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else if (pTransfer->State.enmSource == SHCLSOURCE_REMOTE)
    {
        if (pTransfer->ProviderIface.pfnListOpen)
        {
            rc = pTransfer->ProviderIface.pfnListOpen(&pTransfer->ProviderCtx, pOpenParms, phList);
        }
        else
            rc = VERR_NOT_SUPPORTED;
    }
    else
        AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Closes a list.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer to handle.
 * @param   hList               Handle of list to close.
 */
int ShClTransferListClose(PSHCLTRANSFER pTransfer, SHCLLISTHANDLE hList)
{
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);

    if (hList == SHCLLISTHANDLE_INVALID)
        return VINF_SUCCESS;

    int rc = VINF_SUCCESS;

    if (pTransfer->State.enmSource == SHCLSOURCE_LOCAL)
    {
        PSHCLLISTHANDLEINFO pInfo = shClTransferListGetByHandle(pTransfer, hList);
        if (pInfo)
        {
            switch (pInfo->enmType)
            {
                case SHCLOBJTYPE_DIRECTORY:
                {
                    if (RTDirIsValid(pInfo->u.Local.hDir))
                    {
                        RTDirClose(pInfo->u.Local.hDir);
                        pInfo->u.Local.hDir = NIL_RTDIR;
                    }
                    break;
                }

                default:
                    rc = VERR_NOT_SUPPORTED;
                    break;
            }

            RTListNodeRemove(&pInfo->Node);

            Assert(pTransfer->cListHandles);
            pTransfer->cListHandles--;

            RTMemFree(pInfo);
        }
        else
            rc = VERR_NOT_FOUND;
    }
    else if (pTransfer->State.enmSource == SHCLSOURCE_REMOTE)
    {
        if (pTransfer->ProviderIface.pfnListClose)
        {
            rc = pTransfer->ProviderIface.pfnListClose(&pTransfer->ProviderCtx, hList);
        }
        else
            rc = VERR_NOT_SUPPORTED;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Adds a file to a list heaer.
 *
 * @returns VBox status code.
 * @param   pHdr                List header to add file to.
 * @param   pszPath             Path of file to add.
 */
static int shclTransferListHdrAddFile(PSHCLLISTHDR pHdr, const char *pszPath)
{
    uint64_t cbSize = 0;
    int rc = RTFileQuerySizeByPath(pszPath, &cbSize);
    if (RT_SUCCESS(rc))
    {
        pHdr->cbTotalSize  += cbSize;
        pHdr->cTotalObjects++;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Builds a list header, internal version.
 *
 * @returns VBox status code.
 * @param   pHdr                Where to store the build list header.
 * @param   pcszSrcPath         Source path of list.
 * @param   pcszDstPath         Destination path of list.
 * @param   pcszDstBase         Destination base path.
 * @param   cchDstBase          Number of charaters of destination base path.
 */
static int shclTransferListHdrFromDir(PSHCLLISTHDR pHdr, const char *pcszPathAbs)
{
    AssertPtrReturn(pcszPathAbs, VERR_INVALID_POINTER);

    LogFlowFunc(("pcszPathAbs=%s\n", pcszPathAbs));

    RTFSOBJINFO objInfo;
    int rc = RTPathQueryInfo(pcszPathAbs, &objInfo, RTFSOBJATTRADD_NOTHING);
    if (RT_SUCCESS(rc))
    {
        if (RTFS_IS_DIRECTORY(objInfo.Attr.fMode))
        {
            RTDIR hDir;
            rc = RTDirOpen(&hDir, pcszPathAbs);
            if (RT_SUCCESS(rc))
            {
                size_t        cbDirEntry = 0;
                PRTDIRENTRYEX pDirEntry  = NULL;
                do
                {
                    /* Retrieve the next directory entry. */
                    rc = RTDirReadExA(hDir, &pDirEntry, &cbDirEntry, RTFSOBJATTRADD_NOTHING, RTPATH_F_ON_LINK);
                    if (RT_FAILURE(rc))
                    {
                        if (rc == VERR_NO_MORE_FILES)
                            rc = VINF_SUCCESS;
                        break;
                    }

                    switch (pDirEntry->Info.Attr.fMode & RTFS_TYPE_MASK)
                    {
                        case RTFS_TYPE_DIRECTORY:
                        {
                            /* Skip "." and ".." entries. */
                            if (RTDirEntryExIsStdDotLink(pDirEntry))
                                break;

                            pHdr->cTotalObjects++;
                            break;
                        }
                        case RTFS_TYPE_FILE:
                        {
                            char *pszSrc = RTPathJoinA(pcszPathAbs, pDirEntry->szName);
                            if (pszSrc)
                            {
                                rc = shclTransferListHdrAddFile(pHdr, pszSrc);
                                RTStrFree(pszSrc);
                            }
                            else
                                rc = VERR_NO_MEMORY;
                            break;
                        }
                        case RTFS_TYPE_SYMLINK:
                        {
                            /** @todo Not implemented yet. */
                        }

                        default:
                            break;
                    }

                } while (RT_SUCCESS(rc));

                RTDirReadExAFree(&pDirEntry, &cbDirEntry);
                RTDirClose(hDir);
            }
        }
        else if (RTFS_IS_FILE(objInfo.Attr.fMode))
        {
            rc = shclTransferListHdrAddFile(pHdr, pcszPathAbs);
        }
        else if (RTFS_IS_SYMLINK(objInfo.Attr.fMode))
        {
            /** @todo Not implemented yet. */
        }
        else
            rc = VERR_NOT_SUPPORTED;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Retrieves the header of a Shared Clipboard list.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer to handle.
 * @param   hList               Handle of list to get header for.
 * @param   pHdr                Where to store the returned list header information.
 */
int ShClTransferListGetHeader(PSHCLTRANSFER pTransfer, SHCLLISTHANDLE hList,
                              PSHCLLISTHDR pHdr)
{
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);
    AssertPtrReturn(pHdr,      VERR_INVALID_POINTER);

    int rc;

    LogFlowFunc(("hList=%RU64\n", hList));

    if (pTransfer->State.enmSource == SHCLSOURCE_LOCAL)
    {
        PSHCLLISTHANDLEINFO pInfo = shClTransferListGetByHandle(pTransfer, hList);
        if (pInfo)
        {
            rc = ShClTransferListHdrInit(pHdr);
            if (RT_SUCCESS(rc))
            {
                switch (pInfo->enmType)
                {
                    case SHCLOBJTYPE_DIRECTORY:
                    {
                        LogFlowFunc(("DirAbs: %s\n", pInfo->pszPathLocalAbs));

                        rc = shclTransferListHdrFromDir(pHdr, pInfo->pszPathLocalAbs);
                        break;
                    }

                    case SHCLOBJTYPE_FILE:
                    {
                        LogFlowFunc(("FileAbs: %s\n", pInfo->pszPathLocalAbs));

                        pHdr->cTotalObjects = 1;

                        RTFSOBJINFO objInfo;
                        rc = RTFileQueryInfo(pInfo->u.Local.hFile, &objInfo, RTFSOBJATTRADD_NOTHING);
                        if (RT_SUCCESS(rc))
                        {
                            pHdr->cbTotalSize = objInfo.cbObject;
                        }
                        break;
                    }

                    default:
                        rc = VERR_NOT_SUPPORTED;
                        break;
                }
            }

            LogFlowFunc(("cTotalObj=%RU64, cbTotalSize=%RU64\n", pHdr->cTotalObjects, pHdr->cbTotalSize));
        }
        else
            rc = VERR_NOT_FOUND;
    }
    else if (pTransfer->State.enmSource == SHCLSOURCE_REMOTE)
    {
        if (pTransfer->ProviderIface.pfnListHdrRead)
        {
            rc = pTransfer->ProviderIface.pfnListHdrRead(&pTransfer->ProviderCtx, hList, pHdr);
        }
        else
            rc = VERR_NOT_SUPPORTED;
    }
    else
        AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Returns the current transfer object for a Shared Clipboard transfer list.
 *
 * Currently not implemented and wil return NULL.
 *
 * @returns Pointer to transfer object, or NULL if not found / invalid.
 * @param   pTransfer           Clipboard transfer to return transfer object for.
 * @param   hList               Handle of Shared Clipboard transfer list to get object for.
 * @param   uIdx                Index of object to get.
 */
PSHCLTRANSFEROBJ ShClTransferListGetObj(PSHCLTRANSFER pTransfer,
                                        SHCLLISTHANDLE hList, uint64_t uIdx)
{
    AssertPtrReturn(pTransfer, NULL);

    RT_NOREF(hList, uIdx);

    LogFlowFunc(("hList=%RU64\n", hList));

    return NULL;
}

/**
 * Reads a single Shared Clipboard list entry.
 *
 * @returns VBox status code or VERR_NO_MORE_FILES if the end of the list has been reached.
 * @param   pTransfer           Clipboard transfer to handle.
 * @param   hList               List handle of list to read from.
 * @param   pEntry              Where to store the read information.
 */
int ShClTransferListRead(PSHCLTRANSFER pTransfer, SHCLLISTHANDLE hList,
                         PSHCLLISTENTRY pEntry)
{
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);
    AssertPtrReturn(pEntry,    VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    LogFlowFunc(("hList=%RU64\n", hList));

    if (pTransfer->State.enmSource == SHCLSOURCE_LOCAL)
    {
        PSHCLLISTHANDLEINFO pInfo = shClTransferListGetByHandle(pTransfer, hList);
        if (pInfo)
        {
            switch (pInfo->enmType)
            {
                case SHCLOBJTYPE_DIRECTORY:
                {
                    LogFlowFunc(("\tDirectory: %s\n", pInfo->pszPathLocalAbs));

                    for (;;)
                    {
                        bool fSkipEntry = false; /* Whether to skip an entry in the enumeration. */

                        size_t        cbDirEntry = 0;
                        PRTDIRENTRYEX pDirEntry  = NULL;
                        rc = RTDirReadExA(pInfo->u.Local.hDir, &pDirEntry, &cbDirEntry, RTFSOBJATTRADD_NOTHING, RTPATH_F_ON_LINK);
                        if (RT_SUCCESS(rc))
                        {
                            switch (pDirEntry->Info.Attr.fMode & RTFS_TYPE_MASK)
                            {
                                case RTFS_TYPE_DIRECTORY:
                                {
                                    /* Skip "." and ".." entries. */
                                    if (RTDirEntryExIsStdDotLink(pDirEntry))
                                    {
                                        fSkipEntry = true;
                                        break;
                                    }

                                    LogFlowFunc(("Directory: %s\n", pDirEntry->szName));
                                    break;
                                }

                                case RTFS_TYPE_FILE:
                                {
                                    LogFlowFunc(("File: %s\n", pDirEntry->szName));
                                    break;
                                }

                                case RTFS_TYPE_SYMLINK:
                                {
                                    rc = VERR_NOT_IMPLEMENTED; /** @todo Not implemented yet. */
                                    break;
                                }

                                default:
                                    break;
                            }

                            if (   RT_SUCCESS(rc)
                                && !fSkipEntry)
                            {
                                rc = RTStrCopy(pEntry->pszName, pEntry->cbName, pDirEntry->szName);
                                if (RT_SUCCESS(rc))
                                {
                                    pEntry->cbName = (uint32_t)strlen(pEntry->pszName) + 1; /* Include termination. */

                                    AssertPtr(pEntry->pvInfo);
                                    Assert   (pEntry->cbInfo == sizeof(SHCLFSOBJINFO));

                                    ShClFsObjFromIPRT(PSHCLFSOBJINFO(pEntry->pvInfo), &pDirEntry->Info);

                                    LogFlowFunc(("Entry pszName=%s, pvInfo=%p, cbInfo=%RU32\n",
                                                 pEntry->pszName, pEntry->pvInfo, pEntry->cbInfo));
                                }
                            }

                            RTDirReadExAFree(&pDirEntry, &cbDirEntry);
                        }

                        if (   !fSkipEntry /* Do we have a valid entry? Bail out. */
                            || RT_FAILURE(rc))
                        {
                            break;
                        }
                    }

                    break;
                }

                case SHCLOBJTYPE_FILE:
                {
                    LogFlowFunc(("\tSingle file: %s\n", pInfo->pszPathLocalAbs));

                    RTFSOBJINFO objInfo;
                    rc = RTFileQueryInfo(pInfo->u.Local.hFile, &objInfo, RTFSOBJATTRADD_NOTHING);
                    if (RT_SUCCESS(rc))
                    {
                        pEntry->pvInfo = (PSHCLFSOBJINFO)RTMemAlloc(sizeof(SHCLFSOBJINFO));
                        if (pEntry->pvInfo)
                        {
                            rc = RTStrCopy(pEntry->pszName, pEntry->cbName, pInfo->pszPathLocalAbs);
                            if (RT_SUCCESS(rc))
                            {
                                ShClFsObjFromIPRT(PSHCLFSOBJINFO(pEntry->pvInfo), &objInfo);

                                pEntry->cbInfo = sizeof(SHCLFSOBJINFO);
                                pEntry->fInfo  = VBOX_SHCL_INFO_FLAG_FSOBJINFO;
                            }
                        }
                        else
                            rc = VERR_NO_MEMORY;
                    }

                    break;
                }

                default:
                    rc = VERR_NOT_SUPPORTED;
                    break;
            }
        }
        else
            rc = VERR_NOT_FOUND;
    }
    else if (pTransfer->State.enmSource == SHCLSOURCE_REMOTE)
    {
        if (pTransfer->ProviderIface.pfnListEntryRead)
            rc = pTransfer->ProviderIface.pfnListEntryRead(&pTransfer->ProviderCtx, hList, pEntry);
        else
            rc = VERR_NOT_SUPPORTED;
    }
    else
        AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int ShClTransferListWrite(PSHCLTRANSFER pTransfer, SHCLLISTHANDLE hList,
                          PSHCLLISTENTRY pEntry)
{
    RT_NOREF(pTransfer, hList, pEntry);

    int rc = VINF_SUCCESS;

#if 0
    if (pTransfer->ProviderIface.pfnListEntryWrite)
        rc = pTransfer->ProviderIface.pfnListEntryWrite(&pTransfer->ProviderCtx, hList, pEntry);
#endif

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Returns whether a given list handle is valid or not.
 *
 * @returns \c true if list handle is valid, \c false if not.
 * @param   pTransfer           Clipboard transfer to handle.
 * @param   hList               List handle to check.
 */
bool ShClTransferListHandleIsValid(PSHCLTRANSFER pTransfer, SHCLLISTHANDLE hList)
{
    bool fIsValid = false;

    if (pTransfer->State.enmSource == SHCLSOURCE_LOCAL)
    {
        fIsValid = shClTransferListGetByHandle(pTransfer, hList) != NULL;
    }
    else if (pTransfer->State.enmSource == SHCLSOURCE_REMOTE)
    {
        AssertFailed(); /** @todo Implement. */
    }
    else
        AssertFailedStmt(fIsValid = false);

    return fIsValid;
}

/**
 * Copies a transfer callback table from source to destination.
 *
 * @param   pCallbacksDst       Callback destination.
 * @param   pCallbacksSrc       Callback source. If set to NULL, the
 *                              destination callback table will be unset.
 */
void ShClTransferCopyCallbacks(PSHCLTRANSFERCALLBACKTABLE pCallbacksDst,
                               PSHCLTRANSFERCALLBACKTABLE pCallbacksSrc)
{
    AssertPtrReturnVoid(pCallbacksDst);

    if (pCallbacksSrc) /* Set */
    {
#define SET_CALLBACK(a_pfnCallback) \
        if (pCallbacksSrc->a_pfnCallback) \
            pCallbacksDst->a_pfnCallback = pCallbacksSrc->a_pfnCallback

        SET_CALLBACK(pfnOnInitialize);
        SET_CALLBACK(pfnOnStart);
        SET_CALLBACK(pfnOnCompleted);
        SET_CALLBACK(pfnOnError);
        SET_CALLBACK(pfnOnRegistered);
        SET_CALLBACK(pfnOnUnregistered);

#undef SET_CALLBACK

        pCallbacksDst->pvUser = pCallbacksSrc->pvUser;
        pCallbacksDst->cbUser = pCallbacksSrc->cbUser;
    }
    else /* Unset */
        RT_BZERO(pCallbacksDst, sizeof(SHCLTRANSFERCALLBACKTABLE));
}

/**
 * Sets or unsets the callback table to be used for a Shared Clipboard transfer.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer to set callbacks for.
 * @param   pCallbacks          Pointer to callback table to set. If set to NULL,
 *                              existing callbacks for this transfer will be unset.
 */
void ShClTransferSetCallbacks(PSHCLTRANSFER pTransfer,
                              PSHCLTRANSFERCALLBACKTABLE pCallbacks)
{
    AssertPtrReturnVoid(pTransfer);
    /* pCallbacks can be NULL. */

    ShClTransferCopyCallbacks(&pTransfer->Callbacks, pCallbacks);
}

/**
 * Sets the transfer provider interface for a given transfer.
 *
 * @returns VBox status code.
 * @param   pTransfer           Transfer to create transfer provider for.
 * @param   pCreationCtx        Provider creation context to use for provider creation.
 */
int ShClTransferSetProviderIface(PSHCLTRANSFER pTransfer,
                                 PSHCLTXPROVIDERCREATIONCTX pCreationCtx)
{
    AssertPtrReturn(pTransfer,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCreationCtx, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    int rc = VINF_SUCCESS;

    pTransfer->ProviderIface         = pCreationCtx->Interface;
    pTransfer->ProviderCtx.pTransfer = pTransfer;
    pTransfer->ProviderCtx.pvUser    = pCreationCtx->pvUser;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Clears (resets) the root list of a Shared Clipboard transfer.
 *
 * @param   pTransfer           Transfer to clear transfer root list for.
 */
static void shClTransferListRootsClear(PSHCLTRANSFER pTransfer)
{
    AssertPtrReturnVoid(pTransfer);

    if (pTransfer->pszPathRootAbs)
    {
        RTStrFree(pTransfer->pszPathRootAbs);
        pTransfer->pszPathRootAbs = NULL;
    }

    PSHCLLISTROOT pListRoot, pListRootNext;
    RTListForEachSafe(&pTransfer->lstRoots, pListRoot, pListRootNext, SHCLLISTROOT, Node)
    {
        RTStrFree(pListRoot->pszPathAbs);

        RTListNodeRemove(&pListRoot->Node);

        RTMemFree(pListRoot);
        pListRoot = NULL;
    }

    pTransfer->cRoots = 0;
}

/**
 * Resets a Shared Clipboard transfer.
 *
 * @param   pTransfer           Clipboard transfer to reset.
 */
void ShClTransferReset(PSHCLTRANSFER pTransfer)
{
    AssertPtrReturnVoid(pTransfer);

    LogFlowFuncEnter();

    shClTransferListRootsClear(pTransfer);

    PSHCLLISTHANDLEINFO pItList, pItListNext;
    RTListForEachSafe(&pTransfer->lstList, pItList, pItListNext, SHCLLISTHANDLEINFO, Node)
    {
        ShClTransferListHandleInfoDestroy(pItList);

        RTListNodeRemove(&pItList->Node);

        RTMemFree(pItList);
    }

    PSHCLOBJHANDLEINFO pItObj, pItObjNext;
    RTListForEachSafe(&pTransfer->lstObj, pItObj, pItObjNext, SHCLOBJHANDLEINFO, Node)
    {
        ShClTransferObjHandleInfoDestroy(pItObj);

        RTListNodeRemove(&pItObj->Node);

        RTMemFree(pItObj);
    }
}

/**
 * Returns the number of transfer root list entries.
 *
 * @returns Root list entry count.
 * @param   pTransfer           Clipboard transfer to return root entry count for.
 */
uint32_t ShClTransferRootsCount(PSHCLTRANSFER pTransfer)
{
    AssertPtrReturn(pTransfer, 0);

    LogFlowFunc(("[Transfer %RU32] cRoots=%RU64\n", pTransfer->State.uID, pTransfer->cRoots));
    return (uint32_t)pTransfer->cRoots;
}

/**
 * Returns a specific root list entry of a transfer.
 *
 * @returns Pointer to root list entry if found, or NULL if not found.
 * @param   pTransfer           Clipboard transfer to get root list entry from.
 * @param   uIdx                Index of root list entry to return.
 */
DECLINLINE(PSHCLLISTROOT) shClTransferRootsGetInternal(PSHCLTRANSFER pTransfer, uint32_t uIdx)
{
    if (uIdx >= pTransfer->cRoots)
        return NULL;

    PSHCLLISTROOT pIt = RTListGetFirst(&pTransfer->lstRoots, SHCLLISTROOT, Node);
    while (uIdx--) /** @todo Slow, but works for now. */
        pIt = RTListGetNext(&pTransfer->lstRoots, pIt, SHCLLISTROOT, Node);

    return pIt;
}

/**
 * Get a specific root list entry.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer to get root list entry of.
 * @param   uIndex              Index (zero-based) of entry to get.
 * @param   pEntry              Where to store the returned entry on success.
 */
int ShClTransferRootsEntry(PSHCLTRANSFER pTransfer,
                           uint64_t uIndex, PSHCLROOTLISTENTRY pEntry)
{
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);
    AssertPtrReturn(pEntry,    VERR_INVALID_POINTER);

    if (uIndex >= pTransfer->cRoots)
        return VERR_INVALID_PARAMETER;

    int rc;

    PSHCLLISTROOT pRoot = shClTransferRootsGetInternal(pTransfer, uIndex);
    AssertPtrReturn(pRoot, VERR_INVALID_PARAMETER);

    /* Make sure that we only advertise relative source paths, not absolute ones. */
    const char *pcszSrcPath = pRoot->pszPathAbs;

    char *pszFileName = RTPathFilename(pcszSrcPath);
    if (pszFileName)
    {
        Assert(pszFileName >= pcszSrcPath);
        size_t cchDstBase = pszFileName - pcszSrcPath;
        const char *pszDstPath = &pcszSrcPath[cchDstBase];

        LogFlowFunc(("pcszSrcPath=%s, pszDstPath=%s\n", pcszSrcPath, pszDstPath));

        rc = ShClTransferListEntryInit(pEntry);
        if (RT_SUCCESS(rc))
        {
            rc = RTStrCopy(pEntry->pszName, pEntry->cbName, pszDstPath);
            if (RT_SUCCESS(rc))
            {
                pEntry->cbInfo = sizeof(SHCLFSOBJINFO);
                pEntry->pvInfo = (PSHCLFSOBJINFO)RTMemAlloc(pEntry->cbInfo);
                if (pEntry->pvInfo)
                {
                    RTFSOBJINFO fsObjInfo;
                    rc = RTPathQueryInfo(pcszSrcPath, &fsObjInfo, RTFSOBJATTRADD_NOTHING);
                    if (RT_SUCCESS(rc))
                    {
                        ShClFsObjFromIPRT(PSHCLFSOBJINFO(pEntry->pvInfo), &fsObjInfo);

                        pEntry->fInfo = VBOX_SHCL_INFO_FLAG_FSOBJINFO;
                    }
                }
                else
                    rc = VERR_NO_MEMORY;
            }
        }
    }
    else
        rc = VERR_INVALID_POINTER;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Returns the root entries of a Shared Clipboard transfer.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer to return root entries for.
 * @param   ppRootList          Where to store the root list on success.
 */
int ShClTransferRootsGet(PSHCLTRANSFER pTransfer, PSHCLROOTLIST *ppRootList)
{
    AssertPtrReturn(pTransfer,  VERR_INVALID_POINTER);
    AssertPtrReturn(ppRootList, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    int rc = VINF_SUCCESS;

    if (pTransfer->State.enmSource == SHCLSOURCE_LOCAL)
    {
        PSHCLROOTLIST pRootList = ShClTransferRootListAlloc();
        if (!pRootList)
            return VERR_NO_MEMORY;

        const uint64_t cRoots = (uint32_t)pTransfer->cRoots;

        LogFlowFunc(("cRoots=%RU64\n", cRoots));

        if (cRoots)
        {
            PSHCLROOTLISTENTRY paRootListEntries
                = (PSHCLROOTLISTENTRY)RTMemAllocZ(cRoots * sizeof(SHCLROOTLISTENTRY));
            if (paRootListEntries)
            {
                for (uint64_t i = 0; i < cRoots; ++i)
                {
                    rc = ShClTransferRootsEntry(pTransfer, i, &paRootListEntries[i]);
                    if (RT_FAILURE(rc))
                        break;
                }

                if (RT_SUCCESS(rc))
                    pRootList->paEntries = paRootListEntries;
            }
            else
                rc = VERR_NO_MEMORY;
        }
        else
            rc = VERR_NOT_FOUND;

        if (RT_SUCCESS(rc))
        {
            pRootList->Hdr.cRoots = cRoots;
            pRootList->Hdr.fRoots = 0; /** @todo Implement this. */

            *ppRootList = pRootList;
        }
    }
    else if (pTransfer->State.enmSource == SHCLSOURCE_REMOTE)
    {
        if (pTransfer->ProviderIface.pfnRootsGet)
            rc = pTransfer->ProviderIface.pfnRootsGet(&pTransfer->ProviderCtx, ppRootList);
        else
            rc = VERR_NOT_SUPPORTED;
    }
    else
        AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Sets transfer root list entries for a given transfer.
 *
 * @returns VBox status code.
 * @param   pTransfer           Transfer to set transfer list entries for.
 * @param   pszRoots            String list (separated by CRLF) of root entries to set.
 *                              All entries must have the same root path.
 * @param   cbRoots             Size (in bytes) of string list.
 */
int ShClTransferRootsSet(PSHCLTRANSFER pTransfer, const char *pszRoots, size_t cbRoots)
{
    AssertPtrReturn(pTransfer,      VERR_INVALID_POINTER);
    AssertPtrReturn(pszRoots,       VERR_INVALID_POINTER);
    AssertReturn(cbRoots,           VERR_INVALID_PARAMETER);

    if (!RTStrIsValidEncoding(pszRoots))
        return VERR_INVALID_UTF8_ENCODING;

    int rc = VINF_SUCCESS;

    shClTransferListRootsClear(pTransfer);

    char  *pszPathRootAbs = NULL;

    RTCList<RTCString> lstRootEntries = RTCString(pszRoots, cbRoots - 1).split("\r\n");
    for (size_t i = 0; i < lstRootEntries.size(); ++i)
    {
        PSHCLLISTROOT pListRoot = (PSHCLLISTROOT)RTMemAlloc(sizeof(SHCLLISTROOT));
        AssertPtrBreakStmt(pListRoot, rc = VERR_NO_MEMORY);

        const char *pszPathCur = RTStrDup(lstRootEntries.at(i).c_str());

        LogFlowFunc(("pszPathCur=%s\n", pszPathCur));

        /* No root path determined yet? */
        if (!pszPathRootAbs)
        {
            pszPathRootAbs = RTStrDup(pszPathCur);
            if (pszPathRootAbs)
            {
                RTPathStripFilename(pszPathRootAbs);

                LogFlowFunc(("pszPathRootAbs=%s\n", pszPathRootAbs));

                /* We don't want to have a relative directory here. */
                if (RTPathStartsWithRoot(pszPathRootAbs))
                {
                    rc = shClTransferValidatePath(pszPathRootAbs, true /* Path must exist */);
                }
                else
                    rc = VERR_INVALID_PARAMETER;
            }
            else
                rc = VERR_NO_MEMORY;
        }

        if (RT_FAILURE(rc))
            break;

        pListRoot->pszPathAbs = RTStrDup(pszPathCur);
        if (!pListRoot->pszPathAbs)
        {
            rc = VERR_NO_MEMORY;
            break;
        }

        RTListAppend(&pTransfer->lstRoots, &pListRoot->Node);

        pTransfer->cRoots++;
    }

    /* No (valid) root directory found? Bail out early. */
    if (!pszPathRootAbs)
        rc = VERR_PATH_NOT_FOUND;

    if (RT_SUCCESS(rc))
    {
        /*
         * Step 2:
         * Go through the created list and make sure all entries have the same root path.
         */
        PSHCLLISTROOT pListRoot;
        RTListForEach(&pTransfer->lstRoots, pListRoot, SHCLLISTROOT, Node)
        {
            if (!RTStrStartsWith(pListRoot->pszPathAbs, pszPathRootAbs))
            {
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            rc = shClTransferValidatePath(pListRoot->pszPathAbs, true /* Path must exist */);
            if (RT_FAILURE(rc))
                break;
        }
    }

    /** @todo Entry rollback on failure? */

    if (RT_SUCCESS(rc))
    {
        pTransfer->pszPathRootAbs = pszPathRootAbs;
        LogFlowFunc(("pszPathRootAbs=%s, cRoots=%zu\n", pTransfer->pszPathRootAbs, pTransfer->cRoots));

        LogRel2(("Shared Clipboard: Transfer uses root '%s'\n", pTransfer->pszPathRootAbs));
    }
    else
    {
        LogRel(("Shared Clipboard: Unable to set roots for transfer, rc=%Rrc\n", rc));
        RTStrFree(pszPathRootAbs);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Returns the transfer's ID.
 *
 * @returns The transfer's ID.
 * @param   pTransfer           Clipboard transfer to return ID for.
 */
SHCLTRANSFERID ShClTransferGetID(PSHCLTRANSFER pTransfer)
{
    AssertPtrReturn(pTransfer, 0);

    return pTransfer->State.uID;
}

/**
 * Returns the transfer's direction.
 *
 * @returns The transfer's direction.
 * @param   pTransfer           Clipboard transfer to return direction for.
 */
SHCLTRANSFERDIR ShClTransferGetDir(PSHCLTRANSFER pTransfer)
{
    AssertPtrReturn(pTransfer, SHCLTRANSFERDIR_UNKNOWN);

    LogFlowFunc(("[Transfer %RU32] enmDir=%RU32\n", pTransfer->State.uID, pTransfer->State.enmDir));
    return pTransfer->State.enmDir;
}

/**
 * Returns the transfer's source.
 *
 * @returns The transfer's source.
 * @param   pTransfer           Clipboard transfer to return source for.
 */
SHCLSOURCE ShClTransferGetSource(PSHCLTRANSFER pTransfer)
{
    AssertPtrReturn(pTransfer, SHCLSOURCE_INVALID);

    LogFlowFunc(("[Transfer %RU32] enmSource=%RU32\n", pTransfer->State.uID, pTransfer->State.enmSource));
    return pTransfer->State.enmSource;
}

/**
 * Returns the current transfer status.
 *
 * @returns Current transfer status.
 * @param   pTransfer           Clipboard transfer to return status for.
 */
SHCLTRANSFERSTATUS ShClTransferGetStatus(PSHCLTRANSFER pTransfer)
{
    AssertPtrReturn(pTransfer, SHCLTRANSFERSTATUS_NONE);

    LogFlowFunc(("[Transfer %RU32] enmStatus=%RU32\n", pTransfer->State.uID, pTransfer->State.enmStatus));
    return pTransfer->State.enmStatus;
}

/**
 * Runs a started Shared Clipboard transfer in a dedicated thread.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer to run.
 * @param   pfnThreadFunc       Pointer to thread function to use.
 * @param   pvUser              Pointer to user-provided data. Optional.
 */
int ShClTransferRun(PSHCLTRANSFER pTransfer, PFNRTTHREAD pfnThreadFunc, void *pvUser)
{
    AssertPtrReturn(pTransfer,     VERR_INVALID_POINTER);
    AssertPtrReturn(pfnThreadFunc, VERR_INVALID_POINTER);
    /* pvUser is optional. */

    AssertMsgReturn(pTransfer->State.enmStatus == SHCLTRANSFERSTATUS_STARTED,
                    ("Wrong status (currently is %s)\n", ShClTransferStatusToStr(pTransfer->State.enmStatus)),
                    VERR_WRONG_ORDER);

    int rc = shClTransferThreadCreate(pTransfer, pfnThreadFunc, pvUser);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Starts an initialized transfer.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer to start.
 */
int ShClTransferStart(PSHCLTRANSFER pTransfer)
{
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    /* Ready to start? */
    AssertMsgReturn(pTransfer->State.enmStatus == SHCLTRANSFERSTATUS_INITIALIZED,
                    ("Wrong status (currently is %s)\n", ShClTransferStatusToStr(pTransfer->State.enmStatus)),
                    VERR_WRONG_ORDER);

    int rc;

    if (pTransfer->Callbacks.pfnOnStart)
    {
        rc = pTransfer->Callbacks.pfnOnStart(&pTransfer->CallbackCtx);
    }
    else
        rc = VINF_SUCCESS;

    if (RT_SUCCESS(rc))
    {
        pTransfer->State.enmStatus = SHCLTRANSFERSTATUS_STARTED;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Creates a thread for a Shared Clipboard transfer.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer to create thread for.
 * @param   pfnThreadFunc       Thread function to use for this transfer.
 * @param   pvUser              Pointer to user-provided data.
 */
static int shClTransferThreadCreate(PSHCLTRANSFER pTransfer, PFNRTTHREAD pfnThreadFunc, void *pvUser)

{
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);

    /* Already marked for stopping? */
    AssertMsgReturn(pTransfer->Thread.fStop == false,
                    ("Transfer thread already marked for stopping"), VERR_WRONG_ORDER);
    /* Already started? */
    AssertMsgReturn(pTransfer->Thread.fStarted == false,
                    ("Transfer thread already started"), VERR_WRONG_ORDER);

    /* Spawn a worker thread, so that we don't block the window thread for too long. */
    int rc = RTThreadCreate(&pTransfer->Thread.hThread, pfnThreadFunc,
                            pvUser, 0, RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE,
                            "shclp");
    if (RT_SUCCESS(rc))
    {
        int rc2 = RTThreadUserWait(pTransfer->Thread.hThread, 30 * 1000 /* Timeout in ms */);
        AssertRC(rc2);

        if (pTransfer->Thread.fStarted) /* Did the thread indicate that it started correctly? */
        {
            /* Nothing to do in here. */
        }
        else
            rc = VERR_GENERAL_FAILURE; /** @todo Find a better rc. */
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Destroys a thread of a Shared Clipboard transfer.
 *
 * @returns VBox status code.
 * @param   pTransfer           Clipboard transfer to destroy thread for.
 * @param   uTimeoutMs          Timeout (in ms) to wait for thread creation.
 */
static int shClTransferThreadDestroy(PSHCLTRANSFER pTransfer, RTMSINTERVAL uTimeoutMs)
{
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);

    if (pTransfer->Thread.hThread == NIL_RTTHREAD)
        return VINF_SUCCESS;

    LogFlowFuncEnter();

    /* Set stop indicator. */
    pTransfer->Thread.fStop = true;

    int rcThread = VERR_WRONG_ORDER;
    int rc = RTThreadWait(pTransfer->Thread.hThread, uTimeoutMs, &rcThread);

    LogFlowFunc(("Waiting for thread resulted in %Rrc (thread exited with %Rrc)\n", rc, rcThread));

    return rc;
}

/**
 * Initializes a Shared Clipboard transfer context.
 *
 * @returns VBox status code.
 * @param   pTransferCtx                Transfer context to initialize.
 */
int ShClTransferCtxInit(PSHCLTRANSFERCTX pTransferCtx)
{
    AssertPtrReturn(pTransferCtx, VERR_INVALID_POINTER);

    LogFlowFunc(("pTransferCtx=%p\n", pTransferCtx));

    int rc = RTCritSectInit(&pTransferCtx->CritSect);
    if (RT_SUCCESS(rc))
    {
        RTListInit(&pTransferCtx->List);

        pTransferCtx->cTransfers  = 0;
        pTransferCtx->cRunning    = 0;
        pTransferCtx->cMaxRunning = 64; /** @todo Make this configurable? */

        RT_ZERO(pTransferCtx->bmTransferIds);

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS_HTTP
        ShClTransferHttpServerInit(&pTransferCtx->HttpServer);
#endif
        ShClTransferCtxReset(pTransferCtx);
    }

    return VINF_SUCCESS;
}

/**
 * Destroys a Shared Clipboard transfer context struct.
 *
 * @param   pTransferCtx                Transfer context to destroy.
 */
void ShClTransferCtxDestroy(PSHCLTRANSFERCTX pTransferCtx)
{
    if (!pTransferCtx)
        return;

    LogFlowFunc(("pTransferCtx=%p\n", pTransferCtx));

    if (RTCritSectIsInitialized(&pTransferCtx->CritSect))
        RTCritSectDelete(&pTransferCtx->CritSect);

    PSHCLTRANSFER pTransfer, pTransferNext;
    RTListForEachSafe(&pTransferCtx->List, pTransfer, pTransferNext, SHCLTRANSFER, Node)
    {
        ShClTransferDestroy(pTransfer);

        shclTransferCtxTransferRemoveAndUnregister(pTransferCtx, pTransfer);

        RTMemFree(pTransfer);
        pTransfer = NULL;
    }

    pTransferCtx->cRunning   = 0;
    pTransferCtx->cTransfers = 0;
}

/**
 * Resets a Shared Clipboard transfer.
 *
 * @param   pTransferCtx                Transfer context to reset.
 */
void ShClTransferCtxReset(PSHCLTRANSFERCTX pTransferCtx)
{
    AssertPtrReturnVoid(pTransferCtx);

    LogFlowFuncEnter();

    PSHCLTRANSFER pTransfer;
    RTListForEach(&pTransferCtx->List, pTransfer, SHCLTRANSFER, Node)
        ShClTransferReset(pTransfer);

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS_HTTP
    /** @todo Anything to do here? */
#endif
}

/**
 * Returns a specific Shared Clipboard transfer, internal version.
 *
 * @returns Shared Clipboard transfer, or NULL if not found.
 * @param   pTransferCtx                Transfer context to return transfer for.
 * @param   uID                         ID of the transfer to return.
 */
static PSHCLTRANSFER shClTransferCtxGetTransferByIdInternal(PSHCLTRANSFERCTX pTransferCtx, uint32_t uID)
{
    PSHCLTRANSFER pTransfer;
    RTListForEach(&pTransferCtx->List, pTransfer, SHCLTRANSFER, Node) /** @todo Slow, but works for now. */
    {
        if (pTransfer->State.uID == uID)
            return pTransfer;
    }

    return NULL;
}

/**
 * Returns a specific Shared Clipboard transfer by index, internal version.
 *
 * @returns Shared Clipboard transfer, or NULL if not found.
 * @param   pTransferCtx                Transfer context to return transfer for.
 * @param   uIdx                        Index of the transfer to return.
 */
static PSHCLTRANSFER shClTransferCtxGetTransferByIndexInternal(PSHCLTRANSFERCTX pTransferCtx, uint32_t uIdx)
{
    uint32_t idx = 0;

    PSHCLTRANSFER pTransfer;
    RTListForEach(&pTransferCtx->List, pTransfer, SHCLTRANSFER, Node) /** @todo Slow, but works for now. */
    {
        if (uIdx == idx)
            return pTransfer;
        idx++;
    }

    return NULL;
}

/**
 * Returns a Shared Clipboard transfer for a specific transfer ID.
 *
 * @returns Shared Clipboard transfer, or NULL if not found.
 * @param   pTransferCtx                Transfer context to return transfer for.
 * @param   uID                         ID of the transfer to return.
 */
PSHCLTRANSFER ShClTransferCtxGetTransferById(PSHCLTRANSFERCTX pTransferCtx, uint32_t uID)
{
    return shClTransferCtxGetTransferByIdInternal(pTransferCtx, uID);
}

/**
 * Returns a Shared Clipboard transfer for a specific list index.
 *
 * @returns Shared Clipboard transfer, or NULL if not found.
 * @param   pTransferCtx                Transfer context to return transfer for.
 * @param   uIdx                        List index of the transfer to return.
 */
PSHCLTRANSFER ShClTransferCtxGetTransferByIndex(PSHCLTRANSFERCTX pTransferCtx, uint32_t uIdx)
{
    return shClTransferCtxGetTransferByIndexInternal(pTransferCtx, uIdx);
}

/**
 * Returns the number of running Shared Clipboard transfers.
 *
 * @returns Number of running transfers.
 * @param   pTransferCtx                Transfer context to return number for.
 */
uint32_t ShClTransferCtxGetRunningTransfers(PSHCLTRANSFERCTX pTransferCtx)
{
    AssertPtrReturn(pTransferCtx, 0);
    return pTransferCtx->cRunning;
}

/**
 * Returns the number of total Shared Clipboard transfers.
 *
 * @returns Number of total transfers.
 * @param   pTransferCtx                Transfer context to return number for.
 */
uint32_t ShClTransferCtxGetTotalTransfers(PSHCLTRANSFERCTX pTransferCtx)
{
    AssertPtrReturn(pTransferCtx, 0);
    return pTransferCtx->cTransfers;
}

/**
 * Registers a Shared Clipboard transfer with a transfer context, i.e. allocates a transfer ID.
 *
 * @return  VBox status code.
 * @retval  VERR_SHCLPB_MAX_TRANSFERS_REACHED if the maximum of concurrent transfers
 *          is reached.
 * @param   pTransferCtx        Transfer context to register transfer to.
 * @param   pTransfer           Transfer to register.
 * @param   pidTransfer         Where to return the transfer ID on success. Optional.
 */
int ShClTransferCtxTransferRegister(PSHCLTRANSFERCTX pTransferCtx, PSHCLTRANSFER pTransfer, SHCLTRANSFERID *pidTransfer)
{
    AssertPtrReturn(pTransferCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);
    /* pidTransfer is optional. */

    /*
     * Pick a random bit as starting point.  If it's in use, search forward
     * for a free one, wrapping around.  We've reserved both the zero'th and
     * max-1 IDs.
     */
    SHCLTRANSFERID idTransfer = RTRandU32Ex(1, VBOX_SHCL_MAX_TRANSFERS - 2);

    if (!ASMBitTestAndSet(&pTransferCtx->bmTransferIds[0], idTransfer))
    { /* likely */ }
    else if (pTransferCtx->cTransfers < VBOX_SHCL_MAX_TRANSFERS - 2 /* First and last are not used */)
    {
        /* Forward search. */
        int iHit = ASMBitNextClear(&pTransferCtx->bmTransferIds[0], VBOX_SHCL_MAX_TRANSFERS, idTransfer);
        if (iHit < 0)
            iHit = ASMBitFirstClear(&pTransferCtx->bmTransferIds[0], VBOX_SHCL_MAX_TRANSFERS);
        AssertLogRelMsgReturn(iHit >= 0, ("Transfer count: %RU16\n", pTransferCtx->cTransfers), VERR_SHCLPB_MAX_TRANSFERS_REACHED);
        idTransfer = iHit;
        AssertLogRelMsgReturn(!ASMBitTestAndSet(&pTransferCtx->bmTransferIds[0], idTransfer), ("idObject=%#x\n", idTransfer), VERR_INTERNAL_ERROR_2);
    }
    else
    {
        LogFunc(("Maximum number of transfers reached (%RU16 transfers)\n", pTransferCtx->cTransfers));
        return VERR_SHCLPB_MAX_TRANSFERS_REACHED;
    }

    Log2Func(("pTransfer=%p, idTransfer=%RU32 (%RU16 transfers)\n", pTransfer, idTransfer, pTransferCtx->cTransfers));

    pTransfer->State.uID = idTransfer;

    RTListAppend(&pTransferCtx->List, &pTransfer->Node);

    pTransferCtx->cTransfers++;

    if (pTransfer->Callbacks.pfnOnRegistered)
        pTransfer->Callbacks.pfnOnRegistered(&pTransfer->CallbackCtx, pTransferCtx);

    if (pidTransfer)
        *pidTransfer = idTransfer;

    LogFlowFuncLeaveRC(VINF_SUCCESS);
    return VINF_SUCCESS;
}

/**
 * Registers a Shared Clipboard transfer with a transfer context by specifying an ID for the transfer.
 *
 * @return  VBox status code.
 * @retval  VERR_ALREADY_EXISTS if a transfer with the given ID already exists.
 * @retval  VERR_SHCLPB_MAX_TRANSFERS_REACHED if the maximum of concurrent transfers for this context has been reached.
 * @param   pTransferCtx                Transfer context to register transfer to.
 * @param   pTransfer           Transfer to register.
 * @param   idTransfer          Transfer ID to use for registration.
 */
int ShClTransferCtxTransferRegisterById(PSHCLTRANSFERCTX pTransferCtx, PSHCLTRANSFER pTransfer, SHCLTRANSFERID idTransfer)
{
    LogFlowFunc(("cTransfers=%RU16, idTransfer=%RU32\n", pTransferCtx->cTransfers, idTransfer));

    if (pTransferCtx->cTransfers < VBOX_SHCL_MAX_TRANSFERS - 2 /* First and last are not used */)
    {
        if (!ASMBitTestAndSet(&pTransferCtx->bmTransferIds[0], idTransfer))
        {
            RTListAppend(&pTransferCtx->List, &pTransfer->Node);

            pTransfer->State.uID = idTransfer;

            if (pTransfer->Callbacks.pfnOnRegistered)
                pTransfer->Callbacks.pfnOnRegistered(&pTransfer->CallbackCtx, pTransferCtx);

            pTransferCtx->cTransfers++;
            return VINF_SUCCESS;
        }

        return VERR_ALREADY_EXISTS;
    }

    LogFunc(("Maximum number of transfers reached (%RU16 transfers)\n", pTransferCtx->cTransfers));
    return VERR_SHCLPB_MAX_TRANSFERS_REACHED;
}

/**
 * Removes and unregisters a transfer from a transfer context.
 *
 * @param   pTransferCtx        Transfer context to remove transfer from.
 * @param   pTransfer           Transfer to remove.
 */
static void shclTransferCtxTransferRemoveAndUnregister(PSHCLTRANSFERCTX pTransferCtx, PSHCLTRANSFER pTransfer)
{
    RTListNodeRemove(&pTransfer->Node);

    Assert(pTransferCtx->cTransfers);
    pTransferCtx->cTransfers--;

    Assert(pTransferCtx->cTransfers >= pTransferCtx->cRunning);

    if (pTransfer->Callbacks.pfnOnUnregistered)
        pTransfer->Callbacks.pfnOnUnregistered(&pTransfer->CallbackCtx, pTransferCtx);

    LogFlowFunc(("Now %RU32 transfers left\n", pTransferCtx->cTransfers));
}

/**
 * Unregisters a transfer from an Transfer context.
 *
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_NOT_FOUND if the transfer ID was not found.
 * @param   pTransferCtx        Transfer context to unregister transfer from.
 * @param   idTransfer          Transfer ID to unregister.
 */
int ShClTransferCtxTransferUnregister(PSHCLTRANSFERCTX pTransferCtx, SHCLTRANSFERID idTransfer)
{
    int rc = VINF_SUCCESS;
    AssertMsgStmt(ASMBitTestAndClear(&pTransferCtx->bmTransferIds, idTransfer), ("idTransfer=%#x\n", idTransfer), rc = VERR_NOT_FOUND);

    LogFlowFunc(("idTransfer=%RU32\n", idTransfer));

    PSHCLTRANSFER pTransfer = shClTransferCtxGetTransferByIdInternal(pTransferCtx, idTransfer);
    if (pTransfer)
    {
        shclTransferCtxTransferRemoveAndUnregister(pTransferCtx, pTransfer);
    }
    else
        rc = VERR_NOT_FOUND;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Cleans up all associated transfers which are not needed (anymore).
 * This can be due to transfers which only have been announced but not / never being run.
 *
 * @param   pTransferCtx        Transfer context to cleanup transfers for.
 */
void ShClTransferCtxCleanup(PSHCLTRANSFERCTX pTransferCtx)
{
    AssertPtrReturnVoid(pTransferCtx);

    LogFlowFunc(("pTransferCtx=%p, cTransfers=%RU16 cRunning=%RU16\n",
                 pTransferCtx, pTransferCtx->cTransfers, pTransferCtx->cRunning));

    if (pTransferCtx->cTransfers == 0)
        return;

    /* Remove all transfers which are not in a running state (e.g. only announced). */
    PSHCLTRANSFER pTransfer, pTransferNext;
    RTListForEachSafe(&pTransferCtx->List, pTransfer, pTransferNext, SHCLTRANSFER, Node)
    {
        if (ShClTransferGetStatus(pTransfer) != SHCLTRANSFERSTATUS_STARTED)
        {
            shclTransferCtxTransferRemoveAndUnregister(pTransferCtx, pTransfer);

            ShClTransferDestroy(pTransfer);

            RTMemFree(pTransfer);
            pTransfer = NULL;
        }
    }
}

/**
 * Returns whether the maximum of concurrent transfers of a specific transfer contexthas been reached or not.
 *
 * @returns \c if maximum has been reached, \c false if not.
 * @param   pTransferCtx        Transfer context to determine value for.
 */
bool ShClTransferCtxTransfersMaximumReached(PSHCLTRANSFERCTX pTransferCtx)
{
    AssertPtrReturn(pTransferCtx, true);

    LogFlowFunc(("cRunning=%RU32, cMaxRunning=%RU32\n", pTransferCtx->cRunning, pTransferCtx->cMaxRunning));

    Assert(pTransferCtx->cRunning <= pTransferCtx->cMaxRunning);
    return pTransferCtx->cRunning == pTransferCtx->cMaxRunning;
}

/**
 * Copies file system objinfo from IPRT to Shared Clipboard format.
 *
 * @param   pDst                The Shared Clipboard structure to convert data to.
 * @param   pSrc                The IPRT structure to convert data from.
 */
void ShClFsObjFromIPRT(PSHCLFSOBJINFO pDst, PCRTFSOBJINFO pSrc)
{
    pDst->cbObject          = pSrc->cbObject;
    pDst->cbAllocated       = pSrc->cbAllocated;
    pDst->AccessTime        = pSrc->AccessTime;
    pDst->ModificationTime  = pSrc->ModificationTime;
    pDst->ChangeTime        = pSrc->ChangeTime;
    pDst->BirthTime         = pSrc->BirthTime;
    pDst->Attr.fMode        = pSrc->Attr.fMode;
    /* Clear bits which we don't pass through for security reasons. */
    pDst->Attr.fMode       &= ~(RTFS_UNIX_ISUID | RTFS_UNIX_ISGID | RTFS_UNIX_ISTXT);
    RT_ZERO(pDst->Attr.u);
    switch (pSrc->Attr.enmAdditional)
    {
        default:
        case RTFSOBJATTRADD_NOTHING:
            pDst->Attr.enmAdditional        = SHCLFSOBJATTRADD_NOTHING;
            break;

        case RTFSOBJATTRADD_UNIX:
            pDst->Attr.enmAdditional        = SHCLFSOBJATTRADD_UNIX;
            pDst->Attr.u.Unix.uid           = pSrc->Attr.u.Unix.uid;
            pDst->Attr.u.Unix.gid           = pSrc->Attr.u.Unix.gid;
            pDst->Attr.u.Unix.cHardlinks    = pSrc->Attr.u.Unix.cHardlinks;
            pDst->Attr.u.Unix.INodeIdDevice = pSrc->Attr.u.Unix.INodeIdDevice;
            pDst->Attr.u.Unix.INodeId       = pSrc->Attr.u.Unix.INodeId;
            pDst->Attr.u.Unix.fFlags        = pSrc->Attr.u.Unix.fFlags;
            pDst->Attr.u.Unix.GenerationId  = pSrc->Attr.u.Unix.GenerationId;
            pDst->Attr.u.Unix.Device        = pSrc->Attr.u.Unix.Device;
            break;

        case RTFSOBJATTRADD_EASIZE:
            pDst->Attr.enmAdditional        = SHCLFSOBJATTRADD_EASIZE;
            pDst->Attr.u.EASize.cb          = pSrc->Attr.u.EASize.cb;
            break;
    }
}

/**
 * Converts Shared Clipboard create flags (see SharedClipboard-transfers.h) into IPRT create flags.
 *
 * @returns IPRT status code.
 * @param       fShClFlags  Shared clipboard create flags.
 * @param[out]  pfOpen      Where to store the RTFILE_O_XXX flags for
 *                          RTFileOpen.
 *
 * @sa Initially taken from vbsfConvertFileOpenFlags().
 */
static int shClConvertFileCreateFlags(uint32_t fShClFlags, uint64_t *pfOpen)
{
    AssertMsgReturnStmt(!(fShClFlags & ~SHCL_OBJ_CF_VALID_MASK), ("%#x4\n", fShClFlags), *pfOpen = 0, VERR_INVALID_FLAGS);

    uint64_t fOpen = 0;

    switch (fShClFlags & SHCL_OBJ_CF_ACCESS_MASK_RW)
    {
        case SHCL_OBJ_CF_ACCESS_NONE:
        {
#ifdef RT_OS_WINDOWS
            if ((fShClFlags & SHCL_OBJ_CF_ACCESS_MASK_ATTR) != SHCL_OBJ_CF_ACCESS_ATTR_NONE)
                fOpen |= RTFILE_O_OPEN | RTFILE_O_ATTR_ONLY;
            else
#endif
                fOpen |= RTFILE_O_OPEN | RTFILE_O_READ;
            LogFlowFunc(("SHCL_OBJ_CF_ACCESS_NONE\n"));
            break;
        }

        case SHCL_OBJ_CF_ACCESS_READ:
        {
            fOpen |= RTFILE_O_OPEN | RTFILE_O_READ;
            LogFlowFunc(("SHCL_OBJ_CF_ACCESS_READ\n"));
            break;
        }

        default:
            AssertFailedReturn(VERR_IPE_NOT_REACHED_DEFAULT_CASE);
    }

    switch (fShClFlags & SHCL_OBJ_CF_ACCESS_MASK_ATTR)
    {
        case SHCL_OBJ_CF_ACCESS_ATTR_NONE:
        {
            fOpen |= RTFILE_O_ACCESS_ATTR_DEFAULT;
            LogFlowFunc(("SHCL_OBJ_CF_ACCESS_ATTR_NONE\n"));
            break;
        }

        case SHCL_OBJ_CF_ACCESS_ATTR_READ:
        {
            fOpen |= RTFILE_O_ACCESS_ATTR_READ;
            LogFlowFunc(("SHCL_OBJ_CF_ACCESS_ATTR_READ\n"));
            break;
        }

        default:
            AssertFailedReturn(VERR_IPE_NOT_REACHED_DEFAULT_CASE);
    }

    /* Sharing mask */
    switch (fShClFlags & SHCL_OBJ_CF_ACCESS_MASK_DENY)
    {
        case SHCL_OBJ_CF_ACCESS_DENYNONE:
            fOpen |= RTFILE_O_DENY_NONE;
            LogFlowFunc(("SHCL_OBJ_CF_ACCESS_DENYNONE\n"));
            break;

        case SHCL_OBJ_CF_ACCESS_DENYWRITE:
            fOpen |= RTFILE_O_DENY_WRITE;
            LogFlowFunc(("SHCL_OBJ_CF_ACCESS_DENYWRITE\n"));
            break;

        default:
            AssertFailedReturn(VERR_IPE_NOT_REACHED_DEFAULT_CASE);
    }

    *pfOpen = fOpen;

    LogFlowFuncLeaveRC(VINF_SUCCESS);
    return VINF_SUCCESS;
}

/**
 * Translates a Shared Clipboard transfer status (SHCLTRANSFERSTATUS_XXX) into a string.
 *
 * @returns Transfer status string name.
 * @param   enmStatus           The transfer status to translate.
 */
const char *ShClTransferStatusToStr(SHCLTRANSFERSTATUS enmStatus)
{
    switch (enmStatus)
    {
        RT_CASE_RET_STR(SHCLTRANSFERSTATUS_NONE);
        RT_CASE_RET_STR(SHCLTRANSFERSTATUS_INITIALIZED);
        RT_CASE_RET_STR(SHCLTRANSFERSTATUS_STARTED);
        RT_CASE_RET_STR(SHCLTRANSFERSTATUS_STOPPED);
        RT_CASE_RET_STR(SHCLTRANSFERSTATUS_CANCELED);
        RT_CASE_RET_STR(SHCLTRANSFERSTATUS_KILLED);
        RT_CASE_RET_STR(SHCLTRANSFERSTATUS_ERROR);
    }
    return "Unknown";
}
