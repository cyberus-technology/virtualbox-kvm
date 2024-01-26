/* $Id: VBoxSharedClipboardSvc-win.cpp $ */
/** @file
 * Shared Clipboard Service - Win32 host.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_SHARED_CLIPBOARD
#include <iprt/win/windows.h>

#include <VBox/HostServices/VBoxClipboardSvc.h>
#include <VBox/GuestHost/clipboard-helper.h>
#include <VBox/GuestHost/SharedClipboard-win.h>
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
# include <VBox/GuestHost/SharedClipboard-transfers.h>
#endif

#include <iprt/alloc.h>
#include <iprt/string.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/ldr.h>
#include <iprt/semaphore.h>
#include <iprt/thread.h>
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
# include <iprt/utf16.h>
#endif

#include <process.h>
#include <iprt/win/shlobj.h> /* Needed for shell objects. */

#include "VBoxSharedClipboardSvc-internal.h"
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
# include "VBoxSharedClipboardSvc-transfers.h"
#endif


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int vboxClipboardSvcWinSyncInternal(PSHCLCONTEXT pCtx);

struct SHCLCONTEXT
{
    /** Handle for window message handling thread. */
    RTTHREAD    hThread;
    /** Structure for keeping and communicating with service client. */
    PSHCLCLIENT pClient;
    /** Windows-specific context data. */
    SHCLWINCTX  Win;
};


/**
 * Copy clipboard data into the guest buffer.
 *
 * At first attempt, guest will provide a buffer of default size.
 * Usually 1K or 4K (see platform specific Guest Additions code around
 * VbglR3ClipboardReadData calls). If this buffer is not big enough
 * to fit host clipboard content, this function will return VINF_BUFFER_OVERFLOW
 * and provide guest with host's clipboard buffer actual size. This will be a
 * signal for the guest to re-read host clipboard data providing bigger buffer
 * to store it.
 *
 * @returns IPRT status code.
 * @returns VINF_BUFFER_OVERFLOW returned when guest buffer size if not big
 *          enough to store host clipboard data. This is a signal to the guest
 *          to re-issue host clipboard read request with bigger buffer size
 *          (specified in @a pcbActualDst output parameter).
 * @param   u32Format           VBox clipboard format (VBOX_SHCL_FMT_XXX) of copied data.
 * @param   pvSrc               Pointer to host clipboard data.
 * @param   cbSrc               Size (in bytes) of actual clipboard data to copy.
 * @param   pvDst               Pointer to guest buffer to store clipboard data.
 * @param   cbDst               Size (in bytes) of guest buffer.
 * @param   pcbActualDst        Actual size (in bytes) of host clipboard data.
 *                              Only set if guest buffer size if not big enough
 *                              to store host clipboard content. When set,
 *                              function returns VINF_BUFFER_OVERFLOW.
 */
static int vboxClipboardSvcWinDataGet(uint32_t u32Format, const void *pvSrc, uint32_t cbSrc,
                                      void *pvDst, uint32_t cbDst, uint32_t *pcbActualDst)
{
    AssertPtrReturn(pvSrc,        VERR_INVALID_POINTER);
    AssertReturn   (cbSrc,        VERR_INVALID_PARAMETER);
    AssertPtrReturn(pvDst,        VERR_INVALID_POINTER);
    AssertReturn   (cbDst,        VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcbActualDst, VERR_INVALID_POINTER);

    LogFlowFunc(("cbSrc = %d, cbDst = %d\n", cbSrc, cbDst));

    if (   u32Format == VBOX_SHCL_FMT_HTML
        && SharedClipboardWinIsCFHTML((const char *)pvSrc))
    {
        /** @todo r=bird: Why the double conversion? */
        char *pszBuf = NULL;
        uint32_t cbBuf = 0;
        int rc = SharedClipboardWinConvertCFHTMLToMIME((const char *)pvSrc, cbSrc, &pszBuf, &cbBuf);
        if (RT_SUCCESS(rc))
        {
            *pcbActualDst = cbBuf;
            if (cbBuf > cbDst)
            {
                /* Do not copy data. The dst buffer is not enough. */
                RTMemFree(pszBuf);
                return VINF_BUFFER_OVERFLOW;
            }
            memcpy(pvDst, pszBuf, cbBuf);
            RTMemFree(pszBuf);
        }
        else
            *pcbActualDst = 0;
    }
    else
    {
        *pcbActualDst = cbSrc; /* Tell the caller how much space we need. */

        if (cbSrc > cbDst)
            return VINF_BUFFER_OVERFLOW;

        memcpy(pvDst, pvSrc, cbSrc);
    }

#ifdef LOG_ENABLED
    ShClDbgDumpData(pvDst, cbSrc, u32Format);
#endif

    return VINF_SUCCESS;
}

static int vboxClipboardSvcWinDataRead(PSHCLCONTEXT pCtx, UINT uFormat, void **ppvData, uint32_t *pcbData)
{
    SHCLFORMAT fFormat = SharedClipboardWinClipboardFormatToVBox(uFormat);
    LogFlowFunc(("uFormat=%u -> uFmt=0x%x\n", uFormat, fFormat));

    if (fFormat == VBOX_SHCL_FMT_NONE)
    {
        LogRel2(("Shared Clipboard: Windows format %u not supported, ignoring\n", uFormat));
        return VERR_NOT_SUPPORTED;
    }

    PSHCLEVENT pEvent;
    int rc = ShClSvcGuestDataRequest(pCtx->pClient, fFormat, &pEvent);
    if (RT_SUCCESS(rc))
    {
        PSHCLEVENTPAYLOAD pPayload;
        rc = ShClEventWait(pEvent, 30 * 1000, &pPayload);
        if (RT_SUCCESS(rc))
        {
            *ppvData = pPayload ? pPayload->pvData : NULL;
            *pcbData = pPayload ? pPayload->cbData : 0;
        }

        ShClEventRelease(pEvent);
    }

    if (RT_FAILURE(rc))
        LogRel(("Shared Clipboard: Reading guest clipboard data for Windows host failed with %Rrc\n", rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static LRESULT CALLBACK vboxClipboardSvcWinWndProcMain(PSHCLCONTEXT pCtx,
                                                       HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) RT_NOTHROW_DEF
{
    AssertPtr(pCtx);

    LRESULT lresultRc = 0;

    const PSHCLWINCTX pWinCtx = &pCtx->Win;

    switch (uMsg)
    {
        case WM_CLIPBOARDUPDATE:
        {
            LogFunc(("WM_CLIPBOARDUPDATE\n"));

            int rc = RTCritSectEnter(&pWinCtx->CritSect);
            if (RT_SUCCESS(rc))
            {
                const HWND hWndClipboardOwner = GetClipboardOwner();

                LogFunc(("WM_CLIPBOARDUPDATE: hWndClipboardOwnerUs=%p, hWndNewClipboardOwner=%p\n",
                         pWinCtx->hWndClipboardOwnerUs, hWndClipboardOwner));

                if (pWinCtx->hWndClipboardOwnerUs != hWndClipboardOwner)
                {
                    int rc2 = RTCritSectLeave(&pWinCtx->CritSect);
                    AssertRC(rc2);

                    /* Clipboard was updated by another application, retrieve formats and report back. */
                    rc = vboxClipboardSvcWinSyncInternal(pCtx);
                }
                else
                {
                    int rc2 = RTCritSectLeave(&pWinCtx->CritSect);
                    AssertRC(rc2);
                }
            }

            if (RT_FAILURE(rc))
                LogRel(("Shared Clipboard: WM_CLIPBOARDUPDATE failed with %Rrc\n", rc));

            break;
        }

        case WM_CHANGECBCHAIN:
        {
            LogFunc(("WM_CHANGECBCHAIN\n"));
            lresultRc = SharedClipboardWinHandleWMChangeCBChain(pWinCtx, hWnd, uMsg, wParam, lParam);
            break;
        }

        case WM_DRAWCLIPBOARD:
        {
            LogFunc(("WM_DRAWCLIPBOARD\n"));

            int rc = RTCritSectEnter(&pWinCtx->CritSect);
            if (RT_SUCCESS(rc))
            {
                const HWND hWndClipboardOwner = GetClipboardOwner();

                LogFunc(("WM_DRAWCLIPBOARD: hWndClipboardOwnerUs=%p, hWndNewClipboardOwner=%p\n",
                         pWinCtx->hWndClipboardOwnerUs, hWndClipboardOwner));

                if (pWinCtx->hWndClipboardOwnerUs != hWndClipboardOwner)
                {
                    int rc2 = RTCritSectLeave(&pWinCtx->CritSect);
                    AssertRC(rc2);

                    /* Clipboard was updated by another application, retrieve formats and report back. */
                    rc = vboxClipboardSvcWinSyncInternal(pCtx);
                }
                else
                {
                    int rc2 = RTCritSectLeave(&pWinCtx->CritSect);
                    AssertRC(rc2);
                }
            }

            lresultRc = SharedClipboardWinChainPassToNext(pWinCtx, uMsg, wParam, lParam);
            break;
        }

        case WM_TIMER:
        {
            int rc = SharedClipboardWinHandleWMTimer(pWinCtx);
            AssertRC(rc);

            break;
        }

        case WM_RENDERFORMAT:
        {
            LogFunc(("WM_RENDERFORMAT\n"));

            /* Insert the requested clipboard format data into the clipboard. */
            const UINT       uFormat = (UINT)wParam;
            const SHCLFORMAT fFormat = SharedClipboardWinClipboardFormatToVBox(uFormat);
            LogFunc(("WM_RENDERFORMAT: uFormat=%u -> fFormat=0x%x\n", uFormat, fFormat));

            if (   fFormat       == VBOX_SHCL_FMT_NONE
                || pCtx->pClient == NULL)
            {
                /* Unsupported clipboard format is requested. */
                LogFunc(("WM_RENDERFORMAT unsupported format requested or client is not active\n"));
                SharedClipboardWinClear();
            }
            else
            {
                void    *pvData = NULL;
                uint32_t cbData = 0;
                int rc = vboxClipboardSvcWinDataRead(pCtx, uFormat, &pvData, &cbData);
                if (   RT_SUCCESS(rc)
                    && pvData
                    && cbData)
                {
                    /* Wrap HTML clipboard content info CF_HTML format if needed. */
                    if (fFormat == VBOX_SHCL_FMT_HTML
                        && !SharedClipboardWinIsCFHTML((char *)pvData))
                    {
                        char *pszWrapped = NULL;
                        uint32_t cbWrapped = 0;
                        rc = SharedClipboardWinConvertMIMEToCFHTML((char *)pvData, cbData, &pszWrapped, &cbWrapped);
                        if (RT_SUCCESS(rc))
                        {
                            /* Replace buffer with wrapped data content. */
                            RTMemFree(pvData);
                            pvData = (void *)pszWrapped;
                            cbData = cbWrapped;
                        }
                        else
                            LogRel(("Shared Clipboard: cannot convert HTML clipboard into CF_HTML format, rc=%Rrc\n", rc));
                    }

                    rc = SharedClipboardWinDataWrite(uFormat, pvData, cbData);
                    if (RT_FAILURE(rc))
                        LogRel(("Shared Clipboard: Setting clipboard data for Windows host failed with %Rrc\n", rc));

                    RTMemFree(pvData);
                    cbData = 0;
                }

                if (RT_FAILURE(rc))
                    SharedClipboardWinClear();
            }

            break;
        }

        case WM_RENDERALLFORMATS:
        {
            LogFunc(("WM_RENDERALLFORMATS\n"));

            int rc = SharedClipboardWinHandleWMRenderAllFormats(pWinCtx, hWnd);
            AssertRC(rc);

            break;
        }

        case SHCL_WIN_WM_REPORT_FORMATS:
        {
            /* Announce available formats. Do not insert data -- will be inserted in WM_RENDERFORMAT (or via IDataObject). */
            SHCLFORMATS fFormats = (uint32_t)lParam;
            LogFunc(("SHCL_WIN_WM_REPORT_FORMATS: fFormats=%#xn", fFormats));

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
            if (fFormats & VBOX_SHCL_FMT_URI_LIST)
            {
                PSHCLTRANSFER pTransfer;
                int rc = shClSvcTransferStart(pCtx->pClient,
                                              SHCLTRANSFERDIR_FROM_REMOTE, SHCLSOURCE_REMOTE,
                                              &pTransfer);
                if (RT_SUCCESS(rc))
                {
                    /* Create the IDataObject implementation the host OS needs and assign
                     * the newly created transfer to this object. */
                    rc = SharedClipboardWinTransferCreate(&pCtx->Win, pTransfer);

                    /*  Note: The actual requesting + retrieving of data will be done in the IDataObject implementation
                              (ClipboardDataObjectImpl::GetData()). */
                }
                else
                    LogRel(("Shared Clipboard: Initializing read transfer failed with %Rrc\n", rc));
            }
            else
            {
#endif
                int rc = SharedClipboardWinClearAndAnnounceFormats(pWinCtx, fFormats, hWnd);
                if (RT_FAILURE(rc))
                    LogRel(("Shared Clipboard: Reporting clipboard formats %#x to Windows host failed with %Rrc\n", fFormats, rc));

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
            }
#endif
            LogFunc(("SHCL_WIN_WM_REPORT_FORMATS: lastErr=%ld\n", GetLastError()));
            break;
        }

        case WM_DESTROY:
        {
            LogFunc(("WM_DESTROY\n"));

            int rc = SharedClipboardWinHandleWMDestroy(pWinCtx);
            AssertRC(rc);

            PostQuitMessage(0);
            break;
        }

        default:
            lresultRc = DefWindowProc(hWnd, uMsg, wParam, lParam);
            break;
    }

    LogFlowFunc(("LEAVE hWnd=%p, WM_ %u -> %#zx\n", hWnd, uMsg, lresultRc));
    return lresultRc;
}

/**
 * Static helper function for having a per-client proxy window instances.
 */
static LRESULT CALLBACK vboxClipboardSvcWinWndProcInstance(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) RT_NOTHROW_DEF
{
    LONG_PTR pUserData = GetWindowLongPtr(hWnd, GWLP_USERDATA);
    AssertPtrReturn(pUserData, 0);

    PSHCLCONTEXT pCtx = reinterpret_cast<PSHCLCONTEXT>(pUserData);
    if (pCtx)
        return vboxClipboardSvcWinWndProcMain(pCtx, hWnd, uMsg, wParam, lParam);

    return 0;
}

/**
 * Static helper function for routing Windows messages to a specific
 * proxy window instance.
 */
static LRESULT CALLBACK vboxClipboardSvcWinWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) RT_NOTHROW_DEF
{
    /* Note: WM_NCCREATE is not the first ever message which arrives, but
     *       early enough for us. */
    if (uMsg == WM_NCCREATE)
    {
        LogFlowFunc(("WM_NCCREATE\n"));

        LPCREATESTRUCT pCS = (LPCREATESTRUCT)lParam;
        AssertPtr(pCS);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pCS->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)vboxClipboardSvcWinWndProcInstance);

        return vboxClipboardSvcWinWndProcInstance(hWnd, uMsg, wParam, lParam);
    }

    /* No window associated yet. */
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

DECLCALLBACK(int) vboxClipboardSvcWinThread(RTTHREAD hThreadSelf, void *pvUser)
{
    LogFlowFuncEnter();

    bool fThreadSignalled = false;

    const PSHCLCONTEXT pCtx    = (PSHCLCONTEXT)pvUser;
    AssertPtr(pCtx);
    const PSHCLWINCTX  pWinCtx = &pCtx->Win;

    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

    /* Register the Window Class. */
    WNDCLASS wc;
    RT_ZERO(wc);

    wc.style         = CS_NOCLOSE;
    wc.lpfnWndProc   = vboxClipboardSvcWinWndProc;
    wc.hInstance     = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);

    /* Register an unique wnd class name. */
    char szWndClassName[32];
    RTStrPrintf2(szWndClassName, sizeof(szWndClassName),
                 "%s-%RU64", SHCL_WIN_WNDCLASS_NAME, RTThreadGetNative(hThreadSelf));
    wc.lpszClassName = szWndClassName;

    int rc;

    ATOM atomWindowClass = RegisterClass(&wc);
    if (atomWindowClass == 0)
    {
        LogFunc(("Failed to register window class\n"));
        rc = VERR_NOT_SUPPORTED;
    }
    else
    {
        /* Create a window and make it a clipboard viewer. */
        pWinCtx->hWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
                                       szWndClassName, szWndClassName,
                                       WS_POPUPWINDOW,
                                       -200, -200, 100, 100, NULL, NULL, hInstance, pCtx /* lpParam */);
        if (pWinCtx->hWnd == NULL)
        {
            LogFunc(("Failed to create window\n"));
            rc = VERR_NOT_SUPPORTED;
        }
        else
        {
            SetWindowPos(pWinCtx->hWnd, HWND_TOPMOST, -200, -200, 0, 0,
                         SWP_NOACTIVATE | SWP_HIDEWINDOW | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOSIZE);

            rc = SharedClipboardWinChainAdd(&pCtx->Win);
            if (RT_SUCCESS(rc))
            {
                if (!SharedClipboardWinIsNewAPI(&pWinCtx->newAPI))
                    pWinCtx->oldAPI.timerRefresh = SetTimer(pWinCtx->hWnd, 0, 10 * 1000, NULL);
            }

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
            if (RT_SUCCESS(rc))
            {
                HRESULT hr = OleInitialize(NULL);
                if (FAILED(hr))
                {
                    LogRel(("Shared Clipboard: Initializing window thread OLE failed (%Rhrc) -- file transfers unavailable\n", hr));
                    /* Not critical, the rest of the clipboard might work. */
                }
                else
                    LogRel(("Shared Clipboard: Initialized window thread OLE\n"));
            }
#endif
            int rc2 = RTThreadUserSignal(hThreadSelf);
            AssertRC(rc2);

            fThreadSignalled = true;

            MSG msg;
            BOOL msgret = 0;
            while ((msgret = GetMessage(&msg, NULL, 0, 0)) > 0)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            /*
             * Window procedure can return error, * but this is exceptional situation that should be
             * identified in testing.
             */
            Assert(msgret >= 0);
            LogFunc(("Message loop finished. GetMessage returned %d, message id: %d \n", msgret, msg.message));

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
            OleSetClipboard(NULL); /* Make sure to flush the clipboard on destruction. */
            OleUninitialize();
#endif
        }
    }

    pWinCtx->hWnd = NULL;

    if (atomWindowClass != 0)
    {
        UnregisterClass(szWndClassName, hInstance);
        atomWindowClass = 0;
    }

    if (!fThreadSignalled)
    {
        int rc2 = RTThreadUserSignal(hThreadSelf);
        AssertRC(rc2);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Synchronizes the host and the guest clipboard formats by sending all supported host clipboard
 * formats to the guest.
 *
 * @returns VBox status code, VINF_NO_CHANGE if no synchronization was required.
 * @param   pCtx                Clipboard context to synchronize.
 */
static int vboxClipboardSvcWinSyncInternal(PSHCLCONTEXT pCtx)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    int rc;

    if (pCtx->pClient)
    {
        SHCLFORMATS fFormats = 0;
        rc = SharedClipboardWinGetFormats(&pCtx->Win, &fFormats);
        if (   RT_SUCCESS(rc)
            && fFormats != VBOX_SHCL_FMT_NONE /** @todo r=bird: BUGBUG: revisit this. */
            && ShClSvcIsBackendActive())
            rc = ShClSvcHostReportFormats(pCtx->pClient, fFormats);
    }
    else /* If we don't have any client data (yet), bail out. */
        rc = VINF_NO_CHANGE;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/*
 * Public platform dependent functions.
 */

int ShClBackendInit(PSHCLBACKEND pBackend, VBOXHGCMSVCFNTABLE *pTable)
{
    RT_NOREF(pBackend, pTable);
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    HRESULT hr = OleInitialize(NULL);
    if (FAILED(hr))
    {
        LogRel(("Shared Clipboard: Initializing OLE failed (%Rhrc) -- file transfers unavailable\n", hr));
        /* Not critical, the rest of the clipboard might work. */
    }
    else
        LogRel(("Shared Clipboard: Initialized OLE\n"));
#endif

    return VINF_SUCCESS;
}

void ShClBackendDestroy(PSHCLBACKEND pBackend)
{
    RT_NOREF(pBackend);

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    OleSetClipboard(NULL); /* Make sure to flush the clipboard on destruction. */
    OleUninitialize();
#endif
}

int ShClBackendConnect(PSHCLBACKEND pBackend, PSHCLCLIENT pClient, bool fHeadless)
{
    RT_NOREF(pBackend, fHeadless);

    LogFlowFuncEnter();

    int rc;

    PSHCLCONTEXT pCtx = (PSHCLCONTEXT)RTMemAllocZ(sizeof(SHCLCONTEXT));
    if (pCtx)
    {
        rc = SharedClipboardWinCtxInit(&pCtx->Win);
        if (RT_SUCCESS(rc))
        {
            rc = RTThreadCreate(&pCtx->hThread, vboxClipboardSvcWinThread, pCtx /* pvUser */, _64K /* Stack size */,
                                RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE, "SHCLIP");
            if (RT_SUCCESS(rc))
            {
                int rc2 = RTThreadUserWait(pCtx->hThread, 30 * 1000 /* Timeout in ms */);
                AssertRC(rc2);
            }
        }

        pClient->State.pCtx = pCtx;
        pClient->State.pCtx->pClient = pClient;
    }
    else
        rc = VERR_NO_MEMORY;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int ShClBackendSync(PSHCLBACKEND pBackend, PSHCLCLIENT pClient)
{
    RT_NOREF(pBackend);

    /* Sync the host clipboard content with the client. */
    return vboxClipboardSvcWinSyncInternal(pClient->State.pCtx);
}

int ShClBackendDisconnect(PSHCLBACKEND pBackend, PSHCLCLIENT pClient)
{
    RT_NOREF(pBackend);

    AssertPtrReturn(pClient, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    int rc = VINF_SUCCESS;

    PSHCLCONTEXT pCtx = pClient->State.pCtx;
    if (pCtx)
    {
        if (pCtx->Win.hWnd)
            PostMessage(pCtx->Win.hWnd, WM_DESTROY, 0 /* wParam */, 0 /* lParam */);

        if (pCtx->hThread != NIL_RTTHREAD)
        {
            LogFunc(("Waiting for thread to terminate ...\n"));

            /* Wait for the window thread to terminate. */
            rc = RTThreadWait(pCtx->hThread, 30 * 1000 /* Timeout in ms */, NULL);
            if (RT_FAILURE(rc))
                LogRel(("Shared Clipboard: Waiting for window thread termination failed with rc=%Rrc\n", rc));

            pCtx->hThread = NIL_RTTHREAD;
        }

        SharedClipboardWinCtxDestroy(&pCtx->Win);

        if (RT_SUCCESS(rc))
        {
            RTMemFree(pCtx);
            pCtx = NULL;

            pClient->State.pCtx = NULL;
        }
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int ShClBackendReportFormats(PSHCLBACKEND pBackend, PSHCLCLIENT pClient, SHCLFORMATS fFormats)
{
    RT_NOREF(pBackend);

    AssertPtrReturn(pClient, VERR_INVALID_POINTER);

    PSHCLCONTEXT pCtx = pClient->State.pCtx;
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    LogFlowFunc(("fFormats=0x%x, hWnd=%p\n", fFormats, pCtx->Win.hWnd));

    /*
     * The guest announced formats. Forward to the window thread.
     */
    PostMessage(pCtx->Win.hWnd, SHCL_WIN_WM_REPORT_FORMATS,
                0 /* wParam */, fFormats /* lParam */);


    LogFlowFuncLeaveRC(VINF_SUCCESS);
    return VINF_SUCCESS;
}

int ShClBackendReadData(PSHCLBACKEND pBackend, PSHCLCLIENT pClient, PSHCLCLIENTCMDCTX pCmdCtx,
                        SHCLFORMAT uFmt, void *pvData, uint32_t cbData, uint32_t *pcbActual)
{
    AssertPtrReturn(pClient,   VERR_INVALID_POINTER);
    AssertPtrReturn(pCmdCtx,   VERR_INVALID_POINTER);
    AssertPtrReturn(pvData,    VERR_INVALID_POINTER);
    AssertPtrReturn(pcbActual, VERR_INVALID_POINTER);

    RT_NOREF(pBackend, pCmdCtx);

    AssertPtrReturn(pClient->State.pCtx, VERR_INVALID_POINTER);

    LogFlowFunc(("uFmt=%#x\n", uFmt));

    HANDLE hClip = NULL;

    const PSHCLWINCTX pWinCtx = &pClient->State.pCtx->Win;

    /*
     * The guest wants to read data in the given format.
     */
    int rc = SharedClipboardWinOpen(pWinCtx->hWnd);
    if (RT_SUCCESS(rc))
    {
        if (uFmt & VBOX_SHCL_FMT_BITMAP)
        {
            LogFunc(("CF_DIB\n"));
            hClip = GetClipboardData(CF_DIB);
            if (hClip != NULL)
            {
                LPVOID lp = GlobalLock(hClip);
                if (lp != NULL)
                {
                    rc = vboxClipboardSvcWinDataGet(VBOX_SHCL_FMT_BITMAP, lp, GlobalSize(hClip),
                                                    pvData, cbData, pcbActual);
                    GlobalUnlock(hClip);
                }
                else
                {
                    hClip = NULL;
                }
            }
        }
        else if (uFmt & VBOX_SHCL_FMT_UNICODETEXT)
        {
            LogFunc(("CF_UNICODETEXT\n"));
            hClip = GetClipboardData(CF_UNICODETEXT);
            if (hClip != NULL)
            {
                LPWSTR uniString = (LPWSTR)GlobalLock(hClip);
                if (uniString != NULL)
                {
                    rc = vboxClipboardSvcWinDataGet(VBOX_SHCL_FMT_UNICODETEXT, uniString, (lstrlenW(uniString) + 1) * 2,
                                                    pvData, cbData, pcbActual);
                    GlobalUnlock(hClip);
                }
                else
                {
                    hClip = NULL;
                }
            }
        }
        else if (uFmt & VBOX_SHCL_FMT_HTML)
        {
            LogFunc(("SHCL_WIN_REGFMT_HTML\n"));
            UINT uRegFmt = RegisterClipboardFormat(SHCL_WIN_REGFMT_HTML);
            if (uRegFmt != 0)
            {
                hClip = GetClipboardData(uRegFmt);
                if (hClip != NULL)
                {
                    LPVOID lp = GlobalLock(hClip);
                    if (lp != NULL)
                    {
                        rc = vboxClipboardSvcWinDataGet(VBOX_SHCL_FMT_HTML, lp, GlobalSize(hClip),
                                                        pvData, cbData, pcbActual);
#ifdef LOG_ENABLED
                        if (RT_SUCCESS(rc))
                        {
                            LogFlowFunc(("Raw HTML clipboard data from host:\n"));
                            ShClDbgDumpHtml((char *)pvData, cbData);
                        }
#endif
                        GlobalUnlock(hClip);
                    }
                    else
                    {
                        hClip = NULL;
                    }
                }
            }
        }
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
        else if (uFmt & VBOX_SHCL_FMT_URI_LIST)
        {
            AssertFailed(); /** @todo */
        }
#endif /* VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS */
        SharedClipboardWinClose();
    }

    if (hClip == NULL) /* Empty data is not fatal. */
    {
        /* Reply with empty data. */
        vboxClipboardSvcWinDataGet(0, NULL, 0, pvData, cbData, pcbActual);
    }

    if (RT_FAILURE(rc))
        LogRel(("Shared Clipboard: Error reading host clipboard data in format %#x from Windows, rc=%Rrc\n", uFmt, rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int ShClBackendWriteData(PSHCLBACKEND pBackend, PSHCLCLIENT pClient, PSHCLCLIENTCMDCTX pCmdCtx,
                         SHCLFORMAT uFormat, void *pvData, uint32_t cbData)
{
    RT_NOREF(pBackend, pClient, pCmdCtx, uFormat, pvData, cbData);

    LogFlowFuncEnter();

    /* Nothing to do here yet. */

    LogFlowFuncLeave();
    return VINF_SUCCESS;
}

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
int ShClBackendTransferCreate(PSHCLBACKEND pBackend, PSHCLCLIENT pClient, PSHCLTRANSFER pTransfer)
{
    RT_NOREF(pBackend, pClient, pTransfer);

    LogFlowFuncEnter();

    return VINF_SUCCESS;
}

int ShClBackendTransferDestroy(PSHCLBACKEND pBackend, PSHCLCLIENT pClient, PSHCLTRANSFER pTransfer)
{
    RT_NOREF(pBackend);

    LogFlowFuncEnter();

    SharedClipboardWinTransferDestroy(&pClient->State.pCtx->Win, pTransfer);

    return VINF_SUCCESS;
}

int ShClBackendTransferGetRoots(PSHCLBACKEND pBackend, PSHCLCLIENT pClient, PSHCLTRANSFER pTransfer)
{
    RT_NOREF(pBackend);

    LogFlowFuncEnter();

    const PSHCLWINCTX pWinCtx = &pClient->State.pCtx->Win;

    int rc = SharedClipboardWinGetRoots(pWinCtx, pTransfer);

    LogFlowFuncLeaveRC(rc);
    return rc;
}
#endif /* VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS */

