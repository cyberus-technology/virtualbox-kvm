/* $Id: VBoxClipboard.cpp $ */
/** @file
 * VBoxClipboard - Shared clipboard, Windows Guest Implementation.
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
#include <VBox/log.h>

#include "VBoxTray.h"
#include "VBoxHelpers.h"

#include <iprt/asm.h>
#include <iprt/errcore.h>
#include <iprt/ldr.h>
#include <iprt/mem.h>
#include <iprt/utf16.h>

#include <VBox/GuestHost/SharedClipboard.h>
#include <VBox/GuestHost/SharedClipboard-win.h>
#include <VBox/GuestHost/clipboard-helper.h>
#include <VBox/HostServices/VBoxClipboardSvc.h> /* Temp, remove. */
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
# include <VBox/GuestHost/SharedClipboard-transfers.h>
#endif

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
# include <iprt/win/shlobj.h>
# include <iprt/win/shlwapi.h>
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
struct SHCLCONTEXT
{
    /** Pointer to the VBoxClient service environment. */
    const VBOXSERVICEENV *pEnv;
    /** Command context. */
    VBGLR3SHCLCMDCTX      CmdCtx;
    /** Windows-specific context data. */
    SHCLWINCTX            Win;
    /** Thread handle for window thread. */
    RTTHREAD              hThread;
    /** Start indicator flag. */
    bool                  fStarted;
    /** Shutdown indicator flag. */
    bool                  fShutdown;
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    /** Associated transfer data. */
    SHCLTRANSFERCTX       TransferCtx;
#endif
};


/*********************************************************************************************************************************
*   Static variables                                                                                                             *
*********************************************************************************************************************************/
/** Static clipboard context (since it is the single instance). Directly used in the windows proc. */
static SHCLCONTEXT g_Ctx = { NULL };
/** Static window class name. */
static char s_szClipWndClassName[] = SHCL_WIN_WNDCLASS_NAME;


/*********************************************************************************************************************************
*   Prototypes                                                                                                                   *
*********************************************************************************************************************************/
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
static DECLCALLBACK(int)  vboxClipboardOnTransferInitCallback(PSHCLTXPROVIDERCTX pCtx);
static DECLCALLBACK(int)  vboxClipboardOnTransferStartCallback(PSHCLTXPROVIDERCTX pCtx);
static DECLCALLBACK(void) vboxClipboardOnTransferCompleteCallback(PSHCLTXPROVIDERCTX pCtx, int rc);
static DECLCALLBACK(void) vboxClipboardOnTransferErrorCallback(PSHCLTXPROVIDERCTX pCtx, int rc);
#endif


#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS

/**
 * Cleanup helper function for transfer callbacks.
 *
 * @param   pTransferCtx        Pointer to transfer context that the transfer contains.
 * @param   pTransfer           Pointer to transfer to cleanup.
 */
static void vboxClipboardTransferCallbackCleanup(PSHCLTRANSFERCTX pTransferCtx, PSHCLTRANSFER pTransfer)
{
    LogFlowFuncEnter();

    if (!pTransferCtx || !pTransfer)
        return;

    if (pTransfer->pvUser) /* SharedClipboardWinTransferCtx */
    {
        delete pTransfer->pvUser;
        pTransfer->pvUser = NULL;
    }

    int rc2 = ShClTransferCtxTransferUnregister(pTransferCtx, pTransfer->State.uID);
    AssertRC(rc2);

    ShClTransferDestroy(pTransfer);

    RTMemFree(pTransfer);
    pTransfer = NULL;
}

/** @copydoc SHCLTRANSFERCALLBACKTABLE::pfnOnInitialize */
static DECLCALLBACK(int) vboxClipboardOnTransferInitCallback(PSHCLTRANSFERCALLBACKCTX pCbCtx)
{
    PSHCLCONTEXT pCtx = (PSHCLCONTEXT)pCbCtx->pvUser;
    AssertPtr(pCtx);

    LogFlowFunc(("pCtx=%p\n", pCtx));

    RT_NOREF(pCtx);

    return VINF_SUCCESS;
}

/** @copydoc SHCLTRANSFERCALLBACKTABLE::pfnOnStart */
static DECLCALLBACK(int) vboxClipboardOnTransferStartCallback(PSHCLTRANSFERCALLBACKCTX pCbCtx)
{
    PSHCLCONTEXT pCtx = (PSHCLCONTEXT)pCbCtx->pvUser;
    AssertPtr(pCtx);

    PSHCLTRANSFER pTransfer = pCbCtx->pTransfer;
    AssertPtr(pTransfer);

    const SHCLTRANSFERDIR enmDir = ShClTransferGetDir(pTransfer);

    LogFlowFunc(("pCtx=%p, idTransfer=%RU32, enmDir=%RU32\n", pCtx, ShClTransferGetID(pTransfer), enmDir));

    int rc;

    /* The guest wants to write local data to the host? */
    if (enmDir == SHCLTRANSFERDIR_TO_REMOTE)
    {
        rc = SharedClipboardWinGetRoots(&pCtx->Win, pTransfer);
    }
    /* The guest wants to read data from a remote source. */
    else if (enmDir == SHCLTRANSFERDIR_FROM_REMOTE)
    {
        /* The IDataObject *must* be created on the same thread as our (proxy) window, so post a message to it
         * to do the stuff for us. */
        PSHCLEVENT pEvent;
        rc = ShClEventSourceGenerateAndRegisterEvent(&pTransfer->Events, &pEvent);
        if (RT_SUCCESS(rc))
        {
            /* Don't want to rely on SendMessage (synchronous) here, so just post and wait the event getting signalled. */
            ::PostMessage(pCtx->Win.hWnd, SHCL_WIN_WM_TRANSFER_START, (WPARAM)pTransfer, (LPARAM)pEvent->idEvent);

            PSHCLEVENTPAYLOAD pPayload;
            rc = ShClEventWait(pEvent, 30 * 1000 /* Timeout in ms */, &pPayload);
            if (RT_SUCCESS(rc))
            {
                Assert(pPayload->cbData == sizeof(int));
                rc = *(int *)pPayload->pvData;

                ShClPayloadFree(pPayload);
            }

            ShClEventRelease(pEvent);
        }
        else
            AssertFailedStmt(rc = VERR_SHCLPB_MAX_EVENTS_REACHED);
    }
    else
        AssertFailedStmt(rc = VERR_NOT_SUPPORTED);

    if (RT_FAILURE(rc))
        LogRel(("Shared Clipboard: Starting transfer failed, rc=%Rrc\n", rc));

    LogFlowFunc(("LEAVE: idTransfer=%RU32, rc=%Rrc\n", ShClTransferGetID(pTransfer), rc));
    return rc;
}

/** @copydoc SHCLTRANSFERCALLBACKTABLE::pfnOnCompleted */
static DECLCALLBACK(void) vboxClipboardOnTransferCompletedCallback(PSHCLTRANSFERCALLBACKCTX pCbCtx, int rcCompletion)
{
    PSHCLCONTEXT pCtx = (PSHCLCONTEXT)pCbCtx->pvUser;
    AssertPtr(pCtx);

    LogRel2(("Shared Clipboard: Transfer to destination %s\n",
             rcCompletion == VERR_CANCELLED ? "canceled" : "complete"));

    vboxClipboardTransferCallbackCleanup(&pCtx->TransferCtx, pCbCtx->pTransfer);
}

/** @copydoc SHCLTRANSFERCALLBACKTABLE::pfnOnError */
static DECLCALLBACK(void) vboxClipboardOnTransferErrorCallback(PSHCLTRANSFERCALLBACKCTX pCbCtx, int rcError)
{
    PSHCLCONTEXT pCtx = (PSHCLCONTEXT)pCbCtx->pvUser;
    AssertPtr(pCtx);

    LogRel(("Shared Clipboard: Transfer to destination failed with %Rrc\n", rcError));

    vboxClipboardTransferCallbackCleanup(&pCtx->TransferCtx, pCbCtx->pTransfer);
}

#endif /* VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS */

static LRESULT vboxClipboardWinProcessMsg(PSHCLCONTEXT pCtx, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    AssertPtr(pCtx);

    const PSHCLWINCTX pWinCtx = &pCtx->Win;

    LRESULT lresultRc = 0;

    switch (msg)
    {
        case WM_CLIPBOARDUPDATE:
        {
            LogFunc(("WM_CLIPBOARDUPDATE: pWinCtx=%p\n", pWinCtx));

            if (pCtx->fShutdown) /* If we're about to shut down, skip handling stuff here. */
                break;

            int rc = RTCritSectEnter(&pWinCtx->CritSect);
            if (RT_SUCCESS(rc))
            {
                const HWND hWndClipboardOwner = GetClipboardOwner();

                LogFunc(("WM_CLIPBOARDUPDATE: hWndOldClipboardOwner=%p, hWndNewClipboardOwner=%p\n",
                         pWinCtx->hWndClipboardOwnerUs, hWndClipboardOwner));

                if (pWinCtx->hWndClipboardOwnerUs != hWndClipboardOwner)
                {
                    int rc2 = RTCritSectLeave(&pWinCtx->CritSect);
                    AssertRC(rc2);

                    /* Clipboard was updated by another application.
                     * Report available formats to the host. */
                    SHCLFORMATS fFormats;
                    rc = SharedClipboardWinGetFormats(pWinCtx, &fFormats);
                    if (RT_SUCCESS(rc))
                    {
                        LogFunc(("WM_CLIPBOARDUPDATE: Reporting formats %#x\n", fFormats));
                        rc = VbglR3ClipboardReportFormats(pCtx->CmdCtx.idClient, fFormats);
                    }
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
            lresultRc = SharedClipboardWinHandleWMChangeCBChain(pWinCtx, hwnd, msg, wParam, lParam);
            break;
        }

        case WM_DRAWCLIPBOARD:
        {
            LogFlowFunc(("WM_DRAWCLIPBOARD: pWinCtx=%p\n", pWinCtx));

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

                    /* Clipboard was updated by another application. */
                    /* WM_DRAWCLIPBOARD always expects a return code of 0, so don't change "rc" here. */
                    SHCLFORMATS fFormats;
                    rc = SharedClipboardWinGetFormats(pWinCtx, &fFormats);
                    if (   RT_SUCCESS(rc)
                        && fFormats != VBOX_SHCL_FMT_NONE)
                        rc = VbglR3ClipboardReportFormats(pCtx->CmdCtx.idClient, fFormats);
                }
                else
                {
                    int rc2 = RTCritSectLeave(&pWinCtx->CritSect);
                    AssertRC(rc2);
                }
            }

            lresultRc = SharedClipboardWinChainPassToNext(pWinCtx, msg, wParam, lParam);
            break;
        }

        case WM_TIMER:
        {
            int rc = SharedClipboardWinHandleWMTimer(pWinCtx);
            AssertRC(rc);

            break;
        }

        case WM_CLOSE:
        {
            /* Do nothing. Ignore the message. */
            break;
        }

        case WM_RENDERFORMAT:
        {
            LogFunc(("WM_RENDERFORMAT\n"));

            /* Insert the requested clipboard format data into the clipboard. */
            const UINT cfFormat = (UINT)wParam;

            const SHCLFORMAT fFormat = SharedClipboardWinClipboardFormatToVBox(cfFormat);

            LogFunc(("WM_RENDERFORMAT: cfFormat=%u -> fFormat=0x%x\n", cfFormat, fFormat));

            if (fFormat == VBOX_SHCL_FMT_NONE)
            {
                LogFunc(("WM_RENDERFORMAT: Unsupported format requested\n"));
                SharedClipboardWinClear();
            }
            else
            {
                uint32_t const cbPrealloc = _4K;
                uint32_t       cb         = 0;

                /* Preallocate a buffer, most of small text transfers will fit into it. */
                HANDLE hMem = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, cbPrealloc);
                if (hMem)
                {
                    void *pvMem = GlobalLock(hMem);
                    if (pvMem)
                    {
                        /* Read the host data to the preallocated buffer. */
                        int rc = VbglR3ClipboardReadDataEx(&pCtx->CmdCtx, fFormat, pvMem, cbPrealloc, &cb);
                        if (RT_SUCCESS(rc))
                        {
                            if (cb == 0)
                            {
                                /* 0 bytes returned means the clipboard is empty.
                                 * Deallocate the memory and set hMem to NULL to get to
                                 * the clipboard empty code path. */
                                GlobalUnlock(hMem);
                                GlobalFree(hMem);
                                hMem = NULL;
                            }
                            else if (cb > cbPrealloc)
                            {
                                GlobalUnlock(hMem);

                                LogRel2(("Shared Clipboard: Buffer too small (%RU32), needs %RU32 bytes\n", cbPrealloc, cb));

                                /* The preallocated buffer is too small, adjust the size. */
                                hMem = GlobalReAlloc(hMem, cb, 0);
                                if (hMem)
                                {
                                    pvMem = GlobalLock(hMem);
                                    if (pvMem)
                                    {
                                        /* Read the host data to the preallocated buffer. */
                                        uint32_t cbNew = 0;
                                        rc = VbglR3ClipboardReadDataEx(&pCtx->CmdCtx, fFormat, pvMem, cb, &cbNew);
                                        if (   RT_SUCCESS(rc)
                                            && cbNew <= cb)
                                        {
                                            cb = cbNew;
                                        }
                                        else
                                        {
                                            LogRel(("Shared Clipboard: Receiving host data failed with %Rrc\n", rc));

                                            GlobalUnlock(hMem);
                                            GlobalFree(hMem);
                                            hMem = NULL;
                                        }
                                    }
                                    else
                                    {
                                        LogRel(("Shared Clipboard: Error locking reallocated host data buffer\n"));

                                        GlobalFree(hMem);
                                        hMem = NULL;
                                    }
                                }
                                else
                                    LogRel(("Shared Clipboard: No memory for reallocating host data buffer\n"));
                            }

                            if (hMem)
                            {
                                /* pvMem is the address of the data. cb is the size of returned data. */
                                /* Verify the size of returned text, the memory block for clipboard
                                 * must have the exact string size.
                                 */
                                if (fFormat == VBOX_SHCL_FMT_UNICODETEXT)
                                {
                                    size_t cwcActual = 0;
                                    rc = RTUtf16NLenEx((PCRTUTF16)pvMem, cb / sizeof(RTUTF16), &cwcActual);
                                    if (RT_SUCCESS(rc))
                                        cb = (uint32_t)((cwcActual + 1 /* '\0' */) * sizeof(RTUTF16));
                                    else
                                    {
                                        LogRel(("Shared Clipboard: Invalid UTF16 string from host: cb=%RU32, cwcActual=%zu, rc=%Rrc\n",
                                                cb, cwcActual, rc));

                                        /* Discard invalid data. */
                                        GlobalUnlock(hMem);
                                        GlobalFree(hMem);
                                        hMem = NULL;
                                    }
                                }
                                else if (fFormat == VBOX_SHCL_FMT_HTML)
                                {
                                    /* Wrap content into CF_HTML clipboard format if needed. */
                                    if (!SharedClipboardWinIsCFHTML((const char *)pvMem))
                                    {
                                        char *pszWrapped = NULL;
                                        uint32_t cbWrapped = 0;
                                        rc = SharedClipboardWinConvertMIMEToCFHTML((const char *)pvMem, cb,
                                                                                   &pszWrapped, &cbWrapped);
                                        if (RT_SUCCESS(rc))
                                        {
                                            if (GlobalUnlock(hMem) == 0)
                                            {
                                                hMem = GlobalReAlloc(hMem, cbWrapped, 0);
                                                if (hMem)
                                                {
                                                    pvMem = GlobalLock(hMem);
                                                    if (pvMem)
                                                    {
                                                        /* Copy wrapped content back to memory passed to system clipboard. */
                                                        memcpy(pvMem, pszWrapped, cbWrapped);
                                                        cb = cbWrapped;
                                                    }
                                                    else
                                                    {
                                                        LogRel(("Shared Clipboard: Failed to lock memory (%u), HTML clipboard data won't be converted into CF_HTML clipboard format\n", GetLastError()));
                                                        GlobalFree(hMem);
                                                        hMem = NULL;
                                                    }
                                                }
                                                else
                                                    LogRel(("Shared Clipboard: Failed to re-allocate memory (%u), HTML clipboard data won't be converted into CF_HTML clipboard format\n", GetLastError()));
                                            }
                                            else
                                                LogRel(("Shared Clipboard: Failed to unlock memory (%u), HTML clipboard data won't be converted into CF_HTML clipboard format\n", GetLastError()));

                                            RTMemFree(pszWrapped);
                                        }
                                        else
                                            LogRel(("Shared Clipboard: Cannot convert HTML clipboard data into CF_HTML clipboard format, rc=%Rrc\n", rc));
                                    }
                                }
                            }

                            if (hMem)
                            {
                                GlobalUnlock(hMem);

                                hMem = GlobalReAlloc(hMem, cb, 0);
                                if (hMem)
                                {
                                    /* 'hMem' contains the host clipboard data.
                                     * size is 'cb' and format is 'format'. */
                                    HANDLE hClip = SetClipboardData(cfFormat, hMem);
                                    if (hClip)
                                    {
                                        /* The hMem ownership has gone to the system. Finish the processing. */
                                        break;
                                    }
                                    else
                                        LogRel(("Shared Clipboard: Setting host data buffer to clipboard failed with %ld\n",
                                                GetLastError()));

                                    /* Cleanup follows. */
                                }
                                else
                                    LogRel(("Shared Clipboard: No memory for allocating final host data buffer\n"));
                            }
                        }

                        if (hMem)
                            GlobalUnlock(hMem);
                    }
                    else
                        LogRel(("Shared Clipboard: No memory for allocating host data buffer\n"));

                    if (hMem)
                        GlobalFree(hMem);
                }
            }

            break;
        }

        case WM_RENDERALLFORMATS:
        {
            LogFunc(("WM_RENDERALLFORMATS\n"));

            int rc = SharedClipboardWinHandleWMRenderAllFormats(pWinCtx, hwnd);
            AssertRC(rc);

            break;
        }

        case SHCL_WIN_WM_REPORT_FORMATS:
        {
            LogFunc(("SHCL_WIN_WM_REPORT_FORMATS\n"));

            /* Announce available formats. Do not insert data -- will be inserted in WM_RENDERFORMAT. */
            PVBGLR3CLIPBOARDEVENT pEvent = (PVBGLR3CLIPBOARDEVENT)lParam;
            AssertPtr(pEvent);
            Assert(pEvent->enmType == VBGLR3CLIPBOARDEVENTTYPE_REPORT_FORMATS);

            const SHCLFORMATS fFormats = pEvent->u.fReportedFormats;

            if (fFormats != VBOX_SHCL_FMT_NONE) /* Could arrive with some older GA versions. */
            {
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
                if (fFormats & VBOX_SHCL_FMT_URI_LIST)
                {
                    LogFunc(("VBOX_SHCL_FMT_URI_LIST\n"));

                    /*
                     * Creating and starting the actual transfer will be done in vbglR3ClipboardTransferStart() as soon
                     * as the host announces the start of the transfer via a VBOX_SHCL_HOST_MSG_TRANSFER_STATUS message.
                     * Transfers always are controlled and initiated on the host side!
                     *
                     * So don't announce the transfer to the OS here yet. Don't touch the clipboard in any here; otherwise
                     * this will trigger a WM_DRAWCLIPBOARD or friends, which will result in fun bugs coming up.
                     */
                }
                else
                {
#endif
                    SharedClipboardWinClearAndAnnounceFormats(pWinCtx, fFormats, hwnd);
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
                }
#endif
            }

            LogFunc(("SHCL_WIN_WM_REPORT_FORMATS: fFormats=0x%x, lastErr=%ld\n", fFormats, GetLastError()));
            break;
        }

        case SHCL_WIN_WM_READ_DATA:
        {
            /* Send data in the specified format to the host. */
            PVBGLR3CLIPBOARDEVENT pEvent = (PVBGLR3CLIPBOARDEVENT)lParam;
            AssertPtr(pEvent);
            Assert(pEvent->enmType == VBGLR3CLIPBOARDEVENTTYPE_READ_DATA);

            const SHCLFORMAT fFormat = (uint32_t)pEvent->u.fReadData;

            LogFlowFunc(("SHCL_WIN_WM_READ_DATA: fFormat=%#x\n", fFormat));

            int rc = SharedClipboardWinOpen(hwnd);
            HANDLE hClip = NULL;
            if (RT_SUCCESS(rc))
            {
                if (fFormat & VBOX_SHCL_FMT_BITMAP)
                {
                    hClip = GetClipboardData(CF_DIB);
                    if (hClip != NULL)
                    {
                        LPVOID lp = GlobalLock(hClip);
                        if (lp != NULL)
                        {
                            rc = VbglR3ClipboardWriteDataEx(&pEvent->cmdCtx, fFormat, lp, (uint32_t)GlobalSize(hClip));

                            GlobalUnlock(hClip);
                        }
                        else
                            hClip = NULL;
                    }
                }
                else if (fFormat & VBOX_SHCL_FMT_UNICODETEXT)
                {
                    hClip = GetClipboardData(CF_UNICODETEXT);
                    if (hClip != NULL)
                    {
                        LPWSTR uniString = (LPWSTR)GlobalLock(hClip);
                        if (uniString != NULL)
                        {
                            rc = VbglR3ClipboardWriteDataEx(&pEvent->cmdCtx,
                                                            fFormat, uniString, ((uint32_t)lstrlenW(uniString) + 1) * 2);

                            GlobalUnlock(hClip);
                        }
                        else
                            hClip = NULL;
                    }
                }
                else if (fFormat & VBOX_SHCL_FMT_HTML)
                {
                    UINT format = RegisterClipboardFormat(SHCL_WIN_REGFMT_HTML);
                    if (format != 0)
                    {
                        hClip = GetClipboardData(format);
                        if (hClip != NULL)
                        {
                            LPVOID const pvClip = GlobalLock(hClip);
                            if (pvClip != NULL)
                            {
                                uint32_t const cbClip = (uint32_t)GlobalSize(hClip);

                                /* Unwrap clipboard content from CF_HTML format if needed. */
                                if (SharedClipboardWinIsCFHTML((const char *)pvClip))
                                {
                                    char        *pszBuf = NULL;
                                    uint32_t    cbBuf   = 0;
                                    rc = SharedClipboardWinConvertCFHTMLToMIME((const char *)pvClip, cbClip, &pszBuf, &cbBuf);
                                    if (RT_SUCCESS(rc))
                                    {
                                        rc = VbglR3ClipboardWriteDataEx(&pEvent->cmdCtx, fFormat, pszBuf, cbBuf);
                                        RTMemFree(pszBuf);
                                    }
                                    else
                                        rc = VbglR3ClipboardWriteDataEx(&pEvent->cmdCtx, fFormat, pvClip, cbClip);
                                }
                                else
                                    rc = VbglR3ClipboardWriteDataEx(&pEvent->cmdCtx, fFormat, pvClip, cbClip);

                                GlobalUnlock(hClip);
                            }
                            else
                                hClip = NULL;
                        }
                    }
                }

                if (hClip == NULL)
                    LogFunc(("SHCL_WIN_WM_READ_DATA: hClip=NULL, lastError=%ld\n", GetLastError()));

                SharedClipboardWinClose();
            }

            /* If the requested clipboard format is not available, we must send empty data. */
            if (hClip == NULL)
                VbglR3ClipboardWriteDataEx(&pEvent->cmdCtx, VBOX_SHCL_FMT_NONE, NULL, 0);
            break;
        }

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
        case SHCL_WIN_WM_TRANSFER_START:
        {
            LogFunc(("SHCL_WIN_WM_TRANSFER_START\n"));

            PSHCLTRANSFER pTransfer  = (PSHCLTRANSFER)wParam;
            AssertPtr(pTransfer);

            const SHCLEVENTID idEvent = (SHCLEVENTID)lParam;

            Assert(ShClTransferGetSource(pTransfer) == SHCLSOURCE_REMOTE); /* Sanity. */

            int rcTransfer = SharedClipboardWinTransferCreate(pWinCtx, pTransfer);

            PSHCLEVENTPAYLOAD pPayload = NULL;
            int rc = ShClPayloadAlloc(idEvent, &rcTransfer, sizeof(rcTransfer), &pPayload);
            if (RT_SUCCESS(rc))
            {
                rc = ShClEventSignal(&pTransfer->Events, idEvent, pPayload);
                if (RT_FAILURE(rc))
                    ShClPayloadFree(pPayload);
            }

            break;
        }
#endif
        case WM_DESTROY:
        {
            LogFunc(("WM_DESTROY\n"));

            int rc = SharedClipboardWinHandleWMDestroy(pWinCtx);
            AssertRC(rc);

            /*
             * Don't need to call PostQuitMessage cause
             * the VBoxTray already finished a message loop.
             */

            break;
        }

        default:
        {
            LogFunc(("WM_ %p\n", msg));
            lresultRc = DefWindowProc(hwnd, msg, wParam, lParam);
            break;
        }
    }

    LogFunc(("WM_ rc %d\n", lresultRc));
    return lresultRc;
}

static LRESULT CALLBACK vboxClipboardWinWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static int vboxClipboardCreateWindow(PSHCLCONTEXT pCtx)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    AssertPtr(pCtx->pEnv);
    HINSTANCE hInstance = pCtx->pEnv->hInstance;
    Assert(hInstance != 0);

    /* Register the Window Class. */
    WNDCLASSEX wc;
    RT_ZERO(wc);

    wc.cbSize = sizeof(WNDCLASSEX);

    if (!GetClassInfoEx(hInstance, s_szClipWndClassName, &wc))
    {
        wc.style         = CS_NOCLOSE;
        wc.lpfnWndProc   = vboxClipboardWinWndProc;
        wc.hInstance     = pCtx->pEnv->hInstance;
        wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
        wc.lpszClassName = s_szClipWndClassName;

        ATOM wndClass = RegisterClassEx(&wc);
        if (wndClass == 0)
            rc = RTErrConvertFromWin32(GetLastError());
    }

    if (RT_SUCCESS(rc))
    {
        const PSHCLWINCTX pWinCtx = &pCtx->Win;

        /* Create the window. */
        pWinCtx->hWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
                                       s_szClipWndClassName, s_szClipWndClassName,
                                       WS_POPUPWINDOW,
                                       -200, -200, 100, 100, NULL, NULL, hInstance, NULL);
        if (pWinCtx->hWnd == NULL)
        {
            rc = VERR_NOT_SUPPORTED;
        }
        else
        {
            SetWindowPos(pWinCtx->hWnd, HWND_TOPMOST, -200, -200, 0, 0,
                         SWP_NOACTIVATE | SWP_HIDEWINDOW | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOSIZE);

            rc = SharedClipboardWinChainAdd(pWinCtx);
            if (RT_SUCCESS(rc))
            {
                if (!SharedClipboardWinIsNewAPI(&pWinCtx->newAPI))
                    pWinCtx->oldAPI.timerRefresh = SetTimer(pWinCtx->hWnd, 0, 10 * 1000 /* 10s */, NULL);
            }
        }
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static DECLCALLBACK(int) vboxClipboardWindowThread(RTTHREAD hThread, void *pvUser)
{
    PSHCLCONTEXT pCtx = (PSHCLCONTEXT)pvUser;
    AssertPtr(pCtx);

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    HRESULT hr = OleInitialize(NULL);
    if (FAILED(hr))
    {
        LogRel(("Shared Clipboard: Initializing OLE in window thread failed (%Rhrc) -- file transfers unavailable\n", hr));
        /* Not critical, the rest of the clipboard might work. */
    }
    else
        LogRel(("Shared Clipboard: Initialized OLE in window thread\n"));
#endif

    int rc = vboxClipboardCreateWindow(pCtx);
    if (RT_FAILURE(rc))
    {
        LogRel(("Shared Clipboard: Unable to create window, rc=%Rrc\n", rc));
        return rc;
    }

    pCtx->fStarted = true; /* Set started indicator. */

    int rc2 = RTThreadUserSignal(hThread);
    bool fSignalled = RT_SUCCESS(rc2);

    LogRel2(("Shared Clipboard: Window thread running\n"));

    if (RT_SUCCESS(rc))
    {
        for (;;)
        {
            MSG uMsg;
            BOOL fRet;
            while ((fRet = GetMessage(&uMsg, 0, 0, 0)) > 0)
            {
                TranslateMessage(&uMsg);
                DispatchMessage(&uMsg);
            }
            Assert(fRet >= 0);

            if (ASMAtomicReadBool(&pCtx->fShutdown))
                break;

            /** @todo Immediately drop on failure? */
        }
    }

    if (!fSignalled)
    {
        rc2 = RTThreadUserSignal(hThread);
        AssertRC(rc2);
    }

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    OleSetClipboard(NULL); /* Make sure to flush the clipboard on destruction. */
    OleUninitialize();
#endif

    LogRel(("Shared Clipboard: Window thread ended\n"));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static void vboxClipboardDestroy(PSHCLCONTEXT pCtx)
{
    AssertPtrReturnVoid(pCtx);

    LogFlowFunc(("pCtx=%p\n", pCtx));

    LogRel2(("Shared Clipboard: Destroying ...\n"));

    const PSHCLWINCTX pWinCtx = &pCtx->Win;

    if (pCtx->hThread != NIL_RTTHREAD)
    {
        int rcThread = VERR_WRONG_ORDER;
        int rc = RTThreadWait(pCtx->hThread, 60 * 1000 /* Timeout in ms */, &rcThread);
        LogFlowFunc(("Waiting for thread resulted in %Rrc (thread exited with %Rrc)\n",
                     rc, rcThread));
        RT_NOREF(rc);
    }

    if (pWinCtx->hWnd)
    {
        DestroyWindow(pWinCtx->hWnd);
        pWinCtx->hWnd = NULL;
    }

    UnregisterClass(s_szClipWndClassName, pCtx->pEnv->hInstance);

    SharedClipboardWinCtxDestroy(&pCtx->Win);

    LogRel2(("Shared Clipboard: Destroyed\n"));
}

static LRESULT CALLBACK vboxClipboardWinWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PSHCLCONTEXT pCtx = &g_Ctx; /** @todo r=andy Make pCtx available through SetWindowLongPtr() / GWL_USERDATA. */
    AssertPtr(pCtx);

    /* Forward with proper context. */
    return vboxClipboardWinProcessMsg(pCtx, hWnd, uMsg, wParam, lParam);
}

DECLCALLBACK(int) VBoxShClInit(const PVBOXSERVICEENV pEnv, void **ppInstance)
{
    LogFlowFuncEnter();

    PSHCLCONTEXT pCtx = &g_Ctx; /* Only one instance for now. */
    AssertPtr(pCtx);

    if (pCtx->pEnv)
    {
        /* Clipboard was already initialized. 2 or more instances are not supported. */
        return VERR_NOT_SUPPORTED;
    }

    if (VbglR3AutoLogonIsRemoteSession())
    {
        /* Do not use clipboard for remote sessions. */
        LogRel(("Shared Clipboard: Clipboard has been disabled for a remote session\n"));
        return VERR_NOT_SUPPORTED;
    }

    pCtx->pEnv      = pEnv;
    pCtx->hThread   = NIL_RTTHREAD;
    pCtx->fStarted  = false;
    pCtx->fShutdown = false;

    int rc = VINF_SUCCESS;

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    /*
     * Set callbacks.
     * Those will be registered within VbglR3 when a new transfer gets initialized.
     */
    RT_ZERO(pCtx->CmdCtx.Transfers.Callbacks);

    pCtx->CmdCtx.Transfers.Callbacks.pvUser = pCtx; /* Assign context as user-provided callback data. */
    pCtx->CmdCtx.Transfers.Callbacks.cbUser = sizeof(SHCLCONTEXT);

    pCtx->CmdCtx.Transfers.Callbacks.pfnOnInitialize = vboxClipboardOnTransferInitCallback;
    pCtx->CmdCtx.Transfers.Callbacks.pfnOnStart      = vboxClipboardOnTransferStartCallback;
    pCtx->CmdCtx.Transfers.Callbacks.pfnOnCompleted  = vboxClipboardOnTransferCompletedCallback;
    pCtx->CmdCtx.Transfers.Callbacks.pfnOnError      = vboxClipboardOnTransferErrorCallback;
#endif

    if (RT_SUCCESS(rc))
    {
        rc = SharedClipboardWinCtxInit(&pCtx->Win);
        if (RT_SUCCESS(rc))
            rc = VbglR3ClipboardConnectEx(&pCtx->CmdCtx, VBOX_SHCL_GF_0_CONTEXT_ID);
        if (RT_SUCCESS(rc))
        {
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
            rc = ShClTransferCtxInit(&pCtx->TransferCtx);
#endif
            if (RT_SUCCESS(rc))
            {
                /* Message pump thread for our proxy window. */
                rc = RTThreadCreate(&pCtx->hThread, vboxClipboardWindowThread, pCtx /* pvUser */,
                                    0, RTTHREADTYPE_MSG_PUMP, RTTHREADFLAGS_WAITABLE,
                                    "shclwnd");
                if (RT_SUCCESS(rc))
                {
                    int rc2 = RTThreadUserWait(pCtx->hThread, 30 * 1000 /* Timeout in ms */);
                    AssertRC(rc2);

                    if (!pCtx->fStarted) /* Did the thread fail to start? */
                        rc = VERR_NOT_SUPPORTED; /* Report back Shared Clipboard as not being supported. */
                }
            }

            if (RT_SUCCESS(rc))
            {
                *ppInstance = pCtx;
            }
            else
                VbglR3ClipboardDisconnectEx(&pCtx->CmdCtx);
        }
    }

    if (RT_FAILURE(rc))
        LogRel(("Shared Clipboard: Unable to initialize, rc=%Rrc\n", rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

DECLCALLBACK(int) VBoxShClWorker(void *pInstance, bool volatile *pfShutdown)
{
    AssertPtr(pInstance);
    LogFlowFunc(("pInstance=%p\n", pInstance));

    /*
     * Tell the control thread that it can continue
     * spawning services.
     */
    RTThreadUserSignal(RTThreadSelf());

    const PSHCLCONTEXT pCtx = (PSHCLCONTEXT)pInstance;
    AssertPtr(pCtx);

    const PSHCLWINCTX pWinCtx = &pCtx->Win;

    LogRel2(("Shared Clipboard: Worker loop running\n"));

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    HRESULT hr = OleInitialize(NULL);
    if (FAILED(hr))
    {
        LogRel(("Shared Clipboard: Initializing OLE in worker thread failed (%Rhrc) -- file transfers unavailable\n", hr));
        /* Not critical, the rest of the clipboard might work. */
    }
    else
        LogRel(("Shared Clipboard: Initialized OLE in worker thraed\n"));
#endif

    int rc;

    /* The thread waits for incoming messages from the host. */
    for (;;)
    {
        LogFlowFunc(("Waiting for host message (fUseLegacyProtocol=%RTbool, fHostFeatures=%#RX64) ...\n",
                     pCtx->CmdCtx.fUseLegacyProtocol, pCtx->CmdCtx.fHostFeatures));

        PVBGLR3CLIPBOARDEVENT pEvent = (PVBGLR3CLIPBOARDEVENT)RTMemAllocZ(sizeof(VBGLR3CLIPBOARDEVENT));
        AssertPtrBreakStmt(pEvent, rc = VERR_NO_MEMORY);

        uint32_t idMsg  = 0;
        uint32_t cParms = 0;
        rc = VbglR3ClipboardMsgPeekWait(&pCtx->CmdCtx, &idMsg, &cParms, NULL /* pidRestoreCheck */);
        if (RT_SUCCESS(rc))
        {
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
            rc = VbglR3ClipboardEventGetNextEx(idMsg, cParms, &pCtx->CmdCtx, &pCtx->TransferCtx, pEvent);
#else
            rc = VbglR3ClipboardEventGetNext(idMsg, cParms, &pCtx->CmdCtx, pEvent);
#endif
        }

        if (RT_FAILURE(rc))
        {
            LogFlowFunc(("Getting next event failed with %Rrc\n", rc));

            VbglR3ClipboardEventFree(pEvent);
            pEvent = NULL;

            if (*pfShutdown)
                break;

            /* Wait a bit before retrying. */
            RTThreadSleep(1000);
            continue;
        }
        else
        {
            AssertPtr(pEvent);
            LogFlowFunc(("Event uType=%RU32\n", pEvent->enmType));

            switch (pEvent->enmType)
            {
                case VBGLR3CLIPBOARDEVENTTYPE_REPORT_FORMATS:
                {
                    /* The host has announced available clipboard formats.
                     * Forward the information to the window, so it can later
                     * respond to WM_RENDERFORMAT message. */
                    ::PostMessage(pWinCtx->hWnd, SHCL_WIN_WM_REPORT_FORMATS,
                                  0 /* wParam */, (LPARAM)pEvent /* lParam */);

                    pEvent = NULL; /* Consume pointer. */
                    break;
                }

                case VBGLR3CLIPBOARDEVENTTYPE_READ_DATA:
                {
                    /* The host needs data in the specified format. */
                    ::PostMessage(pWinCtx->hWnd, SHCL_WIN_WM_READ_DATA,
                                  0 /* wParam */, (LPARAM)pEvent /* lParam */);

                    pEvent = NULL; /* Consume pointer. */
                    break;
                }

                case VBGLR3CLIPBOARDEVENTTYPE_QUIT:
                {
                    LogRel2(("Shared Clipboard: Host requested termination\n"));
                    ASMAtomicXchgBool(pfShutdown, true);
                    break;
                }

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
                case VBGLR3CLIPBOARDEVENTTYPE_TRANSFER_STATUS:
                {
                    /* Nothing to do here. */
                    rc = VINF_SUCCESS;
                    break;
                }
#endif
                case VBGLR3CLIPBOARDEVENTTYPE_NONE:
                {
                    /* Nothing to do here. */
                    rc = VINF_SUCCESS;
                    break;
                }

                default:
                {
                    AssertMsgFailedBreakStmt(("Event type %RU32 not implemented\n", pEvent->enmType), rc = VERR_NOT_SUPPORTED);
                }
            }

            if (pEvent)
            {
                VbglR3ClipboardEventFree(pEvent);
                pEvent = NULL;
            }
        }

        if (*pfShutdown)
            break;
    }

    LogRel2(("Shared Clipboard: Worker loop ended\n"));

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    OleSetClipboard(NULL); /* Make sure to flush the clipboard on destruction. */
    OleUninitialize();
#endif

    LogFlowFuncLeaveRC(rc);
    return rc;
}

DECLCALLBACK(int) VBoxShClStop(void *pInstance)
{
    AssertPtrReturn(pInstance, VERR_INVALID_POINTER);

    LogFunc(("Stopping pInstance=%p\n", pInstance));

    PSHCLCONTEXT pCtx = (PSHCLCONTEXT)pInstance;
    AssertPtr(pCtx);

    /* Set shutdown indicator. */
    ASMAtomicWriteBool(&pCtx->fShutdown, true);

    /* Let our clipboard know that we're going to shut down. */
    PostMessage(pCtx->Win.hWnd, WM_QUIT, 0, 0);

    /* Disconnect from the host service.
     * This will also send a VBOX_SHCL_HOST_MSG_QUIT from the host so that we can break out from our message worker. */
    VbglR3ClipboardDisconnect(pCtx->CmdCtx.idClient);
    pCtx->CmdCtx.idClient = 0;

    LogFlowFuncLeaveRC(VINF_SUCCESS);
    return VINF_SUCCESS;
}

DECLCALLBACK(void) VBoxShClDestroy(void *pInstance)
{
    AssertPtrReturnVoid(pInstance);

    PSHCLCONTEXT pCtx = (PSHCLCONTEXT)pInstance;
    AssertPtr(pCtx);

    /* Make sure that we are disconnected. */
    Assert(pCtx->CmdCtx.idClient == 0);

    vboxClipboardDestroy(pCtx);

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    ShClTransferCtxDestroy(&pCtx->TransferCtx);
#endif

    return;
}

/**
 * The service description.
 */
VBOXSERVICEDESC g_SvcDescClipboard =
{
    /* pszName. */
    "clipboard",
    /* pszDescription. */
    "Shared Clipboard",
    /* methods */
    VBoxShClInit,
    VBoxShClWorker,
    VBoxShClStop,
    VBoxShClDestroy
};
