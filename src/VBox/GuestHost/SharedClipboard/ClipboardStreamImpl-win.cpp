/* $Id: ClipboardStreamImpl-win.cpp $ */
/** @file
 * ClipboardStreamImpl-win.cpp - Shared Clipboard IStream object implementation (guest and host side).
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

#include <iprt/asm.h>
#include <iprt/ldr.h>
#include <iprt/thread.h>

#include <VBox/GuestHost/SharedClipboard.h>
#include <VBox/GuestHost/SharedClipboard-win.h>
#include <strsafe.h>

#include <VBox/log.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/



/*********************************************************************************************************************************
*   Static variables                                                                                                             *
*********************************************************************************************************************************/



SharedClipboardWinStreamImpl::SharedClipboardWinStreamImpl(SharedClipboardWinDataObject *pParent, PSHCLTRANSFER pTransfer,
                                                           const Utf8Str &strPath, PSHCLFSOBJINFO pObjInfo)
    : m_pParent(pParent)
    , m_lRefCount(1) /* Our IDataObjct *always* holds the last reference to this object; needed for the callbacks. */
    , m_pTransfer(pTransfer)
    , m_hObj(SHCLOBJHANDLE_INVALID)
    , m_strPath(strPath)
    , m_objInfo(*pObjInfo)
    , m_cbProcessed(0)
    , m_fIsComplete(false)
{
    AssertPtr(m_pTransfer);

    LogFunc(("m_strPath=%s\n", m_strPath.c_str()));
}

SharedClipboardWinStreamImpl::~SharedClipboardWinStreamImpl(void)
{
    LogFlowThisFuncEnter();
}

/*
 * IUnknown methods.
 */

STDMETHODIMP SharedClipboardWinStreamImpl::QueryInterface(REFIID iid, void **ppvObject)
{
    AssertPtrReturn(ppvObject, E_INVALIDARG);

    if (iid == IID_IUnknown)
    {
        LogFlowFunc(("IID_IUnknown\n"));
        *ppvObject = (IUnknown *)(ISequentialStream *)this;
    }
    else if (iid == IID_ISequentialStream)
    {
        LogFlowFunc(("IID_ISequentialStream\n"));
        *ppvObject = (ISequentialStream *)this;
    }
    else if (iid == IID_IStream)
    {
        LogFlowFunc(("IID_IStream\n"));
        *ppvObject = (IStream *)this;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) SharedClipboardWinStreamImpl::AddRef(void)
{
    LONG lCount = InterlockedIncrement(&m_lRefCount);
    LogFlowFunc(("lCount=%RI32\n", lCount));
    return lCount;
}

STDMETHODIMP_(ULONG) SharedClipboardWinStreamImpl::Release(void)
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

/*
 * IStream methods.
 */

STDMETHODIMP SharedClipboardWinStreamImpl::Clone(IStream** ppStream)
{
    RT_NOREF(ppStream);

    LogFlowFuncEnter();
    return E_NOTIMPL;
}

STDMETHODIMP SharedClipboardWinStreamImpl::Commit(DWORD dwFrags)
{
    RT_NOREF(dwFrags);

    LogFlowThisFuncEnter();
    return E_NOTIMPL;
}

STDMETHODIMP SharedClipboardWinStreamImpl::CopyTo(IStream *pDestStream, ULARGE_INTEGER nBytesToCopy, ULARGE_INTEGER *nBytesRead,
                                                  ULARGE_INTEGER *nBytesWritten)
{
    RT_NOREF(pDestStream, nBytesToCopy, nBytesRead, nBytesWritten);

    LogFlowThisFuncEnter();
    return E_NOTIMPL;
}

STDMETHODIMP SharedClipboardWinStreamImpl::LockRegion(ULARGE_INTEGER nStart, ULARGE_INTEGER nBytes,DWORD dwFlags)
{
    RT_NOREF(nStart, nBytes, dwFlags);

    LogFlowThisFuncEnter();
    return STG_E_INVALIDFUNCTION;
}

/* Note: Windows seems to assume EOF if nBytesRead < nBytesToRead. */
STDMETHODIMP SharedClipboardWinStreamImpl::Read(void *pvBuffer, ULONG nBytesToRead, ULONG *nBytesRead)
{
    LogFlowThisFunc(("Enter: m_cbProcessed=%RU64\n", m_cbProcessed));

    /** @todo Is there any locking required so that parallel reads aren't possible? */

    if (!pvBuffer)
        return STG_E_INVALIDPOINTER;

    if (   nBytesToRead == 0
        || m_fIsComplete)
    {
        if (nBytesRead)
            *nBytesRead = 0;
        return S_OK;
    }

    int rc;

    try
    {
        if (   m_hObj == SHCLOBJHANDLE_INVALID
            && m_pTransfer->ProviderIface.pfnObjOpen)
        {
            SHCLOBJOPENCREATEPARMS openParms;
            rc = ShClTransferObjOpenParmsInit(&openParms);
            if (RT_SUCCESS(rc))
            {
                openParms.fCreate = SHCL_OBJ_CF_ACCESS_READ
                                  | SHCL_OBJ_CF_ACCESS_DENYWRITE;

                rc = RTStrCopy(openParms.pszPath, openParms.cbPath, m_strPath.c_str());
                if (RT_SUCCESS(rc))
                {
                    rc = m_pTransfer->ProviderIface.pfnObjOpen(&m_pTransfer->ProviderCtx, &openParms, &m_hObj);
                }

                ShClTransferObjOpenParmsDestroy(&openParms);
            }
        }
        else
            rc = VINF_SUCCESS;

        uint32_t cbRead = 0;

        const uint64_t cbSize   = (uint64_t)m_objInfo.cbObject;
        const uint32_t cbToRead = RT_MIN(cbSize - m_cbProcessed, nBytesToRead);

        if (RT_SUCCESS(rc))
        {
            if (cbToRead)
            {
                rc = m_pTransfer->ProviderIface.pfnObjRead(&m_pTransfer->ProviderCtx, m_hObj,
                                                           pvBuffer, cbToRead, 0 /* fFlags */, &cbRead);
                if (RT_SUCCESS(rc))
                {
                    m_cbProcessed += cbRead;
                    Assert(m_cbProcessed <= cbSize);
                }
            }

            /* Transfer complete? Make sure to close the object again. */
            m_fIsComplete = m_cbProcessed == cbSize;

            if (m_fIsComplete)
            {
                if (m_pTransfer->ProviderIface.pfnObjClose)
                {
                    int rc2 = m_pTransfer->ProviderIface.pfnObjClose(&m_pTransfer->ProviderCtx, m_hObj);
                    AssertRC(rc2);
                }

                if (m_pParent)
                    m_pParent->OnTransferComplete();
            }
        }

        LogFlowThisFunc(("Leave: rc=%Rrc, cbSize=%RU64, cbProcessed=%RU64 -> nBytesToRead=%RU32, cbToRead=%RU32, cbRead=%RU32\n",
                         rc, cbSize, m_cbProcessed, nBytesToRead, cbToRead, cbRead));

        if (nBytesRead)
            *nBytesRead = (ULONG)cbRead;

        if (nBytesToRead != cbRead)
            return S_FALSE;

        return S_OK;
    }
    catch (...)
    {
        LogFunc(("Caught exception\n"));
    }

    return E_FAIL;
}

STDMETHODIMP SharedClipboardWinStreamImpl::Revert(void)
{
    LogFlowThisFuncEnter();
    return E_NOTIMPL;
}

STDMETHODIMP SharedClipboardWinStreamImpl::Seek(LARGE_INTEGER nMove, DWORD dwOrigin, ULARGE_INTEGER* nNewPos)
{
    RT_NOREF(nMove, dwOrigin, nNewPos);

    LogFlowThisFunc(("nMove=%RI64, dwOrigin=%RI32\n", nMove, dwOrigin));

    return E_NOTIMPL;
}

STDMETHODIMP SharedClipboardWinStreamImpl::SetSize(ULARGE_INTEGER nNewSize)
{
    RT_NOREF(nNewSize);

    LogFlowThisFuncEnter();
    return E_NOTIMPL;
}

STDMETHODIMP SharedClipboardWinStreamImpl::Stat(STATSTG *pStatStg, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if (pStatStg)
    {
        RT_ZERO(*pStatStg);

        switch (dwFlags)
        {
            case STATFLAG_NONAME:
                pStatStg->pwcsName = NULL;
                break;

            case STATFLAG_DEFAULT:
            {
                /** @todo r=bird: This is using the wrong allocator.  According to MSDN the
                 * caller will pass this to CoTaskMemFree, so we should use CoTaskMemAlloc to
                 * allocate it. */
                int rc2 = RTStrToUtf16(m_strPath.c_str(), &pStatStg->pwcsName);
                if (RT_FAILURE(rc2))
                    hr = E_FAIL;
                break;
            }

            default:
                hr = STG_E_INVALIDFLAG;
                break;
        }

        if (SUCCEEDED(hr))
        {
            pStatStg->type              = STGTY_STREAM;
            pStatStg->grfMode           = STGM_READ;
            pStatStg->grfLocksSupported = 0;
            pStatStg->cbSize.QuadPart   = (uint64_t)m_objInfo.cbObject;
        }
    }
    else
       hr = STG_E_INVALIDPOINTER;

    LogFlowThisFunc(("hr=%Rhrc\n", hr));
    return hr;
}

STDMETHODIMP SharedClipboardWinStreamImpl::UnlockRegion(ULARGE_INTEGER nStart, ULARGE_INTEGER nBytes, DWORD dwFlags)
{
    RT_NOREF(nStart, nBytes, dwFlags);

    LogFlowThisFuncEnter();
    return E_NOTIMPL;
}

STDMETHODIMP SharedClipboardWinStreamImpl::Write(const void *pvBuffer, ULONG nBytesToRead, ULONG *nBytesRead)
{
    RT_NOREF(pvBuffer, nBytesToRead, nBytesRead);

    LogFlowThisFuncEnter();
    return E_NOTIMPL;
}

/*
 * Own stuff.
 */

/**
 * Factory to create our own IStream implementation.
 *
 * @returns HRESULT
 * @param   pParent             Pointer to the parent data object.
 * @param   pTransfer           Pointer to Shared Clipboard transfer object to use.
 * @param   strPath             Path of object to handle for the stream.
 * @param   pObjInfo            Pointer to object information.
 * @param   ppStream            Where to return the created stream object on success.
 */
/* static */
HRESULT SharedClipboardWinStreamImpl::Create(SharedClipboardWinDataObject *pParent, PSHCLTRANSFER pTransfer,
                                             const Utf8Str &strPath, PSHCLFSOBJINFO pObjInfo,
                                             IStream **ppStream)
{
    AssertPtrReturn(pTransfer, E_POINTER);

    SharedClipboardWinStreamImpl *pStream = new SharedClipboardWinStreamImpl(pParent, pTransfer, strPath, pObjInfo);
    if (pStream)
    {
        pStream->AddRef();

        *ppStream = pStream;
        return S_OK;
    }

    return E_FAIL;
}

