/* $Id: ClipboardDataObjectImpl-win.cpp $ */
/** @file
 * ClipboardDataObjectImpl-win.cpp - Shared Clipboard IDataObject implementation.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_SHARED_CLIPBOARD
#include <VBox/GuestHost/SharedClipboard-win.h>
#include <VBox/GuestHost/SharedClipboard-transfers.h>

#include <iprt/win/windows.h>
#include <iprt/win/shlobj.h>
#include <iprt/win/shlwapi.h>

#include <iprt/asm.h>
#include <iprt/err.h>
#include <iprt/path.h>
#include <iprt/semaphore.h>
#include <iprt/uri.h>
#include <iprt/utf16.h>

#include <iprt/errcore.h>
#include <VBox/log.h>

/** @todo Also handle Unicode entries.
 *        !!! WARNING: Buggy, doesn't work yet (some memory corruption / garbage in the file name descriptions) !!! */
//#define VBOX_CLIPBOARD_WITH_UNICODE_SUPPORT 1

SharedClipboardWinDataObject::SharedClipboardWinDataObject(PSHCLTRANSFER pTransfer,
                                                           LPFORMATETC pFormatEtc, LPSTGMEDIUM pStgMed, ULONG cFormats)
    : m_enmStatus(Uninitialized)
    , m_lRefCount(0)
    , m_cFormats(0)
    , m_pTransfer(pTransfer)
    , m_pStream(NULL)
    , m_uObjIdx(0)
    , m_fRunning(false)
    , m_EventListComplete(NIL_RTSEMEVENT)
    , m_EventTransferComplete(NIL_RTSEMEVENT)
{
    AssertPtr(m_pTransfer);

    HRESULT hr;

    ULONG cFixedFormats = 3; /* CFSTR_FILEDESCRIPTORA + CFSTR_FILECONTENTS + CFSTR_PERFORMEDDROPEFFECT */
#ifdef VBOX_CLIPBOARD_WITH_UNICODE_SUPPORT
    cFixedFormats++; /* CFSTR_FILEDESCRIPTORW */
#endif
    const ULONG cAllFormats   = cFormats + cFixedFormats;

    try
    {
        m_pFormatEtc = new FORMATETC[cAllFormats];
        RT_BZERO(m_pFormatEtc, sizeof(FORMATETC) * cAllFormats);
        m_pStgMedium = new STGMEDIUM[cAllFormats];
        RT_BZERO(m_pStgMedium, sizeof(STGMEDIUM) * cAllFormats);

        /** @todo Do we need CFSTR_FILENAME / CFSTR_SHELLIDLIST here? */

        /*
         * Register fixed formats.
         */
        unsigned uIdx = 0;

        LogFlowFunc(("Registering CFSTR_FILEDESCRIPTORA ...\n"));
        m_cfFileDescriptorA = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);
        registerFormat(&m_pFormatEtc[uIdx++], m_cfFileDescriptorA);
#ifdef VBOX_CLIPBOARD_WITH_UNICODE_SUPPORT
        LogFlowFunc(("Registering CFSTR_FILEDESCRIPTORW ...\n"));
        m_cfFileDescriptorW = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
        registerFormat(&m_pFormatEtc[uIdx++], m_cfFileDescriptorW);
#endif

        /* IStream interface, implemented in ClipboardStreamImpl-win.cpp. */
        LogFlowFunc(("Registering CFSTR_FILECONTENTS ...\n"));
        m_cfFileContents = RegisterClipboardFormat(CFSTR_FILECONTENTS);
        registerFormat(&m_pFormatEtc[uIdx++], m_cfFileContents, TYMED_ISTREAM, 0 /* lIndex */);

        /* We want to know from the target what the outcome of the operation was to react accordingly (e.g. abort a transfer). */
        LogFlowFunc(("Registering CFSTR_PERFORMEDDROPEFFECT ...\n"));
        m_cfPerformedDropEffect = RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT);
        registerFormat(&m_pFormatEtc[uIdx++], m_cfPerformedDropEffect, TYMED_HGLOBAL, -1 /* lIndex */, DVASPECT_CONTENT);

        /*
         * Registration of dynamic formats needed?
         */
        LogFlowFunc(("%RU32 dynamic formats\n", cFormats));
        if (cFormats)
        {
            AssertPtr(pFormatEtc);
            AssertPtr(pStgMed);

            for (ULONG i = 0; i < cFormats; i++)
            {
                LogFlowFunc(("Format %RU32: cfFormat=%RI16, tyMed=%RU32, dwAspect=%RU32\n",
                             i, pFormatEtc[i].cfFormat, pFormatEtc[i].tymed, pFormatEtc[i].dwAspect));
                m_pFormatEtc[cFixedFormats + i] = pFormatEtc[i];
                m_pStgMedium[cFixedFormats + i] = pStgMed[i];
            }
        }

        hr = S_OK;
    }
    catch (std::bad_alloc &)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        m_cFormats  = cAllFormats;
        m_enmStatus = Initialized;

        int rc2 = RTSemEventCreate(&m_EventListComplete);
        AssertRC(rc2);
        rc2 = RTSemEventCreate(&m_EventTransferComplete);
        AssertRC(rc2);
    }

    LogFlowFunc(("cAllFormats=%RU32, hr=%Rhrc\n", cAllFormats, hr));
}

SharedClipboardWinDataObject::~SharedClipboardWinDataObject(void)
{
    LogFlowFuncEnter();

    RTSemEventDestroy(m_EventListComplete);
    m_EventListComplete = NIL_RTSEMEVENT;

    RTSemEventDestroy(m_EventTransferComplete);
    m_EventTransferComplete = NIL_RTSEMEVENT;

    if (m_pStream)
        m_pStream->Release();

    if (m_pFormatEtc)
        delete[] m_pFormatEtc;

    if (m_pStgMedium)
        delete[] m_pStgMedium;

    LogFlowFunc(("mRefCount=%RI32\n", m_lRefCount));
}

/*
 * IUnknown methods.
 */

STDMETHODIMP_(ULONG) SharedClipboardWinDataObject::AddRef(void)
{
    LONG lCount = InterlockedIncrement(&m_lRefCount);
    LogFlowFunc(("lCount=%RI32\n", lCount));
    return lCount;
}

STDMETHODIMP_(ULONG) SharedClipboardWinDataObject::Release(void)
{
    LONG lCount = InterlockedDecrement(&m_lRefCount);
    LogFlowFunc(("lCount=%RI32\n", m_lRefCount));
    if (lCount == 0)
    {
        delete this;
        return 0;
    }

    return lCount;
}

STDMETHODIMP SharedClipboardWinDataObject::QueryInterface(REFIID iid, void **ppvObject)
{
    AssertPtrReturn(ppvObject, E_INVALIDARG);

    if (   iid == IID_IDataObject
        || iid == IID_IUnknown)
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }

    *ppvObject = 0;
    return E_NOINTERFACE;
}

/**
 * Copies a chunk of data into a HGLOBAL object.
 *
 * @returns VBox status code.
 * @param   pvData              Data to copy.
 * @param   cbData              Size (in bytes) to copy.
 * @param   fFlags              GlobalAlloc flags, used for allocating the HGLOBAL block.
 * @param   phGlobal            Where to store the allocated HGLOBAL object.
 */
int SharedClipboardWinDataObject::copyToHGlobal(const void *pvData, size_t cbData, UINT fFlags, HGLOBAL *phGlobal)
{
    AssertPtrReturn(phGlobal, VERR_INVALID_POINTER);

    HGLOBAL hGlobal = GlobalAlloc(fFlags, cbData);
    if (!hGlobal)
        return VERR_NO_MEMORY;

    void *pvAlloc = GlobalLock(hGlobal);
    if (pvAlloc)
    {
        CopyMemory(pvAlloc, pvData, cbData);
        GlobalUnlock(hGlobal);

        *phGlobal = hGlobal;

        return VINF_SUCCESS;
    }

    GlobalFree(hGlobal);
    return VERR_ACCESS_DENIED;
}

/**
 * Reads (handles) a specific directory reursively and inserts its entry into the
 * objects's entry list.
 *
 * @returns VBox status code.
 * @param   pTransfer           Shared Clipboard transfer object to handle.
 * @param   strDir              Directory path to handle.
 */
int SharedClipboardWinDataObject::readDir(PSHCLTRANSFER pTransfer, const Utf8Str &strDir)
{
    LogFlowFunc(("strDir=%s\n", strDir.c_str()));

    SHCLLISTOPENPARMS openParmsList;
    int rc = ShClTransferListOpenParmsInit(&openParmsList);
    if (RT_SUCCESS(rc))
    {
        rc = RTStrCopy(openParmsList.pszPath, openParmsList.cbPath, strDir.c_str());
        if (RT_SUCCESS(rc))
        {
            SHCLLISTHANDLE hList;
            rc = ShClTransferListOpen(pTransfer, &openParmsList, &hList);
            if (RT_SUCCESS(rc))
            {
                LogFlowFunc(("strDir=%s -> hList=%RU64\n", strDir.c_str(), hList));

                SHCLLISTHDR hdrList;
                rc = ShClTransferListGetHeader(pTransfer, hList, &hdrList);
                if (RT_SUCCESS(rc))
                {
                    LogFlowFunc(("cTotalObjects=%RU64, cbTotalSize=%RU64\n\n",
                                 hdrList.cTotalObjects, hdrList.cbTotalSize));

                    for (uint64_t o = 0; o < hdrList.cTotalObjects; o++)
                    {
                        SHCLLISTENTRY entryList;
                        rc = ShClTransferListEntryInit(&entryList);
                        if (RT_SUCCESS(rc))
                        {
                            rc = ShClTransferListRead(pTransfer, hList, &entryList);
                            if (RT_SUCCESS(rc))
                            {
                                if (ShClTransferListEntryIsValid(&entryList))
                                {
                                    PSHCLFSOBJINFO pFsObjInfo = (PSHCLFSOBJINFO)entryList.pvInfo;
                                    Assert(entryList.cbInfo == sizeof(SHCLFSOBJINFO));

                                    Utf8Str strPath = strDir + Utf8Str("\\") + Utf8Str(entryList.pszName);

                                    LogFlowFunc(("\t%s (%RU64 bytes) -> %s\n",
                                                 entryList.pszName, pFsObjInfo->cbObject, strPath.c_str()));

                                    if (RTFS_IS_DIRECTORY(pFsObjInfo->Attr.fMode))
                                    {
                                        FSOBJENTRY objEntry = { strPath.c_str(), *pFsObjInfo };

                                        m_lstEntries.push_back(objEntry); /** @todo Can this throw? */

                                        rc = readDir(pTransfer, strPath.c_str());
                                    }
                                    else if (RTFS_IS_FILE(pFsObjInfo->Attr.fMode))
                                    {
                                        FSOBJENTRY objEntry = { strPath.c_str(), *pFsObjInfo };

                                        m_lstEntries.push_back(objEntry); /** @todo Can this throw? */
                                    }
                                    else
                                        rc = VERR_NOT_SUPPORTED;

                                    /** @todo Handle symlinks. */
                                }
                                else
                                    rc = VERR_INVALID_PARAMETER;
                            }

                            ShClTransferListEntryDestroy(&entryList);
                        }

                        if (   RT_FAILURE(rc)
                            && pTransfer->Thread.fStop)
                            break;
                    }
                }

                ShClTransferListClose(pTransfer, hList);
            }
        }

        ShClTransferListOpenParmsDestroy(&openParmsList);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Thread for reading transfer data.
 * The data object needs the (high level, root) transfer listing at the time of ::GetData(), so we need
 * to block and wait until we have this data (via this thread) and continue.
 *
 * @returns VBox status code.
 * @param   ThreadSelf          Thread handle. Unused at the moment.
 * @param   pvUser              Pointer to user-provided data. Of type SharedClipboardWinDataObject.
 */
/* static */
DECLCALLBACK(int) SharedClipboardWinDataObject::readThread(RTTHREAD ThreadSelf, void *pvUser)
{
    RT_NOREF(ThreadSelf);

    LogFlowFuncEnter();

    SharedClipboardWinDataObject *pThis = (SharedClipboardWinDataObject *)pvUser;

    PSHCLTRANSFER pTransfer = pThis->m_pTransfer;
    AssertPtr(pTransfer);

    pTransfer->Thread.fStarted = true;
    pTransfer->Thread.fStop    = false;

    RTThreadUserSignal(RTThreadSelf());

    LogRel2(("Shared Clipboard: Calculating transfer ...\n"));

    PSHCLROOTLIST pRootList;
    int rc = ShClTransferRootsGet(pTransfer, &pRootList);
    if (RT_SUCCESS(rc))
    {
        LogFlowFunc(("cRoots=%RU32\n\n", pRootList->Hdr.cRoots));

        for (uint32_t i = 0; i < pRootList->Hdr.cRoots; i++)
        {
            PSHCLLISTENTRY pRootEntry = &pRootList->paEntries[i];
            AssertPtr(pRootEntry);

            Assert(pRootEntry->cbInfo == sizeof(SHCLFSOBJINFO));
            PSHCLFSOBJINFO pFsObjInfo = (PSHCLFSOBJINFO)pRootEntry->pvInfo;

            LogFlowFunc(("pszRoot=%s, fMode=0x%x\n", pRootEntry->pszName, pFsObjInfo->Attr.fMode));

            if (RTFS_IS_DIRECTORY(pFsObjInfo->Attr.fMode))
            {
                FSOBJENTRY objEntry = { pRootEntry->pszName, *pFsObjInfo };

                pThis->m_lstEntries.push_back(objEntry); /** @todo Can this throw? */

                rc = pThis->readDir(pTransfer, pRootEntry->pszName);
            }
            else if (RTFS_IS_FILE(pFsObjInfo->Attr.fMode))
            {
                FSOBJENTRY objEntry = { pRootEntry->pszName, *pFsObjInfo };

                pThis->m_lstEntries.push_back(objEntry); /** @todo Can this throw? */
            }
            else
                rc = VERR_NOT_SUPPORTED;

            if (ASMAtomicReadBool(&pTransfer->Thread.fStop))
            {
                LogRel2(("Shared Clipboard: Stopping transfer calculation ...\n"));
                break;
            }

            if (RT_FAILURE(rc))
                break;
        }

        ShClTransferRootListFree(pRootList);
        pRootList = NULL;

        if (   RT_SUCCESS(rc)
            && !ASMAtomicReadBool(&pTransfer->Thread.fStop))
        {
            LogRel2(("Shared Clipboard: Transfer calculation complete (%zu root entries)\n", pThis->m_lstEntries.size()));

            /*
             * Signal the "list complete" event so that this data object can return (valid) data via ::GetData().
             * This in turn then will create IStream instances (by the OS) for each file system object to handle.
             */
            int rc2 = RTSemEventSignal(pThis->m_EventListComplete);
            AssertRC(rc2);

            if (pThis->m_lstEntries.size())
            {
                LogRel2(("Shared Clipboard: Waiting for transfer to complete ...\n"));

                LogFlowFunc(("Waiting for transfer to complete ...\n"));

                /* Transferring stuff can take a while, so don't use any timeout here. */
                rc2 = RTSemEventWait(pThis->m_EventTransferComplete, RT_INDEFINITE_WAIT);
                AssertRC(rc2);

                switch (pThis->m_enmStatus)
                {
                    case Completed:
                        LogRel2(("Shared Clipboard: Transfer complete\n"));
                        break;

                    case Canceled:
                        LogRel2(("Shared Clipboard: Transfer canceled\n"));
                        break;

                    case Error:
                        LogRel2(("Shared Clipboard: Transfer error occurred\n"));
                        break;

                    default:
                        break;
                }
            }
            else
               LogRel(("Shared Clipboard: No transfer root entries found -- should not happen, please file a bug report\n"));
        }
        else if (RT_FAILURE(rc))
            LogRel(("Shared Clipboard: Transfer failed with %Rrc\n", rc));
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Creates a FILEGROUPDESCRIPTOR object from a given Shared Clipboard transfer and stores the result into an HGLOBAL object.
 *
 * @returns VBox status code.
 * @param   pTransfer           Shared Clipboard transfer to create file grou desciprtor for.
 * @param   fUnicode            Whether the FILEGROUPDESCRIPTOR object shall contain Unicode data or not.
 * @param   phGlobal            Where to store the allocated HGLOBAL object on success.
 */
int SharedClipboardWinDataObject::createFileGroupDescriptorFromTransfer(PSHCLTRANSFER pTransfer,
                                                                        bool fUnicode, HGLOBAL *phGlobal)
{
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);
    AssertPtrReturn(phGlobal,  VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    const size_t cbFileGroupDescriptor = fUnicode ? sizeof(FILEGROUPDESCRIPTORW) : sizeof(FILEGROUPDESCRIPTORA);
    const size_t cbFileDescriptor = fUnicode ? sizeof(FILEDESCRIPTORW) : sizeof(FILEDESCRIPTORA);

    const UINT   cItems = (UINT)m_lstEntries.size(); /** UINT vs. size_t. */
    if (!cItems)
        return VERR_NOT_FOUND;

    UINT         curIdx = 0; /* Current index of the handled file group descriptor (FGD). */

    const size_t cbFGD  = cbFileGroupDescriptor + (cbFileDescriptor * (cItems - 1));

    LogFunc(("fUnicode=%RTbool, cItems=%u, cbFileDescriptor=%zu\n", fUnicode, cItems, cbFileDescriptor));

    /* FILEGROUPDESCRIPTORA / FILEGROUPDESCRIPTOR matches except the cFileName member (TCHAR vs. WCHAR). */
    FILEGROUPDESCRIPTOR *pFGD = (FILEGROUPDESCRIPTOR *)RTMemAllocZ(cbFGD);
    if (!pFGD)
        return VERR_NO_MEMORY;

    int rc = VINF_SUCCESS;

    pFGD->cItems = cItems;

    char *pszFileSpec = NULL;

    FsObjEntryList::const_iterator itRoot = m_lstEntries.begin();
    while (itRoot != m_lstEntries.end())
    {
        FILEDESCRIPTOR *pFD = &pFGD->fgd[curIdx];
        RT_BZERO(pFD, cbFileDescriptor);

        const char *pszFile = itRoot->strPath.c_str();
        AssertPtr(pszFile);

        pszFileSpec = RTStrDup(pszFile);
        AssertBreakStmt(pszFileSpec != NULL, rc = VERR_NO_MEMORY);

        if (fUnicode)
        {
            PRTUTF16 pwszFileSpec;
            rc = RTStrToUtf16(pszFileSpec, &pwszFileSpec);
            if (RT_SUCCESS(rc))
            {
                rc = RTUtf16CopyEx((PRTUTF16 )pFD->cFileName, sizeof(pFD->cFileName) / sizeof(WCHAR),
                                   pwszFileSpec, RTUtf16Len(pwszFileSpec));
                RTUtf16Free(pwszFileSpec);

                LogFlowFunc(("pFD->cFileNameW=%ls\n", pFD->cFileName));
            }
        }
        else
        {
            rc = RTStrCopy(pFD->cFileName, sizeof(pFD->cFileName), pszFileSpec);
            LogFlowFunc(("pFD->cFileNameA=%s\n", pFD->cFileName));
        }

        RTStrFree(pszFileSpec);
        pszFileSpec = NULL;

        if (RT_FAILURE(rc))
            break;

        pFD->dwFlags          = FD_PROGRESSUI | FD_ATTRIBUTES;
        if (fUnicode) /** @todo Only >= Vista. */
            pFD->dwFlags     |= FD_UNICODE;
        pFD->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

        const SHCLFSOBJINFO *pObjInfo = &itRoot->objInfo;

        if (RTFS_IS_DIRECTORY(pObjInfo->Attr.fMode))
        {
            pFD->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
        }
        else if (RTFS_IS_FILE(pObjInfo->Attr.fMode))
        {
            pFD->dwFlags |= FD_FILESIZE;

            const uint64_t cbObjSize = pObjInfo->cbObject;

            pFD->nFileSizeHigh = RT_HI_U32(cbObjSize);
            pFD->nFileSizeLow  = RT_LO_U32(cbObjSize);
        }
        else if (RTFS_IS_SYMLINK(pObjInfo->Attr.fMode))
        {
            /** @todo Implement. */
        }
#if 0 /** @todo Implement this. */
        pFD->dwFlags = FD_ATTRIBUTES | FD_CREATETIME | FD_ACCESSTIME | FD_WRITESTIME | FD_FILESIZE;
        pFD->dwFileAttributes =
        pFD->ftCreationTime   =
        pFD->ftLastAccessTime =
        pFD->ftLastWriteTime  =
#endif
        ++curIdx;
        ++itRoot;
    }

    if (pszFileSpec)
        RTStrFree(pszFileSpec);

    if (RT_SUCCESS(rc))
        rc = copyToHGlobal(pFGD, cbFGD, GMEM_MOVEABLE, phGlobal);

    RTMemFree(pFGD);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Retrieves the data stored in this object and store the result in
 * pMedium.
 *
 * @return  IPRT status code.
 * @return  HRESULT
 * @param   pFormatEtc
 * @param   pMedium
 */
STDMETHODIMP SharedClipboardWinDataObject::GetData(LPFORMATETC pFormatEtc, LPSTGMEDIUM pMedium)
{
    AssertPtrReturn(pFormatEtc, DV_E_FORMATETC);
    AssertPtrReturn(pMedium, DV_E_FORMATETC);

    LogFlowFuncEnter();

    LogFlowFunc(("lIndex=%RI32\n", pFormatEtc->lindex));

    /*
     * Initialize default values.
     */
    RT_BZERO(pMedium, sizeof(STGMEDIUM));

    HRESULT hr = DV_E_FORMATETC; /* Play safe. */

    if (   pFormatEtc->cfFormat == m_cfFileDescriptorA
#ifdef VBOX_CLIPBOARD_WITH_UNICODE_SUPPORT
        || pFormatEtc->cfFormat == m_cfFileDescriptorW
#endif
       )
    {
        const bool fUnicode = pFormatEtc->cfFormat == m_cfFileDescriptorW;

        const uint32_t enmTransferStatus = ShClTransferGetStatus(m_pTransfer);
        RT_NOREF(enmTransferStatus);

        LogFlowFunc(("FormatIndex_FileDescriptor%s, enmTransferStatus=%s, m_fRunning=%RTbool\n",
                     fUnicode ? "W" : "A", ShClTransferStatusToStr(enmTransferStatus), m_fRunning));

        int rc;

        /* The caller can call GetData() several times, so make sure we don't do the same transfer multiple times. */
        if (!m_fRunning)
        {
            /* Start the transfer asynchronously in a separate thread. */
            rc = ShClTransferRun(m_pTransfer, &SharedClipboardWinDataObject::readThread, this);
            if (RT_SUCCESS(rc))
            {
                m_fRunning = true;

                /* Don't block for too long here, as this also will screw other apps running on the OS. */
                LogFunc(("Waiting for listing to arrive ...\n"));
                rc = RTSemEventWait(m_EventListComplete, 30 * 1000 /* 30s timeout */);
                if (RT_SUCCESS(rc))
                {
                    LogFunc(("Listing complete\n"));
                }
            }
        }
        else
            rc = VINF_SUCCESS;

        if (RT_SUCCESS(rc))
        {
            HGLOBAL hGlobal;
            rc = createFileGroupDescriptorFromTransfer(m_pTransfer, fUnicode, &hGlobal);
            if (RT_SUCCESS(rc))
            {
                pMedium->tymed   = TYMED_HGLOBAL;
                pMedium->hGlobal = hGlobal;
                /* Note: hGlobal now is being owned by pMedium / the caller. */

                hr = S_OK;
            }
            else /* We can't tell any better to the caller, unfortunately. */
                hr = E_UNEXPECTED;
        }

        if (RT_FAILURE(rc))
            LogRel(("Shared Clipboard: Data object unable to get data, rc=%Rrc\n", rc));
    }

    if (pFormatEtc->cfFormat == m_cfFileContents)
    {
        if (          pFormatEtc->lindex >= 0
            && (ULONG)pFormatEtc->lindex <  m_lstEntries.size())
        {
            m_uObjIdx = pFormatEtc->lindex; /* lIndex of FormatEtc contains the actual index to the object being handled. */

            FSOBJENTRY &fsObjEntry = m_lstEntries.at(m_uObjIdx);

            LogFlowFunc(("FormatIndex_FileContents: m_uObjIdx=%u (entry '%s')\n", m_uObjIdx, fsObjEntry.strPath.c_str()));

            LogRel2(("Shared Clipboard: Receiving object '%s' ...\n", fsObjEntry.strPath.c_str()));

            /* Hand-in the provider so that our IStream implementation can continue working with it. */
            hr = SharedClipboardWinStreamImpl::Create(this /* pParent */, m_pTransfer,
                                                      fsObjEntry.strPath.c_str()/* File name */, &fsObjEntry.objInfo /* PSHCLFSOBJINFO */,
                                                      &m_pStream);
            if (SUCCEEDED(hr))
            {
                /* Hand over the stream to the caller. */
                pMedium->tymed = TYMED_ISTREAM;
                pMedium->pstm  = m_pStream;
            }
        }
    }
    else if (pFormatEtc->cfFormat == m_cfPerformedDropEffect)
    {
        HGLOBAL hGlobal = GlobalAlloc(GHND, sizeof(DWORD));

        DWORD* pdwDropEffect = (DWORD*)GlobalLock(hGlobal);
        *pdwDropEffect = DROPEFFECT_COPY;

        GlobalUnlock(hGlobal);

        pMedium->tymed          = TYMED_HGLOBAL;
        pMedium->hGlobal        = hGlobal;
        pMedium->pUnkForRelease = NULL;
    }

    if (   FAILED(hr)
        && hr != DV_E_FORMATETC) /* Can happen if the caller queries unknown / unhandled formats. */
    {
        LogRel(("Shared Clipboard: Error returning data from data object (%Rhrc)\n", hr));
    }

    LogFlowFunc(("hr=%Rhrc\n", hr));
    return hr;
}

/**
 * Only required for IStream / IStorage interfaces.
 *
 * @return  IPRT status code.
 * @return  HRESULT
 * @param   pFormatEtc
 * @param   pMedium
 */
STDMETHODIMP SharedClipboardWinDataObject::GetDataHere(LPFORMATETC pFormatEtc, LPSTGMEDIUM pMedium)
{
    RT_NOREF(pFormatEtc, pMedium);
    LogFlowFunc(("\n"));
    return E_NOTIMPL;
}

/**
 * Query if this objects supports a specific format.
 *
 * @return  IPRT status code.
 * @return  HRESULT
 * @param   pFormatEtc
 */
STDMETHODIMP SharedClipboardWinDataObject::QueryGetData(LPFORMATETC pFormatEtc)
{
    LogFlowFunc(("\n"));
    return lookupFormatEtc(pFormatEtc, NULL /* puIndex */) ? S_OK : DV_E_FORMATETC;
}

STDMETHODIMP SharedClipboardWinDataObject::GetCanonicalFormatEtc(LPFORMATETC pFormatEtc, LPFORMATETC pFormatEtcOut)
{
    RT_NOREF(pFormatEtc);
    LogFlowFunc(("\n"));

    /* Set this to NULL in any case. */
    pFormatEtcOut->ptd = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP SharedClipboardWinDataObject::SetData(LPFORMATETC pFormatEtc, LPSTGMEDIUM pMedium, BOOL fRelease)
{
    if (   pFormatEtc == NULL
        || pMedium    == NULL)
        return E_INVALIDARG;

    if (pFormatEtc->lindex != -1)
        return DV_E_LINDEX;

    if (pFormatEtc->tymed != TYMED_HGLOBAL)
        return DV_E_TYMED;

    if (pFormatEtc->dwAspect != DVASPECT_CONTENT)
        return DV_E_DVASPECT;

    LogFlowFunc(("cfFormat=%RU16, lookupFormatEtc=%RTbool\n",
                 pFormatEtc->cfFormat, lookupFormatEtc(pFormatEtc, NULL /* puIndex */)));

    /* CFSTR_PERFORMEDDROPEFFECT is used by the drop target (caller of this IDataObject) to communicate
     * the outcome of the overall operation. */
    if (   pFormatEtc->cfFormat == m_cfPerformedDropEffect
        && pMedium->tymed       == TYMED_HGLOBAL)
    {
        DWORD dwEffect = *(DWORD *)GlobalLock(pMedium->hGlobal);
        GlobalUnlock(pMedium->hGlobal);

        LogFlowFunc(("dwEffect=%RI32\n", dwEffect));

        /* Did the user cancel the operation via UI (shell)? This also might happen when overwriting an existing file
         * and the user doesn't want to allow this. */
        if (dwEffect == DROPEFFECT_NONE)
        {
            LogRel2(("Shared Clipboard: Transfer canceled by user interaction\n"));

            OnTransferCanceled();
        }
        /** @todo Detect move / overwrite actions here. */

        if (fRelease)
            ReleaseStgMedium(pMedium);

        return S_OK;
    }

    return E_NOTIMPL;
}

STDMETHODIMP SharedClipboardWinDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc)
{
    LogFlowFunc(("dwDirection=%RI32, mcFormats=%RI32, mpFormatEtc=%p\n", dwDirection, m_cFormats, m_pFormatEtc));

    HRESULT hr;
    if (dwDirection == DATADIR_GET)
        hr = SharedClipboardWinEnumFormatEtc::CreateEnumFormatEtc(m_cFormats, m_pFormatEtc, ppEnumFormatEtc);
    else
        hr = E_NOTIMPL;

    LogFlowFunc(("hr=%Rhrc\n", hr));
    return hr;
}

STDMETHODIMP SharedClipboardWinDataObject::DAdvise(LPFORMATETC pFormatEtc, DWORD fAdvise, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    RT_NOREF(pFormatEtc, fAdvise, pAdvSink, pdwConnection);
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP SharedClipboardWinDataObject::DUnadvise(DWORD dwConnection)
{
    RT_NOREF(dwConnection);
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP SharedClipboardWinDataObject::EnumDAdvise(IEnumSTATDATA **ppEnumAdvise)
{
    RT_NOREF(ppEnumAdvise);
    return OLE_E_ADVISENOTSUPPORTED;
}

#ifdef VBOX_WITH_SHARED_CLIPBOARD_WIN_ASYNC
/*
 * IDataObjectAsyncCapability methods.
 */

STDMETHODIMP SharedClipboardWinDataObject::EndOperation(HRESULT hResult, IBindCtx *pbcReserved, DWORD dwEffects)
{
     RT_NOREF(hResult, pbcReserved, dwEffects);
     return E_NOTIMPL;
}

STDMETHODIMP SharedClipboardWinDataObject::GetAsyncMode(BOOL *pfIsOpAsync)
{
     RT_NOREF(pfIsOpAsync);
     return E_NOTIMPL;
}

STDMETHODIMP SharedClipboardWinDataObject::InOperation(BOOL *pfInAsyncOp)
{
     RT_NOREF(pfInAsyncOp);
     return E_NOTIMPL;
}

STDMETHODIMP SharedClipboardWinDataObject::SetAsyncMode(BOOL fDoOpAsync)
{
     RT_NOREF(fDoOpAsync);
     return E_NOTIMPL;
}

STDMETHODIMP SharedClipboardWinDataObject::StartOperation(IBindCtx *pbcReserved)
{
     RT_NOREF(pbcReserved);
     return E_NOTIMPL;
}
#endif /* VBOX_WITH_SHARED_CLIPBOARD_WIN_ASYNC */

/*
 * Own stuff.
 */

int SharedClipboardWinDataObject::Init(void)
{
    LogFlowFuncLeaveRC(VINF_SUCCESS);
    return VINF_SUCCESS;
}

void SharedClipboardWinDataObject::OnTransferComplete(int rc /* = VINF_SUCESS */)
{
    RT_NOREF(rc);

    LogFlowFunc(("m_uObjIdx=%RU32 (total: %zu)\n", m_uObjIdx, m_lstEntries.size()));

    if (RT_SUCCESS(rc))
    {
        const bool fComplete = m_uObjIdx == m_lstEntries.size() - 1 /* Object index is zero-based */;
        if (fComplete)
        {
            m_enmStatus = Completed;
        }
    }
    else
        m_enmStatus = Error;

    if (m_enmStatus != Initialized)
    {
        if (m_EventTransferComplete != NIL_RTSEMEVENT)
        {
            int rc2 = RTSemEventSignal(m_EventTransferComplete);
            AssertRC(rc2);
        }
    }

    LogFlowFuncLeaveRC(rc);
}

void SharedClipboardWinDataObject::OnTransferCanceled(void)
{
    LogFlowFuncEnter();

    m_enmStatus = Canceled;

    if (m_EventTransferComplete != NIL_RTSEMEVENT)
    {
        int rc2 = RTSemEventSignal(m_EventTransferComplete);
        AssertRC(rc2);
    }

    LogFlowFuncLeave();
}

/* static */
void SharedClipboardWinDataObject::logFormat(CLIPFORMAT fmt)
{
    char szFormat[128];
    if (GetClipboardFormatName(fmt, szFormat, sizeof(szFormat)))
    {
        LogFlowFunc(("clipFormat=%RI16 -> %s\n", fmt, szFormat));
    }
    else
        LogFlowFunc(("clipFormat=%RI16 is unknown\n", fmt));
}

bool SharedClipboardWinDataObject::lookupFormatEtc(LPFORMATETC pFormatEtc, ULONG *puIndex)
{
    AssertReturn(pFormatEtc, false);
    /* puIndex is optional. */

    for (ULONG i = 0; i < m_cFormats; i++)
    {
        if(    (pFormatEtc->tymed & m_pFormatEtc[i].tymed)
            && pFormatEtc->cfFormat == m_pFormatEtc[i].cfFormat)
            /* Note: Do *not* compare dwAspect here, as this can be dynamic, depending on how the object should be represented. */
            //&& pFormatEtc->dwAspect == m_pFormatEtc[i].dwAspect)
        {
            LogRel2(("Shared Clipboard: Format found: tyMed=%RI32, cfFormat=%RI16, dwAspect=%RI32, ulIndex=%RU32\n",
                     pFormatEtc->tymed, pFormatEtc->cfFormat, pFormatEtc->dwAspect, i));
            if (puIndex)
                *puIndex = i;
            return true;
        }
    }

    LogRel2(("Shared Clipboard: Format NOT found: tyMed=%RI32, cfFormat=%RI16, dwAspect=%RI32\n",
             pFormatEtc->tymed, pFormatEtc->cfFormat, pFormatEtc->dwAspect));

    logFormat(pFormatEtc->cfFormat);

    return false;
}

void SharedClipboardWinDataObject::registerFormat(LPFORMATETC pFormatEtc, CLIPFORMAT clipFormat,
                                                  TYMED tyMed, LONG lIndex, DWORD dwAspect,
                                                  DVTARGETDEVICE *pTargetDevice)
{
    AssertPtr(pFormatEtc);

    pFormatEtc->cfFormat = clipFormat;
    pFormatEtc->tymed    = tyMed;
    pFormatEtc->lindex   = lIndex;
    pFormatEtc->dwAspect = dwAspect;
    pFormatEtc->ptd      = pTargetDevice;

    LogFlowFunc(("Registered format=%ld\n", pFormatEtc->cfFormat));

    logFormat(pFormatEtc->cfFormat);
}
