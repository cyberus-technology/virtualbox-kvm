/** @file
 * Shared Clipboard: Common X11 code.
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

/* Note: to automatically run regression tests on the Shared Clipboard,
 * execute the tstClipboardGH-X11 testcase.  If you often make changes to the
 * clipboard code, adding the line
 *
 *   OTHERS += $(PATH_tstClipboardGH-X11)/tstClipboardGH-X11.run
 *
 * to LocalConfig.kmk will cause the tests to be run every time the code is
 * changed.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_SHARED_CLIPBOARD

#include <errno.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef RT_OS_SOLARIS
#include <tsol/label.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/Xproto.h>
#include <X11/StringDefs.h>

#include <iprt/assert.h>
#include <iprt/types.h>
#include <iprt/mem.h>
#include <iprt/semaphore.h>
#include <iprt/thread.h>
#include <iprt/utf16.h>
#include <iprt/uri.h>

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
# include <iprt/cpp/list.h>
# include <iprt/cpp/ministring.h>
# include <VBox/GuestHost/SharedClipboard-transfers.h>
#endif

#include <VBox/log.h>
#include <VBox/version.h>

#include <VBox/GuestHost/SharedClipboard.h>
#include <VBox/GuestHost/SharedClipboard-x11.h>
#include <VBox/GuestHost/clipboard-helper.h>
#include <VBox/HostServices/VBoxClipboardSvc.h>

/** Own macro for declaring function visibility / linkage based on whether this
 *  code runs as part of test cases or not. */
#ifdef TESTCASE
# define SHCL_X11_DECL(x) x
#else
# define SHCL_X11_DECL(x) static x
#endif


/*********************************************************************************************************************************
*   Externals                                                                                                                    *
*********************************************************************************************************************************/
#ifdef TESTCASE
extern void tstThreadScheduleCall(void (*proc)(void *, void *), void *client_data);
extern void tstClipRequestData(SHCLX11CTX* pCtx, SHCLX11FMTIDX target, void *closure);
extern void tstRequestTargets(SHCLX11CTX* pCtx);
#endif


/*********************************************************************************************************************************
*   Prototypes                                                                                                                   *
*********************************************************************************************************************************/
class formats;
SHCL_X11_DECL(Atom) clipGetAtom(PSHCLX11CTX pCtx, const char *pszName);
SHCL_X11_DECL(void) clipQueryX11Targets(PSHCLX11CTX pCtx);

static int          clipInitInternal(PSHCLX11CTX pCtx);
static void         clipUninitInternal(PSHCLX11CTX pCtx);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/

/**
 * The table maps X11 names to data formats
 * and to the corresponding VBox clipboard formats.
 */
SHCL_X11_DECL(SHCLX11FMTTABLE) g_aFormats[] =
{
    { "INVALID",                            SHCLX11FMT_INVALID,     VBOX_SHCL_FMT_NONE },

    { "UTF8_STRING",                        SHCLX11FMT_UTF8,        VBOX_SHCL_FMT_UNICODETEXT },
    { "text/plain;charset=UTF-8",           SHCLX11FMT_UTF8,        VBOX_SHCL_FMT_UNICODETEXT },
    { "text/plain;charset=utf-8",           SHCLX11FMT_UTF8,        VBOX_SHCL_FMT_UNICODETEXT },
    { "STRING",                             SHCLX11FMT_TEXT,        VBOX_SHCL_FMT_UNICODETEXT },
    { "TEXT",                               SHCLX11FMT_TEXT,        VBOX_SHCL_FMT_UNICODETEXT },
    { "text/plain",                         SHCLX11FMT_TEXT,        VBOX_SHCL_FMT_UNICODETEXT },

    { "text/html",                          SHCLX11FMT_HTML,        VBOX_SHCL_FMT_HTML },
    { "text/html;charset=utf-8",            SHCLX11FMT_HTML,        VBOX_SHCL_FMT_HTML },
    { "application/x-moz-nativehtml",       SHCLX11FMT_HTML,        VBOX_SHCL_FMT_HTML },

    { "image/bmp",                          SHCLX11FMT_BMP,         VBOX_SHCL_FMT_BITMAP },
    { "image/x-bmp",                        SHCLX11FMT_BMP,         VBOX_SHCL_FMT_BITMAP },
    { "image/x-MS-bmp",                     SHCLX11FMT_BMP,         VBOX_SHCL_FMT_BITMAP },
    /** @todo Inkscape exports image/png but not bmp... */

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    { "text/uri-list",                      SHCLX11FMT_URI_LIST,    VBOX_SHCL_FMT_URI_LIST },
    { "x-special/gnome-copied-files",       SHCLX11FMT_URI_LIST,    VBOX_SHCL_FMT_URI_LIST },
    { "x-special/nautilus-clipboard",       SHCLX11FMT_URI_LIST,    VBOX_SHCL_FMT_URI_LIST },
    { "application/x-kde-cutselection",     SHCLX11FMT_URI_LIST,    VBOX_SHCL_FMT_URI_LIST },
    /** @todo Anything else we need to add here? */
    /** @todo Add Wayland / Weston support. */
#endif
};


#ifdef TESTCASE
# ifdef RT_OS_SOLARIS_10
char XtStrings [] = "";
WidgetClassRec* applicationShellWidgetClass;
char XtShellStrings [] = "";
int XmbTextPropertyToTextList(
    Display*            /* display */,
    XTextProperty*      /* text_prop */,
    char***             /* list_return */,
    int*                /* count_return */
)
{
  return 0;
}
# else
const char XtStrings [] = "";
_WidgetClassRec* applicationShellWidgetClass;
const char XtShellStrings [] = "";
# endif /* RT_OS_SOLARIS_10 */
#endif /* TESTCASE */


/*********************************************************************************************************************************
*   Defines                                                                                                                      *
*********************************************************************************************************************************/

#define SHCL_MAX_X11_FORMATS        RT_ELEMENTS(g_aFormats)


/*********************************************************************************************************************************
*   Internal structures                                                                                                          *
*********************************************************************************************************************************/

/**
 * A structure containing information about where to store a request
 * for the X11 clipboard contents.
 */
typedef struct _CLIPREADX11CBREQ
{
    /** The format VBox would like the data in. */
    SHCLFORMAT       uFmtVBox;
    /** The format we requested from X11. */
    SHCLX11FMTIDX    idxFmtX11;
    /** The clipboard context this request is associated with. */
    SHCLX11CTX      *pCtx;
    /** The request structure passed in from the backend. */
    CLIPREADCBREQ   *pReq;
} CLIPREADX11CBREQ;



#ifdef TESTCASE
/**
 * Return the max. number of elements in the X11 format table.
 * Used by the testing code in tstClipboardGH-X11.cpp
 * which cannot use RT_ELEMENTS(g_aFormats) directly.
 *
 * @return size_t The number of elements in the g_aFormats array.
 */
SHCL_X11_DECL(size_t) clipReportMaxX11Formats(void)
{
    return (RT_ELEMENTS(g_aFormats));
}
#endif

/**
 * Returns the atom corresponding to a supported X11 format.
 *
 * @returns Found atom to the corresponding X11 format.
 * @param   pCtx                The X11 clipboard context to use.
 * @param   uFmtIdx             Format index to look up atom for.
 */
static Atom clipAtomForX11Format(PSHCLX11CTX pCtx, SHCLX11FMTIDX uFmtIdx)
{
    AssertReturn(uFmtIdx < RT_ELEMENTS(g_aFormats), 0);
    return clipGetAtom(pCtx, g_aFormats[uFmtIdx].pcszAtom);
}

/**
 * Returns the SHCLX11FMT corresponding to a supported X11 format.
 *
 * @return  SHCLX11FMT for a specific format index.
 * @param   uFmtIdx             Format index to look up SHCLX11CLIPFMT for.
 */
SHCL_X11_DECL(SHCLX11FMT) clipRealFormatForX11Format(SHCLX11FMTIDX uFmtIdx)
{
    AssertReturn(uFmtIdx < RT_ELEMENTS(g_aFormats), SHCLX11FMT_INVALID);
    return g_aFormats[uFmtIdx].enmFmtX11;
}

/**
 * Returns the VBox format corresponding to a supported X11 format.
 *
 * @return  SHCLFORMAT for a specific format index.
 * @param   uFmtIdx             Format index to look up VBox format for.
 */
static SHCLFORMAT clipVBoxFormatForX11Format(SHCLX11FMTIDX uFmtIdx)
{
    AssertReturn(uFmtIdx < RT_ELEMENTS(g_aFormats), VBOX_SHCL_FMT_NONE);
    return g_aFormats[uFmtIdx].uFmtVBox;
}

/**
 * Looks up the X11 format matching a given X11 atom.
 *
 * @returns The format on success, NIL_CLIPX11FORMAT on failure.
 * @param   pCtx                The X11 clipboard context to use.
 * @param   atomFormat          Atom to look up X11 format for.
 */
static SHCLX11FMTIDX clipFindX11FormatByAtom(PSHCLX11CTX pCtx, Atom atomFormat)
{
    for (unsigned i = 0; i < RT_ELEMENTS(g_aFormats); ++i)
        if (clipAtomForX11Format(pCtx, i) == atomFormat)
        {
            LogFlowFunc(("Returning index %u for atom '%s'\n", i, g_aFormats[i].pcszAtom));
            return i;
        }
    return NIL_CLIPX11FORMAT;
}

/**
 * Enumerates supported X11 clipboard formats corresponding to given VBox formats.
 *
 * @returns The next matching X11 format index in the list, or NIL_CLIPX11FORMAT if there are no more.
 * @param   uFormatsVBox            VBox formats to enumerate supported X11 clipboard formats for.
 * @param   lastFmtIdx              The value returned from the last call of this function.
 *                                  Use NIL_CLIPX11FORMAT to start the enumeration.
 */
static SHCLX11FMTIDX clipEnumX11Formats(SHCLFORMATS uFormatsVBox,
                                        SHCLX11FMTIDX lastFmtIdx)
{
    for (unsigned i = lastFmtIdx + 1; i < RT_ELEMENTS(g_aFormats); ++i)
    {
        if (uFormatsVBox & clipVBoxFormatForX11Format(i))
            return i;
    }

    return NIL_CLIPX11FORMAT;
}

/**
 * Array of structures for mapping Xt widgets to context pointers.  We
 * need this because the widget clipboard callbacks do not pass user data.
 */
static struct
{
    /** Pointer to widget we want to associate the context with. */
    Widget      pWidget;
    /** Pointer to X11 context associated with the widget. */
    PSHCLX11CTX pCtx;
} g_aContexts[VBOX_SHARED_CLIPBOARD_X11_CONNECTIONS_MAX];

/**
 * Registers a new X11 clipboard context.
 *
 * @returns VBox status code.
 * @param   pCtx                The X11 clipboard context to use.
 */
static int clipRegisterContext(PSHCLX11CTX pCtx)
{
    AssertPtrReturn(pCtx, VERR_INVALID_PARAMETER);

    bool fFound = false;

    Widget pWidget = pCtx->pWidget;
    AssertReturn(pWidget != NULL, VERR_INVALID_PARAMETER);

    for (unsigned i = 0; i < RT_ELEMENTS(g_aContexts); ++i)
    {
        AssertReturn(   (g_aContexts[i].pWidget != pWidget)
                     && (g_aContexts[i].pCtx != pCtx), VERR_WRONG_ORDER);
        if (g_aContexts[i].pWidget == NULL && !fFound)
        {
            AssertReturn(g_aContexts[i].pCtx == NULL, VERR_INTERNAL_ERROR);
            g_aContexts[i].pWidget = pWidget;
            g_aContexts[i].pCtx = pCtx;
            fFound = true;
        }
    }

    return fFound ? VINF_SUCCESS : VERR_OUT_OF_RESOURCES;
}

/**
 * Unregister an X11 clipboard context.
 *
 * @param   pCtx                The X11 clipboard context to use.
 */
static void clipUnregisterContext(PSHCLX11CTX pCtx)
{
    AssertPtrReturnVoid(pCtx);

    Widget pWidget = pCtx->pWidget;
    AssertPtrReturnVoid(pWidget);

    bool fFound = false;
    for (unsigned i = 0; i < RT_ELEMENTS(g_aContexts); ++i)
    {
        Assert(!fFound || g_aContexts[i].pWidget != pWidget);
        if (g_aContexts[i].pWidget == pWidget)
        {
            Assert(g_aContexts[i].pCtx != NULL);
            g_aContexts[i].pWidget = NULL;
            g_aContexts[i].pCtx = NULL;
            fFound = true;
        }
    }
}

/**
 * Finds a X11 clipboard context for a specific X11 widget.
 *
 * @returns Pointer to associated X11 clipboard context if found, or NULL if not found.
 * @param   pWidget                 X11 widget to return X11 clipboard context for.
 */
static PSHCLX11CTX clipLookupContext(Widget pWidget)
{
    AssertPtrReturn(pWidget, NULL);

    for (unsigned i = 0; i < RT_ELEMENTS(g_aContexts); ++i)
    {
        if (g_aContexts[i].pWidget == pWidget)
        {
            Assert(g_aContexts[i].pCtx != NULL);
            return g_aContexts[i].pCtx;
        }
    }

    return NULL;
}

/**
 * Converts an atom name string to an X11 atom, looking it up in a cache before asking the server.
 *
 * @returns Found X11 atom.
 * @param   pCtx                The X11 clipboard context to use.
 * @param   pcszName            Name of atom to return atom for.
 */
SHCL_X11_DECL(Atom) clipGetAtom(PSHCLX11CTX pCtx, const char *pcszName)
{
    AssertPtrReturn(pcszName, None);
    return XInternAtom(XtDisplay(pCtx->pWidget), pcszName, False);
}

/** String written to the wakeup pipe. */
#define WAKE_UP_STRING      "WakeUp!"
/** Length of the string written. */
#define WAKE_UP_STRING_LEN  ( sizeof(WAKE_UP_STRING) - 1 )

/**
 * Schedules a function call to run on the Xt event thread by passing it to
 * the application context as a 0ms timeout and waking up the event loop by
 * writing to the wakeup pipe which it monitors.
 */
static int clipThreadScheduleCall(PSHCLX11CTX pCtx,
                                  void (*proc)(void *, void *),
                                  void *client_data)
{
    LogFlowFunc(("proc=%p, client_data=%p\n", proc, client_data));

#ifndef TESTCASE
    AssertReturn(pCtx, VERR_INVALID_POINTER);
    AssertReturn(pCtx->pAppContext, VERR_INVALID_POINTER);

    XtAppAddTimeOut(pCtx->pAppContext, 0, (XtTimerCallbackProc)proc,
                    (XtPointer)client_data);
    ssize_t cbWritten = write(pCtx->wakeupPipeWrite, WAKE_UP_STRING, WAKE_UP_STRING_LEN);
    Assert(cbWritten == WAKE_UP_STRING_LEN);
    RT_NOREF(cbWritten);
#else
    RT_NOREF(pCtx);
    tstThreadScheduleCall(proc, client_data);
#endif

    LogFlowFuncLeaveRC(VINF_SUCCESS);
    return VINF_SUCCESS;
}

/**
 * Reports the formats currently supported by the X11 clipboard to VBox.
 *
 * @note    Runs in Xt event thread.
 *
 * @param   pCtx                The X11 clipboard context to use.
 */
static void clipReportFormatsToVBox(PSHCLX11CTX pCtx)
{
    SHCLFORMATS vboxFmt  = clipVBoxFormatForX11Format(pCtx->idxFmtText);
                vboxFmt |= clipVBoxFormatForX11Format(pCtx->idxFmtBmp);
                vboxFmt |= clipVBoxFormatForX11Format(pCtx->idxFmtHTML);
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
                vboxFmt |= clipVBoxFormatForX11Format(pCtx->idxFmtURI);
#endif

    LogFlowFunc(("idxFmtText=%u ('%s'), idxFmtBmp=%u ('%s'), idxFmtHTML=%u ('%s')",
                 pCtx->idxFmtText, g_aFormats[pCtx->idxFmtText].pcszAtom,
                 pCtx->idxFmtBmp,  g_aFormats[pCtx->idxFmtBmp].pcszAtom,
                 pCtx->idxFmtHTML, g_aFormats[pCtx->idxFmtHTML].pcszAtom));
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    LogFlowFunc((", idxFmtURI=%u ('%s')", pCtx->idxFmtURI, g_aFormats[pCtx->idxFmtURI].pcszAtom));
#endif
    LogFlow((" -> vboxFmt=%#x\n", vboxFmt));

#ifdef LOG_ENABLED
    char *pszFmts = ShClFormatsToStrA(vboxFmt);
    AssertPtrReturnVoid(pszFmts);
    LogRel2(("Shared Clipboard: X11 reported available VBox formats '%s'\n", pszFmts));
    RTStrFree(pszFmts);
#endif

    pCtx->Callbacks.pfnReportFormats(pCtx->pFrontend, vboxFmt, NULL /* pvUser */);
}

/**
 * Forgets which formats were previously in the X11 clipboard.  Called when we
 * grab the clipboard.
 *
 * @param   pCtx                The X11 clipboard context to use.
 */
static void clipResetX11Formats(PSHCLX11CTX pCtx)
{
    LogFlowFuncEnter();

    pCtx->idxFmtText = 0;
    pCtx->idxFmtBmp  = 0;
    pCtx->idxFmtHTML = 0;
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    pCtx->idxFmtURI  = 0;
#endif
}

/**
 * Tells VBox that X11 currently has nothing in its clipboard.
 *
 * @param  pCtx                 The X11 clipboard context to use.
 */
SHCL_X11_DECL(void) clipReportEmpty(PSHCLX11CTX pCtx)
{
    clipResetX11Formats(pCtx);
    clipReportFormatsToVBox(pCtx);
}

/**
 * Go through an array of X11 clipboard targets to see if they contain a text
 * format we can support, and if so choose the ones we prefer (e.g. we like
 * UTF-8 better than plain text).
 *
 * @return Index to supported X clipboard format.
 * @param  pCtx                 The X11 clipboard context to use.
 * @param  paIdxFmtTargets      The list of targets.
 * @param  cTargets             The size of the list in @a pTargets.
 */
SHCL_X11_DECL(SHCLX11FMTIDX) clipGetTextFormatFromTargets(PSHCLX11CTX pCtx,
                                                          SHCLX11FMTIDX *paIdxFmtTargets,
                                                          size_t cTargets)
{
    AssertPtrReturn(pCtx, NIL_CLIPX11FORMAT);
    AssertReturn(RT_VALID_PTR(paIdxFmtTargets) || cTargets == 0, NIL_CLIPX11FORMAT);

    SHCLX11FMTIDX idxFmtText = NIL_CLIPX11FORMAT;
    SHCLX11FMT    fmtTextX11 = SHCLX11FMT_INVALID;
    for (unsigned i = 0; i < cTargets; ++i)
    {
        SHCLX11FMTIDX idxFmt = paIdxFmtTargets[i];
        if (idxFmt != NIL_CLIPX11FORMAT)
        {
            if (   (clipVBoxFormatForX11Format(idxFmt) == VBOX_SHCL_FMT_UNICODETEXT)
                && fmtTextX11 < clipRealFormatForX11Format(idxFmt))
            {
                fmtTextX11 = clipRealFormatForX11Format(idxFmt);
                idxFmtText = idxFmt;
            }
        }
    }
    return idxFmtText;
}

/**
 * Goes through an array of X11 clipboard targets to see if they contain a bitmap
 * format we can support, and if so choose the ones we prefer (e.g. we like
 * BMP better than PNG because we don't have to convert).
 *
 * @return Supported X clipboard format.
 * @param  pCtx                 The X11 clipboard context to use.
 * @param  paIdxFmtTargets      The list of targets.
 * @param  cTargets             The size of the list in @a pTargets.
 */
static SHCLX11FMTIDX clipGetBitmapFormatFromTargets(PSHCLX11CTX pCtx,
                                                    SHCLX11FMTIDX *paIdxFmtTargets,
                                                    size_t cTargets)
{
    AssertPtrReturn(pCtx, NIL_CLIPX11FORMAT);
    AssertReturn(RT_VALID_PTR(paIdxFmtTargets) || cTargets == 0, NIL_CLIPX11FORMAT);

    SHCLX11FMTIDX idxFmtBmp = NIL_CLIPX11FORMAT;
    SHCLX11FMT    fmtBmpX11 = SHCLX11FMT_INVALID;
    for (unsigned i = 0; i < cTargets; ++i)
    {
        SHCLX11FMTIDX idxFmt = paIdxFmtTargets[i];
        if (idxFmt != NIL_CLIPX11FORMAT)
        {
            if (   (clipVBoxFormatForX11Format(idxFmt) == VBOX_SHCL_FMT_BITMAP)
                && fmtBmpX11 < clipRealFormatForX11Format(idxFmt))
            {
                fmtBmpX11 = clipRealFormatForX11Format(idxFmt);
                idxFmtBmp = idxFmt;
            }
        }
    }
    return idxFmtBmp;
}

/**
 * Goes through an array of X11 clipboard targets to see if they contain a HTML
 * format we can support, and if so choose the ones we prefer.
 *
 * @return Supported X clipboard format.
 * @param  pCtx                 The X11 clipboard context to use.
 * @param  paIdxFmtTargets      The list of targets.
 * @param  cTargets             The size of the list in @a pTargets.
 */
static SHCLX11FMTIDX clipGetHtmlFormatFromTargets(PSHCLX11CTX pCtx,
                                                  SHCLX11FMTIDX *paIdxFmtTargets,
                                                  size_t cTargets)
{
    AssertPtrReturn(pCtx, NIL_CLIPX11FORMAT);
    AssertReturn(RT_VALID_PTR(paIdxFmtTargets) || cTargets == 0, NIL_CLIPX11FORMAT);

    SHCLX11FMTIDX idxFmtHTML = NIL_CLIPX11FORMAT;
    SHCLX11FMT    fmxHTMLX11 = SHCLX11FMT_INVALID;
    for (unsigned i = 0; i < cTargets; ++i)
    {
        SHCLX11FMTIDX idxFmt = paIdxFmtTargets[i];
        if (idxFmt != NIL_CLIPX11FORMAT)
        {
            if (   (clipVBoxFormatForX11Format(idxFmt) == VBOX_SHCL_FMT_HTML)
                && fmxHTMLX11 < clipRealFormatForX11Format(idxFmt))
            {
                fmxHTMLX11 = clipRealFormatForX11Format(idxFmt);
                idxFmtHTML = idxFmt;
            }
        }
    }
    return idxFmtHTML;
}

# ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
/**
 * Goes through an array of X11 clipboard targets to see if they contain an URI list
 * format we can support, and if so choose the ones we prefer.
 *
 * @return Supported X clipboard format.
 * @param  pCtx                 The X11 clipboard context to use.
 * @param  paIdxFmtTargets      The list of targets.
 * @param  cTargets             The size of the list in @a pTargets.
 */
static SHCLX11FMTIDX clipGetURIListFormatFromTargets(PSHCLX11CTX pCtx,
                                                     SHCLX11FMTIDX *paIdxFmtTargets,
                                                     size_t cTargets)
{
    AssertPtrReturn(pCtx, NIL_CLIPX11FORMAT);
    AssertReturn(RT_VALID_PTR(paIdxFmtTargets) || cTargets == 0, NIL_CLIPX11FORMAT);

    SHCLX11FMTIDX idxFmtURI = NIL_CLIPX11FORMAT;
    SHCLX11FMT    fmtURIX11 = SHCLX11FMT_INVALID;
    for (unsigned i = 0; i < cTargets; ++i)
    {
        SHCLX11FMTIDX idxFmt = paIdxFmtTargets[i];
        if (idxFmt != NIL_CLIPX11FORMAT)
        {
            if (   (clipVBoxFormatForX11Format(idxFmt) == VBOX_SHCL_FMT_URI_LIST)
                && fmtURIX11 < clipRealFormatForX11Format(idxFmt))
            {
                fmtURIX11 = clipRealFormatForX11Format(idxFmt);
                idxFmtURI = idxFmt;
            }
        }
    }
    return idxFmtURI;
}
# endif /* VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS */

/**
 * Goes through an array of X11 clipboard targets to see if we can support any
 * of them and if relevant to choose the ones we prefer (e.g. we like Utf8
 * better than plain text).
 *
 * @param  pCtx                 The X11 clipboard context to use.
 * @param  paIdxFmtTargets      The list of targets.
 * @param  cTargets             The size of the list in @a pTargets.
 */
static void clipGetFormatsFromTargets(PSHCLX11CTX pCtx,
                                      SHCLX11FMTIDX *paIdxFmtTargets, size_t cTargets)
{
    AssertPtrReturnVoid(pCtx);
    AssertPtrReturnVoid(paIdxFmtTargets);

    SHCLX11FMTIDX idxFmtText = clipGetTextFormatFromTargets(pCtx, paIdxFmtTargets, cTargets);
    if (pCtx->idxFmtText != idxFmtText)
        pCtx->idxFmtText = idxFmtText;

    pCtx->idxFmtBmp = SHCLX11FMT_INVALID; /* not yet supported */ /** @todo r=andy Check this. */
    SHCLX11FMTIDX idxFmtBmp = clipGetBitmapFormatFromTargets(pCtx, paIdxFmtTargets, cTargets);
    if (pCtx->idxFmtBmp != idxFmtBmp)
        pCtx->idxFmtBmp = idxFmtBmp;

    SHCLX11FMTIDX idxFmtHTML = clipGetHtmlFormatFromTargets(pCtx, paIdxFmtTargets, cTargets);
    if (pCtx->idxFmtHTML != idxFmtHTML)
        pCtx->idxFmtHTML = idxFmtHTML;

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    SHCLX11FMTIDX idxFmtURI = clipGetURIListFormatFromTargets(pCtx, paIdxFmtTargets, cTargets);
    if (pCtx->idxFmtURI != idxFmtURI)
        pCtx->idxFmtURI = idxFmtURI;
#endif
}

#ifdef VBOX_WITH_SHARED_CLIPBOARD_XT_BUSY
DECLINLINE(bool) clipGetXtBusy(PSHCLX11CTX pCtx)
{
    LogFlowFunc(("fXtBusy=%RTbool, fXtNeedsUpdate=%RTbool\n", pCtx->fXtBusy, pCtx->fXtNeedsUpdate));
    return pCtx->fXtBusy;
}

DECLINLINE(bool) clipGetXtNeedsUpdate(PSHCLX11CTX pCtx)
{
    LogFlowFunc(("fXtBusy=%RTbool, fXtNeedsUpdate=%RTbool\n", pCtx->fXtBusy, pCtx->fXtNeedsUpdate));
    return pCtx->fXtNeedsUpdate;
}

DECLINLINE(bool) clipSetXtBusy(PSHCLX11CTX pCtx, bool fBusy)
{
    pCtx->fXtBusy = fBusy;
    LogFlowFunc(("fXtBusy=%RTbool, fXtNeedsUpdate=%RTbool\n", pCtx->fXtBusy, pCtx->fXtNeedsUpdate));
    return pCtx->fXtBusy;
}

DECLINLINE(bool) clipSetXtNeedsUpdate(PSHCLX11CTX pCtx, bool fNeedsUpdate)
{
    pCtx->fXtNeedsUpdate = fNeedsUpdate;
    LogFlowFunc(("fXtBusy=%RTbool, fXtNeedsUpdate=%RTbool\n", pCtx->fXtBusy, pCtx->fXtNeedsUpdate));
    return pCtx->fXtNeedsUpdate;
}
#endif /* VBOX_WITH_SHARED_CLIPBOARD_XT_BUSY */

/**
 * Updates the context's information about targets currently supported by X11,
 * based on an array of X11 atoms.
 *
 * @param  pCtx                 The X11 clipboard context to use.
 * @param  pTargets             The array of atoms describing the targets supported.
 * @param  cTargets             The size of the array @a pTargets.
 */
SHCL_X11_DECL(void) clipUpdateX11Targets(PSHCLX11CTX pCtx, SHCLX11FMTIDX *paIdxFmtTargets, size_t cTargets)
{
    LogFlowFuncEnter();

#ifdef VBOX_WITH_SHARED_CLIPBOARD_XT_BUSY
    clipSetXtBusy(pCtx, false);
    if (clipGetXtNeedsUpdate(pCtx))
    {
        /* We may already be out of date. */
        clipSetXtNeedsUpdate(pCtx, false);
        clipQueryX11Targets(pCtx);
        return;
    }
#endif

    if (paIdxFmtTargets == NULL)
    {
        /* No data available */
        clipReportEmpty(pCtx);
        return;
    }

    clipGetFormatsFromTargets(pCtx, paIdxFmtTargets, cTargets);
    clipReportFormatsToVBox(pCtx);
}

/**
 * Notifies the VBox clipboard about available data formats ("targets" on X11),
 * based on the information obtained from the X11 clipboard.
 *
 * @note  Callback installed by clipQueryX11Targets() for XtGetSelectionValue().
 * @note  This function is treated as API glue, and as such is not part of any
 *        unit test.  So keep it simple, be paranoid and log everything.
 */
SHCL_X11_DECL(void) clipQueryX11TargetsCallback(Widget widget, XtPointer pClient,
                                                Atom * /* selection */, Atom *atomType,
                                                XtPointer pValue, long unsigned int *pcLen,
                                                int *piFormat)
{
    RT_NOREF(piFormat);

    PSHCLX11CTX pCtx = reinterpret_cast<SHCLX11CTX *>(pClient);

    LogFlowFunc(("pValue=%p, *pcLen=%u, *atomType=%d%s\n",
                 pValue, *pcLen, *atomType, *atomType == XT_CONVERT_FAIL ? " (XT_CONVERT_FAIL)" : ""));

    Atom *pAtoms = (Atom *)pValue;

    unsigned cFormats = *pcLen;

    LogRel2(("Shared Clipboard: Querying X11 formats ...\n"));
    LogRel2(("Shared Clipboard: %u X11 formats were found\n", cFormats));

    SHCLX11FMTIDX *paIdxFmt = NULL;
    if (   cFormats
        && pValue
        && (*atomType != XT_CONVERT_FAIL /* time out */))
    {
        /* Allocated array to hold the format indices. */
        paIdxFmt = (SHCLX11FMTIDX *)RTMemAllocZ(cFormats * sizeof(SHCLX11FMTIDX));
    }

#if !defined(TESTCASE)
    if (pValue)
    {
        for (unsigned i = 0; i < cFormats; ++i)
        {
            if (pAtoms[i])
            {
                char *pszName = XGetAtomName(XtDisplay(widget), pAtoms[i]);
                LogRel2(("Shared Clipboard: Found X11 format '%s'\n", pszName));
                XFree(pszName);
            }
            else
                LogFunc(("Found empty target\n"));
        }
    }
#endif

    if (paIdxFmt)
    {
        for (unsigned i = 0; i < cFormats; ++i)
        {
            for (unsigned j = 0; j < RT_ELEMENTS(g_aFormats); ++j)
            {
                Atom target = XInternAtom(XtDisplay(widget),
                                          g_aFormats[j].pcszAtom, False);
                if (*(pAtoms + i) == target)
                    paIdxFmt[i] = j;
            }
#if !defined(TESTCASE)
            if (paIdxFmt[i] != SHCLX11FMT_INVALID)
                LogRel2(("Shared Clipboard: Reporting X11 format '%s'\n", g_aFormats[paIdxFmt[i]].pcszAtom));
#endif
        }
    }
    else
        LogFunc(("Reporting empty targets (none reported or allocation failure)\n"));

    clipUpdateX11Targets(pCtx, paIdxFmt, cFormats);
    RTMemFree(paIdxFmt);

    XtFree(reinterpret_cast<char *>(pValue));
}

/**
 * Queries the current formats ("targets") of the X11 clipboard ("CLIPBOARD").
 *
 * @param   pCtx                The X11 clipboard context to use.
 */
SHCL_X11_DECL(void) clipQueryX11Targets(PSHCLX11CTX pCtx)
{
#ifndef TESTCASE

# ifdef VBOX_WITH_SHARED_CLIPBOARD_XT_BUSY
    if (clipGetXtBusy(pCtx))
    {
        clipSetXtNeedsUpdate(pCtx, true);
        return;
    }
    clipSetXtBusy(pCtx, true);
# endif

    XtGetSelectionValue(pCtx->pWidget,
                        clipGetAtom(pCtx, "CLIPBOARD"),
                        clipGetAtom(pCtx, "TARGETS"),
                        clipQueryX11TargetsCallback, pCtx,
                        CurrentTime);
#else
    tstRequestTargets(pCtx);
#endif
}

typedef struct
{
    int type;                   /* event base */
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    int subtype;
    Window owner;
    Atom selection;
    Time timestamp;
    Time selection_timestamp;
} XFixesSelectionNotifyEvent;

#ifndef TESTCASE
/**
 * Waits until an event arrives and handle it if it is an XFIXES selection
 * event, which Xt doesn't know about.
 *
 * @param   pCtx                The X11 clipboard context to use.
 */
static void clipPeekEventAndDoXFixesHandling(PSHCLX11CTX pCtx)
{
    union
    {
        XEvent event;
        XFixesSelectionNotifyEvent fixes;
    } event = { { 0 } };

    if (XtAppPeekEvent(pCtx->pAppContext, &event.event))
    {
        if (   (event.event.type == pCtx->fixesEventBase)
            && (event.fixes.owner != XtWindow(pCtx->pWidget)))
        {
            if (   (event.fixes.subtype == 0  /* XFixesSetSelectionOwnerNotify */)
                && (event.fixes.owner != 0))
                clipQueryX11Targets(pCtx);
            else
                clipReportEmpty(pCtx);
        }
    }
}

/**
 * The main loop of our X11 event thread.
 *
 * @returns VBox status code.
 * @param   hThreadSelf             Associated thread handle.
 * @param   pvUser                  Pointer to the X11 clipboard context to use.
 */
static DECLCALLBACK(int) clipThreadMain(RTTHREAD hThreadSelf, void *pvUser)
{
    PSHCLX11CTX pCtx = (PSHCLX11CTX)pvUser;
    AssertPtr(pCtx);

    LogFlowFunc(("pCtx=%p\n", pCtx));

    bool fSignalled = false; /* Whether we have signalled the parent already or not. */

    int rc = clipInitInternal(pCtx);
    if (RT_SUCCESS(rc))
    {
        rc = clipRegisterContext(pCtx);
        if (RT_SUCCESS(rc))
        {
            if (pCtx->fGrabClipboardOnStart)
                clipQueryX11Targets(pCtx);

            pCtx->fThreadStarted = true;

            /* We're now ready to run, tell parent. */
            int rc2 = RTThreadUserSignal(hThreadSelf);
            AssertRC(rc2);

            fSignalled = true;

            while (XtAppGetExitFlag(pCtx->pAppContext) == FALSE)
            {
                clipPeekEventAndDoXFixesHandling(pCtx);
                XtAppProcessEvent(pCtx->pAppContext, XtIMAll);
            }

            LogRel(("Shared Clipboard: X11 event thread exiting\n"));

            clipUnregisterContext(pCtx);
        }
        else
        {
            LogRel(("Shared Clipboard: unable to register clip context: %Rrc\n", rc));
        }

        clipUninitInternal(pCtx);
    }

    if (!fSignalled) /* Signal parent if we didn't do so yet. */
    {
        int rc2 = RTThreadUserSignal(hThreadSelf);
        AssertRC(rc2);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Worker function for stopping the clipboard which runs on the event
 * thread.
 *
 * @param   pvUserData          Pointer to the X11 clipboard context to use.
 */
static void clipThreadSignalStop(void *pvUserData, void *)
{
    PSHCLX11CTX pCtx = (PSHCLX11CTX)pvUserData;

    /* This might mean that we are getting stopped twice. */
    Assert(pCtx->pWidget != NULL);

    /* Set the termination flag to tell the Xt event loop to exit.  We
     * reiterate that any outstanding requests from the X11 event loop to
     * the VBox part *must* have returned before we do this. */
    XtAppSetExitFlag(pCtx->pAppContext);
}

/**
 * Sets up the XFixes library and load the XFixesSelectSelectionInput symbol.
 */
static int clipLoadXFixes(Display *pDisplay, PSHCLX11CTX pCtx)
{
    int rc;

    void *hFixesLib = dlopen("libXfixes.so.1", RTLD_LAZY);
    if (!hFixesLib)
        hFixesLib = dlopen("libXfixes.so.2", RTLD_LAZY);
    if (!hFixesLib)
        hFixesLib = dlopen("libXfixes.so.3", RTLD_LAZY);
    if (!hFixesLib)
        hFixesLib = dlopen("libXfixes.so.4", RTLD_LAZY);
    if (hFixesLib)
    {
        /* For us, a NULL function pointer is a failure */
        pCtx->fixesSelectInput = (void (*)(Display *, Window, Atom, long unsigned int))
                                 (uintptr_t)dlsym(hFixesLib, "XFixesSelectSelectionInput");
        if (pCtx->fixesSelectInput)
        {
            int dummy1 = 0;
            int dummy2 = 0;
            if (XQueryExtension(pDisplay, "XFIXES", &dummy1, &pCtx->fixesEventBase, &dummy2) != 0)
            {
                if (pCtx->fixesEventBase >= 0)
                {
                    rc = VINF_SUCCESS;
                }
                else
                {
                    LogRel(("Shared Clipboard: fixesEventBase is less than zero: %d\n", pCtx->fixesEventBase));
                    rc = VERR_NOT_SUPPORTED;
                }
            }
            else
            {
                LogRel(("Shared Clipboard: XQueryExtension failed\n"));
                rc = VERR_NOT_SUPPORTED;
            }
        }
        else
        {
            LogRel(("Shared Clipboard: Symbol XFixesSelectSelectionInput not found!\n"));
            rc = VERR_NOT_SUPPORTED;
        }
    }
    else
    {
        LogRel(("Shared Clipboard: libxFixes.so.* not found!\n"));
        rc = VERR_NOT_SUPPORTED;
    }
    return rc;
}

/**
 * This is the callback which is scheduled when data is available on the
 * wakeup pipe.  It simply reads all data from the pipe.
 *
 * @param   pvUserData          Pointer to the X11 clipboard context to use.
 */
static void clipThreadDrainWakeupPipe(XtPointer pvUserData, int *, XtInputId *)
{
    LogFlowFuncEnter();

    PSHCLX11CTX pCtx = (PSHCLX11CTX)pvUserData;
    char acBuf[WAKE_UP_STRING_LEN];

    while (read(pCtx->wakeupPipeRead, acBuf, sizeof(acBuf)) > 0) {}
}
#endif /* !TESTCASE */

/**
 * X11-specific initialisation for the Shared Clipboard.
 *
 * Note: Must be called from the thread serving the Xt stuff.
 *
 * @returns VBox status code.
 * @param   pCtx                The X11 clipboard context to init.
 */
static int clipInitInternal(PSHCLX11CTX pCtx)
{
    LogFlowFunc(("pCtx=%p\n", pCtx));

    /* Make sure we are thread safe. */
    XtToolkitThreadInitialize();

    /*
     * Set up the Clipboard application context and main window.  We call all
     * these functions directly instead of calling XtOpenApplication() so
     * that we can fail gracefully if we can't get an X11 display.
     */
    XtToolkitInitialize();

    int rc = VINF_SUCCESS;

    Assert(pCtx->pAppContext == NULL); /* No nested initialization. */
    pCtx->pAppContext = XtCreateApplicationContext();
    if (pCtx->pAppContext == NULL)
    {
        LogRel(("Shared Clipboard: Failed to create Xt application context\n"));
        return VERR_NOT_SUPPORTED; /** @todo Fudge! */
    }

    /* Create a window and make it a clipboard viewer. */
    int      cArgc  = 0;
    char    *pcArgv = 0;
    Display *pDisplay = XtOpenDisplay(pCtx->pAppContext, 0, 0, "VBoxShCl", 0, 0, &cArgc, &pcArgv);
    if (pDisplay == NULL)
    {
        LogRel(("Shared Clipboard: Failed to connect to the X11 clipboard - the window system may not be running\n"));
        rc = VERR_NOT_SUPPORTED;
    }

#ifndef TESTCASE
    if (RT_SUCCESS(rc))
    {
        rc = clipLoadXFixes(pDisplay, pCtx);
        if (RT_FAILURE(rc))
           LogRel(("Shared Clipboard: Failed to load the XFIXES extension\n"));
    }
#endif

    if (RT_SUCCESS(rc))
    {
        pCtx->pWidget = XtVaAppCreateShell(0, "VBoxShCl",
                                           applicationShellWidgetClass,
                                           pDisplay, XtNwidth, 1, XtNheight,
                                           1, NULL);
        if (pCtx->pWidget == NULL)
        {
            LogRel(("Shared Clipboard: Failed to create Xt app shell\n"));
            rc = VERR_NO_MEMORY; /** @todo r=andy Improve this. */
        }
        else
        {
#ifndef TESTCASE
            if (!XtAppAddInput(pCtx->pAppContext, pCtx->wakeupPipeRead,
                               (XtPointer) XtInputReadMask,
                               clipThreadDrainWakeupPipe, (XtPointer) pCtx))
            {
                LogRel(("Shared Clipboard: Failed to add input to Xt app context\n"));
                rc = VERR_ACCESS_DENIED; /** @todo r=andy Improve this. */
            }
#endif
        }
    }

    if (RT_SUCCESS(rc))
    {
        XtSetMappedWhenManaged(pCtx->pWidget, false);
        XtRealizeWidget(pCtx->pWidget);

#ifndef TESTCASE
        /* Enable clipboard update notification. */
        pCtx->fixesSelectInput(pDisplay, XtWindow(pCtx->pWidget),
                               clipGetAtom(pCtx, "CLIPBOARD"),
                               7 /* All XFixes*Selection*NotifyMask flags */);
#endif
    }

    if (RT_FAILURE(rc))
    {
        LogRel(("Shared Clipboard: Initialisation failed: %Rrc\n", rc));
        clipUninitInternal(pCtx);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * X11-specific uninitialisation for the Shared Clipboard.
 *
 * Note: Must be called from the thread serving the Xt stuff.
 *
 * @param   pCtx                The X11 clipboard context to uninit.
 */
static void clipUninitInternal(PSHCLX11CTX pCtx)
{
    AssertPtrReturnVoid(pCtx);

    LogFlowFunc(("pCtx=%p\n", pCtx));

    if (pCtx->pWidget)
    {
        /* Valid widget + invalid appcontext = bug.  But don't return yet. */
        AssertPtr(pCtx->pAppContext);

        XtDestroyWidget(pCtx->pWidget);
        pCtx->pWidget = NULL;
    }

    if (pCtx->pAppContext)
    {
        XtDestroyApplicationContext(pCtx->pAppContext);
        pCtx->pAppContext = NULL;
    }

    LogFlowFuncLeaveRC(VINF_SUCCESS);
}

/**
 * Sets the callback table, internal version.
 *
 * @param   pCtx                The clipboard context.
 * @param   pCallbacks          Callback table to set. If NULL, the current callback table will be cleared.
 */
static void shClX11SetCallbacksInternal(PSHCLX11CTX pCtx, PSHCLCALLBACKS pCallbacks)
{
    if (pCallbacks)
    {
        memcpy(&pCtx->Callbacks, pCallbacks, sizeof(SHCLCALLBACKS));
    }
    else
        RT_ZERO(pCtx->Callbacks);
}

/**
 * Sets the callback table.
 *
 * @param   pCtx                The clipboard context.
 * @param   pCallbacks          Callback table to set. If NULL, the current callback table will be cleared.
 */
void ShClX11SetCallbacks(PSHCLX11CTX pCtx, PSHCLCALLBACKS pCallbacks)
{
    shClX11SetCallbacksInternal(pCtx, pCallbacks);
}

/**
 * Initializes a X11 context of the Shared Clipboard.
 *
 * @returns VBox status code.
 * @param   pCtx                The clipboard context to initialize.
 * @param   pCallbacks          Callback table to use.
 * @param   pParent             Parent context to use.
 * @param   fHeadless           Whether the code runs in a headless environment or not.
 */
int ShClX11Init(PSHCLX11CTX pCtx, PSHCLCALLBACKS pCallbacks, PSHCLCONTEXT pParent, bool fHeadless)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    LogFlowFunc(("pCtx=%p\n", pCtx));

    int rc = VINF_SUCCESS;

    RT_BZERO(pCtx, sizeof(SHCLX11CTX));

    if (fHeadless)
    {
        /*
         * If we don't find the DISPLAY environment variable we assume that
         * we are not connected to an X11 server. Don't actually try to do
         * this then, just fail silently and report success on every call.
         * This is important for VBoxHeadless.
         */
        LogRel(("Shared Clipboard: X11 DISPLAY variable not set -- disabling clipboard sharing\n"));
    }

    /* Install given callbacks. */
    shClX11SetCallbacksInternal(pCtx, pCallbacks);

    pCtx->fHaveX11       = !fHeadless;
    pCtx->pFrontend      = pParent;

#ifdef VBOX_WITH_SHARED_CLIPBOARD_XT_BUSY
    pCtx->fXtBusy        = false;
    pCtx->fXtNeedsUpdate = false;
#endif

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS_HTTP
    ShClTransferHttpServerInit(&pCtx->HttpCtx.HttpServer);
#endif

#ifdef TESTCASE
    if (RT_SUCCESS(rc))
    {
        /** @todo The testcases currently do not utilize the threading code. So init stuff here. */
        rc = clipInitInternal(pCtx);
        if (RT_SUCCESS(rc))
            rc = clipRegisterContext(pCtx);
    }
#endif

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Destroys a Shared Clipboard X11 context.
 *
 * @param   pCtx                The X11 clipboard context to destroy.
 */
void ShClX11Destroy(PSHCLX11CTX pCtx)
{
    if (!pCtx)
        return;

    LogFlowFunc(("pCtx=%p\n", pCtx));

#ifdef TESTCASE
    /** @todo The testcases currently do not utilize the threading code. So uninit stuff here. */
    clipUnregisterContext(pCtx);
    clipUninitInternal(pCtx);
#endif

    if (pCtx->fHaveX11)
    {
        /* We set this to NULL when the event thread exits.  It really should
         * have exited at this point, when we are about to unload the code from
         * memory. */
        Assert(pCtx->pWidget == NULL);
    }
}

#ifndef TESTCASE
/**
 * Starts our own Xt even thread for handling Shared Clipboard messages, extended version.
 *
 * @returns VBox status code.
 * @param   pCtx                The X11 clipboard context to use.
 * @param   pszName             Thread name to use.
 * @param   fGrab               Whether we should try to grab the shared clipboard at once.
 */
int ShClX11ThreadStartEx(PSHCLX11CTX pCtx, const char *pszName, bool fGrab)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    /*
     * Immediately return if we are not connected to the X server.
     */
    if (!pCtx->fHaveX11)
        return VINF_SUCCESS;

    pCtx->fGrabClipboardOnStart = fGrab;

    clipResetX11Formats(pCtx);

    int rc;

    /*
     * Create the pipes.
     ** @todo r=andy Replace this with RTPipe API.
     */
    int pipes[2];
    if (!pipe(pipes))
    {
        pCtx->wakeupPipeRead  = pipes[0];
        pCtx->wakeupPipeWrite = pipes[1];

        if (!fcntl(pCtx->wakeupPipeRead, F_SETFL, O_NONBLOCK))
        {
            rc = VINF_SUCCESS;
        }
        else
            rc = RTErrConvertFromErrno(errno);
    }
    else
        rc = RTErrConvertFromErrno(errno);

    if (RT_SUCCESS(rc))
    {
        LogRel2(("Shared Clipboard: Starting X11 event thread ...\n"));

        rc = RTThreadCreate(&pCtx->Thread, clipThreadMain, pCtx, 0,
                            RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE, pszName);
        if (RT_SUCCESS(rc))
            rc = RTThreadUserWait(pCtx->Thread, RT_MS_30SEC /* msTimeout */);

        if (RT_FAILURE(rc))
        {
            LogRel(("Shared Clipboard: Failed to start the X11 event thread with %Rrc\n", rc));
            clipUninitInternal(pCtx);
        }
        else
        {
            if (!pCtx->fThreadStarted)
            {
                LogRel(("Shared Clipboard: X11 event thread reported an error while starting\n"));
            }
            else
                LogRel2(("Shared Clipboard: X11 event thread started\n"));
        }
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Starts our own Xt even thread for handling Shared Clipboard messages.
 *
 * @returns VBox status code.
 * @param   pCtx                The X11 clipboard context to use.
 * @param   fGrab               Whether we should try to grab the shared clipboard at once.
 */
int ShClX11ThreadStart(PSHCLX11CTX pCtx, bool fGrab)
{
    return ShClX11ThreadStartEx(pCtx, "SHCLX11", fGrab);
}

/**
 * Stops the Shared Clipboard Xt even thread.
 *
 * @note  Any requests from this object to get clipboard data from VBox
 *        *must* have completed or aborted before we are called, as
 *        otherwise the X11 event loop will still be waiting for the request
 *        to return and will not be able to terminate.
 *
 * @returns VBox status code.
 * @param   pCtx                The X11 clipboard context to use.
 */
int ShClX11ThreadStop(PSHCLX11CTX pCtx)
{
    int rc;
    /*
     * Immediately return if we are not connected to the X server.
     */
    if (!pCtx->fHaveX11)
        return VINF_SUCCESS;

    LogRel2(("Shared Clipboard: Signalling the X11 event thread to stop\n"));

    /* Write to the "stop" pipe. */
    rc = clipThreadScheduleCall(pCtx, clipThreadSignalStop, (XtPointer)pCtx);
    if (RT_FAILURE(rc))
    {
        LogRel(("Shared Clipboard: cannot notify X11 event thread on shutdown with %Rrc\n", rc));
        return rc;
    }

    LogRel2(("Shared Clipboard: Waiting for X11 event thread to stop ...\n"));

    int rcThread;
    rc = RTThreadWait(pCtx->Thread, RT_MS_30SEC /* msTimeout */, &rcThread);
    if (RT_SUCCESS(rc))
        rc = rcThread;
    if (RT_SUCCESS(rc))
    {
        if (pCtx->wakeupPipeRead != 0)
        {
            close(pCtx->wakeupPipeRead);
            pCtx->wakeupPipeRead = 0;
        }

        if (pCtx->wakeupPipeWrite != 0)
        {
            close(pCtx->wakeupPipeWrite);
            pCtx->wakeupPipeWrite = 0;
        }
    }

    if (RT_SUCCESS(rc))
    {
        LogRel2(("Shared Clipboard: X11 event thread stopped successfully\n"));
    }
    else
        LogRel(("Shared Clipboard: Stopping X11 event thread failed with %Rrc\n", rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}
#endif /* !TESTCASE */

/**
 * Returns the targets supported by VBox.
 *
 * This will return a list of atoms which tells the caller
 * what kind of clipboard formats we support.
 *
 * @returns VBox status code.
 * @param  pCtx                 The X11 clipboard context to use.
 * @param  atomTypeReturn       The type of the data we are returning.
 * @param  pValReturn           A pointer to the data we are returning. This
 *                              should be set to memory allocated by XtMalloc,
 *                              which will be freed later by the Xt toolkit.
 * @param  pcLenReturn          The length of the data we are returning.
 * @param  piFormatReturn       The format (8bit, 16bit, 32bit) of the data we are
 *                              returning.
 * @note  X11 backend code, called by the XtOwnSelection callback.
 */
static int clipCreateX11Targets(PSHCLX11CTX pCtx, Atom *atomTypeReturn,
                                XtPointer *pValReturn,
                                unsigned long *pcLenReturn,
                                int *piFormatReturn)
{
    const unsigned cFixedTargets = 3; /* See below. */

    Atom *pAtomTargets = (Atom *)XtMalloc((SHCL_MAX_X11_FORMATS + cFixedTargets) * sizeof(Atom));
    if (!pAtomTargets)
        return VERR_NO_MEMORY;

    unsigned cTargets = 0;
    SHCLX11FMTIDX idxFmt = NIL_CLIPX11FORMAT;
    do
    {
        idxFmt = clipEnumX11Formats(pCtx->vboxFormats, idxFmt);
        if (idxFmt != NIL_CLIPX11FORMAT)
        {
            pAtomTargets[cTargets] = clipAtomForX11Format(pCtx, idxFmt);
            ++cTargets;
        }
    } while (idxFmt != NIL_CLIPX11FORMAT);

    /* We always offer these fixed targets. */
    pAtomTargets[cTargets]     = clipGetAtom(pCtx, "TARGETS");
    pAtomTargets[cTargets + 1] = clipGetAtom(pCtx, "MULTIPLE");
    pAtomTargets[cTargets + 2] = clipGetAtom(pCtx, "TIMESTAMP");

    *atomTypeReturn = XA_ATOM;
    *pValReturn = (XtPointer)pAtomTargets;
    *pcLenReturn = cTargets + cFixedTargets;
    *piFormatReturn = 32;

    LogFlowFunc(("cTargets=%u\n", cTargets + cFixedTargets));

    return VINF_SUCCESS;
}

/**
 * Helper for ShClX11RequestDataForX11Callback() that will cache the data returned.
 *
 * @returns VBox status code. VERR_NO_DATA if no data available.
 * @param   pCtx                The X11 clipboard context to use.
 * @param   uFmt                Clipboard format to read data in.
 * @param   ppv                 Returns an allocated buffer with data read on success.
 *                              Needs to be free'd with RTMemFree() by the caller.
 * @param   pcb                 Returns the amount of data read (in bytes) on success.
 */
static int shClX11RequestDataForX11CallbackHelper(PSHCLX11CTX pCtx, SHCLFORMAT uFmt,
                                                  void **ppv, uint32_t *pcb)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(ppv,  VERR_INVALID_POINTER);
    AssertPtrReturn(pcb,  VERR_INVALID_POINTER);

    LogFlowFunc(("pCtx=%p, uFmt=%#x\n", pCtx, uFmt));

    int rc = VINF_SUCCESS;

    void    *pv = NULL;
    uint32_t cb = 0;

    if (uFmt == VBOX_SHCL_FMT_UNICODETEXT)
    {
        if (pCtx->pvUnicodeCache == NULL) /** @todo r=andy Using string cache here? */
            rc = pCtx->Callbacks.pfnOnRequestDataFromSource(pCtx->pFrontend, uFmt, &pCtx->pvUnicodeCache, &pCtx->cbUnicodeCache,
                                                            NULL /* pvUser */);
        if (   RT_SUCCESS(rc)
            /* Catch misbehaving callbacks. */
            && pCtx->pvUnicodeCache
            && pCtx->cbUnicodeCache)
        {
            pv = RTMemDup(pCtx->pvUnicodeCache, pCtx->cbUnicodeCache);
            if (pv)
                cb = pCtx->cbUnicodeCache;
            else
                rc = VERR_NO_MEMORY;
        }
    }
    else
        rc = pCtx->Callbacks.pfnOnRequestDataFromSource(pCtx->pFrontend, uFmt, &pv, &cb, NULL /* pvUser */);


    /* Safey net in case the callbacks above misbehave
     * (must return VERR_NO_DATA if no data available). */
    if (   RT_SUCCESS(rc)
        && (pv == NULL || cb == 0))
        rc = VERR_NO_DATA;

    if (RT_SUCCESS(rc))
    {
        *ppv = pv;
        *pcb = cb;
    }

    LogFlowFunc(("Returning pv=%p, cb=%RU32, rc=%Rrc\n", pv, cb, rc));
    return rc;
}

/**
 * Satisfies a request from X11 to convert the clipboard text to UTF-8 LF.
 *
 * @returns VBox status code. VERR_NO_DATA if no data was converted.
 * @param  pDisplay             An X11 display structure, needed for conversions
 *                              performed by Xlib.
 * @param  pv                   The text to be converted (UCS-2 with Windows EOLs).
 * @param  cb                   The length of the text in @cb in bytes.
 * @param  atomTypeReturn       Where to store the atom for the type of the data
 *                              we are returning.
 * @param  pValReturn           Where to store the pointer to the data we are
 *                              returning.  This should be to memory allocated by
 *                              XtMalloc, which will be freed by the Xt toolkit
 *                              later.
 * @param  pcLenReturn          Where to store the length of the data we are
 *                              returning.
 * @param  piFormatReturn       Where to store the bit width (8, 16, 32) of the
 *                              data we are returning.
 */
static int clipConvertUtf16ToX11Data(Display *pDisplay, PRTUTF16 pwszSrc,
                                     size_t cbSrc, Atom *atomTarget,
                                     Atom *atomTypeReturn,
                                     XtPointer *pValReturn,
                                     unsigned long *pcLenReturn,
                                     int *piFormatReturn)
{
    RT_NOREF(pDisplay);
    AssertReturn(cbSrc % sizeof(RTUTF16) == 0, VERR_INVALID_PARAMETER);

    const size_t cwcSrc = cbSrc / sizeof(RTUTF16);
    if (!cwcSrc)
        return VERR_NO_DATA;

    /* This may slightly overestimate the space needed. */
    size_t chDst = 0;
    int rc = ShClUtf16LenUtf8(pwszSrc, cwcSrc, &chDst);
    if (RT_SUCCESS(rc))
    {
        chDst++; /* Add space for terminator. */

        char *pszDst = (char *)XtMalloc(chDst);
        if (pszDst)
        {
            size_t cbActual = 0;
            rc = ShClConvUtf16CRLFToUtf8LF(pwszSrc, cwcSrc, pszDst, chDst, &cbActual);
            if (RT_SUCCESS(rc))
            {
                *atomTypeReturn = *atomTarget;
                *pValReturn     = (XtPointer)pszDst;
                *pcLenReturn    = cbActual + 1 /* Include terminator */;
                *piFormatReturn = 8;
            }
        }
        else
            rc = VERR_NO_MEMORY;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Satisfies a request from X11 to convert the clipboard HTML fragment to UTF-8.  We
 * return null-terminated text, but can cope with non-null-terminated input.
 *
 * @returns VBox status code.
 * @param  pDisplay             An X11 display structure, needed for conversions
 *                              performed by Xlib.
 * @param  pv                   The text to be converted (UTF8 with Windows EOLs).
 * @param  cb                   The length of the text in @cb in bytes.
 * @param  atomTypeReturn       Where to store the atom for the type of the data
 *                              we are returning.
 * @param  pValReturn           Where to store the pointer to the data we are
 *                              returning.  This should be to memory allocated by
 *                              XtMalloc, which will be freed by the Xt toolkit later.
 * @param  pcLenReturn          Where to store the length of the data we are returning.
 * @param  piFormatReturn       Where to store the bit width (8, 16, 32) of the
 *                              data we are returning.
 */
static int clipConvertHtmlToX11Data(Display *pDisplay, const char *pszSrc,
                                    size_t cbSrc, Atom *atomTarget,
                                    Atom *atomTypeReturn,
                                    XtPointer *pValReturn,
                                    unsigned long *pcLenReturn,
                                    int *piFormatReturn)
{
    RT_NOREF(pDisplay, pValReturn);

    /* This may slightly overestimate the space needed. */
    LogFlowFunc(("Source: %s", pszSrc));

    char *pszDest = (char *)XtMalloc(cbSrc);
    if (pszDest == NULL)
        return VERR_NO_MEMORY;

    memcpy(pszDest, pszSrc, cbSrc);

    *atomTypeReturn = *atomTarget;
    *pValReturn = (XtPointer)pszDest;
    *pcLenReturn = cbSrc;
    *piFormatReturn = 8;

    return VINF_SUCCESS;
}


/**
 * Does this atom correspond to one of the two selection types we support?
 *
 * @param  pCtx                 The X11 clipboard context to use.
 * @param  selType              The atom in question.
 */
static bool clipIsSupportedSelectionType(PSHCLX11CTX pCtx, Atom selType)
{
    return(   (selType == clipGetAtom(pCtx, "CLIPBOARD"))
           || (selType == clipGetAtom(pCtx, "PRIMARY")));
}

/**
 * Removes a trailing nul character from a string by adjusting the string
 * length.  Some X11 applications don't like zero-terminated text...
 *
 * @param  pText                The text in question.
 * @param  pcText               The length of the text, adjusted on return.
 * @param  format               The format of the text.
 */
static void clipTrimTrailingNul(XtPointer pText, unsigned long *pcText,
                                SHCLX11FMT format)
{
    AssertPtrReturnVoid(pText);
    AssertPtrReturnVoid(pcText);
    AssertReturnVoid((format == SHCLX11FMT_UTF8) || (format == SHCLX11FMT_TEXT) || (format == SHCLX11FMT_HTML));

    if (((char *)pText)[*pcText - 1] == '\0')
       --(*pcText);
}

static int clipConvertToX11Data(PSHCLX11CTX pCtx, Atom *atomTarget,
                                Atom *atomTypeReturn,
                                XtPointer *pValReturn,
                                unsigned long *pcLenReturn,
                                int *piFormatReturn)
{
    int rc = VERR_NOT_SUPPORTED; /* Play safe by default. */

    SHCLX11FMTIDX idxFmtX11 = clipFindX11FormatByAtom(pCtx, *atomTarget);
    SHCLX11FMT    fmtX11    = clipRealFormatForX11Format(idxFmtX11);

    LogFlowFunc(("vboxFormats=0x%x, idxFmtX11=%u ('%s'), fmtX11=%u\n",
                 pCtx->vboxFormats, idxFmtX11, g_aFormats[idxFmtX11].pcszAtom, fmtX11));

#ifdef LOG_ENABLED
    char *pszFmts = ShClFormatsToStrA(pCtx->vboxFormats);
    AssertPtrReturn(pszFmts, VERR_NO_MEMORY);
    LogRel2(("Shared Clipboard: Converting VBox formats '%s' to '%s' for X11\n",
             pszFmts, g_aFormats[idxFmtX11].pcszAtom));
    RTStrFree(pszFmts);
#endif

    void    *pv = NULL;
    uint32_t cb = 0;

    if (   (   (fmtX11 == SHCLX11FMT_UTF8)
            || (fmtX11 == SHCLX11FMT_TEXT)
           )
        && (pCtx->vboxFormats & VBOX_SHCL_FMT_UNICODETEXT))
    {
        rc = shClX11RequestDataForX11CallbackHelper(pCtx, VBOX_SHCL_FMT_UNICODETEXT, &pv, &cb);
        if (   RT_SUCCESS(rc)
            && (   (fmtX11 == SHCLX11FMT_UTF8)
                || (fmtX11 == SHCLX11FMT_TEXT)))
        {
            rc = clipConvertUtf16ToX11Data(XtDisplay(pCtx->pWidget),
                                           (PRTUTF16)pv, cb, atomTarget,
                                           atomTypeReturn, pValReturn,
                                           pcLenReturn, piFormatReturn);
        }

        if (RT_SUCCESS(rc))
            clipTrimTrailingNul(*(XtPointer *)pValReturn, pcLenReturn, fmtX11);

        RTMemFree(pv);
    }
    else if (   (fmtX11 == SHCLX11FMT_BMP)
             && (pCtx->vboxFormats & VBOX_SHCL_FMT_BITMAP))
    {
        rc = shClX11RequestDataForX11CallbackHelper(pCtx, VBOX_SHCL_FMT_BITMAP, &pv, &cb);
        if (   RT_SUCCESS(rc)
            && (fmtX11 == SHCLX11FMT_BMP))
        {
            /* Create a full BMP from it. */
            rc = ShClDibToBmp(pv, cb, (void **)pValReturn,
                              (size_t *)pcLenReturn);
        }

        if (RT_SUCCESS(rc))
        {
            *atomTypeReturn = *atomTarget;
            *piFormatReturn = 8;
        }

        RTMemFree(pv);
    }
    else if (  (fmtX11 == SHCLX11FMT_HTML)
            && (pCtx->vboxFormats & VBOX_SHCL_FMT_HTML))
    {
        rc = shClX11RequestDataForX11CallbackHelper(pCtx, VBOX_SHCL_FMT_HTML, &pv, &cb);
        if (RT_SUCCESS(rc))
        {
            /**
             * The common VBox HTML encoding will be UTF-8.
             * Before sending it to the X11 clipboard we have to convert it to UTF-8 first.
             *
             * Strange that we get UTF-16 from the X11 clipboard, but
             * in same time we send UTF-8 to X11 clipboard and it works.
             ** @todo r=andy Verify this.
             */
            rc = clipConvertHtmlToX11Data(XtDisplay(pCtx->pWidget),
                                          (const char*)pv, cb, atomTarget,
                                          atomTypeReturn, pValReturn,
                                          pcLenReturn, piFormatReturn);
            if (RT_SUCCESS(rc))
                clipTrimTrailingNul(*(XtPointer *)pValReturn, pcLenReturn, fmtX11);

            RTMemFree(pv);
        }
    }
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    else if (fmtX11 == SHCLX11FMT_URI_LIST)
    {
        if (pCtx->vboxFormats & VBOX_SHCL_FMT_URI_LIST)
        {
            rc = shClX11RequestDataForX11CallbackHelper(pCtx, VBOX_SHCL_FMT_URI_LIST, &pv, &cb);
            if (RT_SUCCESS(rc))
            {
                void *pvDst = (void *)XtMalloc(cb);
                if (pvDst)
                {
                    memcpy(pvDst, pv, cb);

                    *atomTypeReturn = *atomTarget;
                    *pValReturn     = (XtPointer)pvDst;
                    *pcLenReturn    = cb;
                    *piFormatReturn = 8;
                }
                else
                    rc = VERR_NO_MEMORY;
            }
        }
        /* else not supported yet. */
    }
#endif
    else
    {
        *atomTypeReturn = XT_CONVERT_FAIL;
        *pValReturn     = (XtPointer)NULL;
        *pcLenReturn    = 0;
        *piFormatReturn = 0;
    }

    if (RT_FAILURE(rc))
    {
        char *pszFmts2 = ShClFormatsToStrA(pCtx->vboxFormats);
        char *pszAtomName = XGetAtomName(XtDisplay(pCtx->pWidget), *atomTarget);

        LogRel(("Shared Clipboard: Converting VBox formats '%s' to '%s' for X11 (idxFmtX11=%u, fmtX11=%u, atomTarget='%s') failed, rc=%Rrc\n",
                pszFmts2 ? pszFmts2 : "unknown", g_aFormats[idxFmtX11].pcszAtom, idxFmtX11, fmtX11, pszAtomName ? pszAtomName : "unknown", rc));

        if (pszFmts2)
            RTStrFree(pszFmts2);
        if (pszAtomName)
            XFree(pszAtomName);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Returns VBox's clipboard data for an X11 client.
 *
 * @note Callback for XtOwnSelection.
 */
static Boolean clipXtConvertSelectionProc(Widget widget, Atom *atomSelection,
                                          Atom *atomTarget,
                                          Atom *atomTypeReturn,
                                          XtPointer *pValReturn,
                                          unsigned long *pcLenReturn,
                                          int *piFormatReturn)
{
    LogFlowFuncEnter();

    PSHCLX11CTX pCtx = clipLookupContext(widget);
    if (!pCtx)
        return False;

    /* Is this the rigt selection (clipboard) we were asked for? */
    if (!clipIsSupportedSelectionType(pCtx, *atomSelection))
        return False;

    int rc;
    if (*atomTarget == clipGetAtom(pCtx, "TARGETS"))
        rc = clipCreateX11Targets(pCtx, atomTypeReturn, pValReturn,
                                  pcLenReturn, piFormatReturn);
    else
        rc = clipConvertToX11Data(pCtx, atomTarget, atomTypeReturn,
                                  pValReturn, pcLenReturn, piFormatReturn);

#if 0 /** @todo Disabled -- crashes when running with tstClipboardGH-X11. */
    XSelectionRequestEvent* pReq =
        XtGetSelectionRequest(widget, *atomSelection, (XtRequestId)NULL);
    LogFlowFunc(("returning pVBoxWnd=%#x, ownerWnd=%#x, reqWnd=%#x, %RTbool, rc=%Rrc\n",
                 XtWindow(pCtx->pWidget), pReq->owner, pReq->requestor, RT_SUCCESS(rc), rc));
#endif
    return RT_SUCCESS(rc) ? True : False;
}

static void clipXtConvertSelectionProcLose(Widget widget, Atom *atomSelection)
{
    RT_NOREF(widget, atomSelection);
    LogFlowFuncEnter();
}

static void clipXtConvertSelectionProcDone(Widget widget, Atom *atomSelection, Atom *atomTarget)
{
    RT_NOREF(widget, atomSelection, atomTarget);
    LogFlowFuncEnter();
}

/**
 * Structure used to pass information about formats that VBox supports.
 */
typedef struct _CLIPNEWVBOXFORMATS
{
    /** Context information for the X11 clipboard. */
    PSHCLX11CTX pCtx;
    /** Formats supported by VBox. */
    SHCLFORMATS  Formats;
} CLIPNEWVBOXFORMATS, *PCLIPNEWVBOXFORMATS;



/**
 * Invalidates the local cache of the data in the VBox clipboard.
 *
 * @param   pCtx                The X11 clipboard context to use.
 */
static void clipInvalidateClipboardCache(PSHCLX11CTX pCtx)
{
    if (pCtx->pvUnicodeCache != NULL)
    {
        RTMemFree(pCtx->pvUnicodeCache);
        pCtx->pvUnicodeCache = NULL;
    }
}

/**
 * Takes possession of the X11 clipboard (and middle-button selection).
 *
 * @param   pCtx                The X11 clipboard context to use.
 * @param   uFormats            Clipboard formats to set.
 */
static void clipGrabX11Clipboard(PSHCLX11CTX pCtx, SHCLFORMATS uFormats)
{
    LogFlowFuncEnter();

    /** @ŧodo r=andy The docs say: "the value CurrentTime is not acceptable" here!? */
    if (XtOwnSelection(pCtx->pWidget, clipGetAtom(pCtx, "CLIPBOARD"),
                       CurrentTime,
                       clipXtConvertSelectionProc, clipXtConvertSelectionProcLose, clipXtConvertSelectionProcDone))
    {
        pCtx->vboxFormats = uFormats;

        /* Grab the middle-button paste selection too. */
        XtOwnSelection(pCtx->pWidget, clipGetAtom(pCtx, "PRIMARY"),
                       CurrentTime, clipXtConvertSelectionProc, NULL, 0);
#ifndef TESTCASE
        /* Xt suppresses these if we already own the clipboard, so send them
         * ourselves. */
        XSetSelectionOwner(XtDisplay(pCtx->pWidget),
                           clipGetAtom(pCtx, "CLIPBOARD"),
                           XtWindow(pCtx->pWidget), CurrentTime);
        XSetSelectionOwner(XtDisplay(pCtx->pWidget),
                           clipGetAtom(pCtx, "PRIMARY"),
                           XtWindow(pCtx->pWidget), CurrentTime);
#endif
    }
}

/**
 * Worker function for ShClX11ReportFormatsToX11 which runs on the
 * event thread.
 *
 * @param pvUserData            Pointer to a CLIPNEWVBOXFORMATS structure containing
 *                              information about the VBox formats available and the
 *                              clipboard context data.  Must be freed by the worker.
 */
static void ShClX11ReportFormatsToX11Worker(void *pvUserData, void * /* interval */)
{
    AssertPtrReturnVoid(pvUserData);

    CLIPNEWVBOXFORMATS *pFormats = (CLIPNEWVBOXFORMATS *)pvUserData;

    PSHCLX11CTX pCtx     = pFormats->pCtx;
    SHCLFORMATS fFormats = pFormats->Formats;

    RTMemFree(pFormats);

#ifdef LOG_ENABLED
    char *pszFmts = ShClFormatsToStrA(fFormats);
    AssertPtrReturnVoid(pszFmts);
    LogRel2(("Shared Clipboard: Reported available VBox formats %s to X11\n", pszFmts));
    RTStrFree(pszFmts);
#endif

    clipInvalidateClipboardCache(pCtx);
    clipGrabX11Clipboard(pCtx, fFormats);
    clipResetX11Formats(pCtx);

    LogFlowFuncLeave();
}

/**
 * Announces new clipboard formats to the X11 clipboard.
 *
 * @returns VBox status code.
 * @param   pCtx                Context data for the clipboard backend.
 * @param   uFormats            Clipboard formats offered.
 */
int ShClX11ReportFormatsToX11(PSHCLX11CTX pCtx, SHCLFORMATS uFormats)
{
    /*
     * Immediately return if we are not connected to the X server.
     */
    if (!pCtx->fHaveX11)
        return VINF_SUCCESS;

    int rc;

    /* This must be free'd by the worker callback. */
    PCLIPNEWVBOXFORMATS pFormats = (PCLIPNEWVBOXFORMATS)RTMemAlloc(sizeof(CLIPNEWVBOXFORMATS));
    if (pFormats)
    {
        pFormats->pCtx    = pCtx;
        pFormats->Formats = uFormats;
        rc = clipThreadScheduleCall(pCtx, ShClX11ReportFormatsToX11Worker,
                                    (XtPointer)pFormats);
        if (RT_FAILURE(rc))
            RTMemFree(pFormats);
    }
    else
        rc = VERR_NO_MEMORY;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Converts the data obtained from the X11 clipboard to the required format,
 * place it in the buffer supplied and signal that data has arrived.
 *
 * Converts the text obtained UTF-16LE with Windows EOLs.
 * Converts full BMP data to DIB format.
 */
SHCL_X11_DECL(void) clipConvertDataFromX11Worker(void *pClient, void *pvSrc, unsigned cbSrc)
{
    CLIPREADX11CBREQ *pReq = (CLIPREADX11CBREQ *)pClient;
    AssertPtrReturnVoid(pReq);

    LogFlowFunc(("pReq->uFmtVBox=%#x, pReq->idxFmtX11=%u, pReq->pCtx=%p\n", pReq->uFmtVBox, pReq->idxFmtX11, pReq->pCtx));

    LogRel2(("Shared Clipboard: Converting X11 format '%s' to VBox format %#x\n",
             g_aFormats[pReq->idxFmtX11].pcszAtom, pReq->uFmtVBox));

    AssertPtr(pReq->pCtx);
    Assert(pReq->uFmtVBox != VBOX_SHCL_FMT_NONE); /* Sanity. */

    int rc = VINF_SUCCESS;

    void  *pvDst = NULL;
    size_t cbDst = 0;

#ifdef VBOX_WITH_SHARED_CLIPBOARD_XT_BUSY
    PSHCLX11CTX pCtx = pReq->pCtx;
    AssertPtr(pReq->pCtx);
    clipSetXtBusy(pCtx, false);
    if (clipGetXtNeedsUpdate(pCtx))
        clipQueryX11Targets(pCtx);
#endif

    /* If X11 clipboard buffer has no data, libXt can pass to XtGetSelectionValue()
     * callback an empty string, in this case cbSrc is 0. */
    if (pvSrc == NULL || cbSrc == 0)
    {
        /* The clipboard selection may have changed before we could get it. */
        rc = VERR_NO_DATA;
    }
    else if (pReq->uFmtVBox == VBOX_SHCL_FMT_UNICODETEXT)
    {
        /* In which format is the clipboard data? */
        switch (clipRealFormatForX11Format(pReq->idxFmtX11))
        {
            case SHCLX11FMT_UTF8:
                RT_FALL_THROUGH();
            case SHCLX11FMT_TEXT:
            {
                size_t cwDst;
                /* If we are given broken UTF-8, we treat it as Latin1. */ /** @todo BUGBUG Is this acceptable? */
                if (RT_SUCCESS(RTStrValidateEncodingEx((char *)pvSrc, cbSrc, 0)))
                    rc = ShClConvUtf8LFToUtf16CRLF((const char *)pvSrc, cbSrc,
                                                   (PRTUTF16 *)&pvDst, &cwDst);
                else
                    rc = ShClConvLatin1LFToUtf16CRLF((char *)pvSrc, cbSrc,
                                                     (PRTUTF16 *)&pvDst, &cwDst);
                if (RT_SUCCESS(rc))
                {
                    cwDst += 1                        /* Include terminator */;
                    cbDst  = cwDst * sizeof(RTUTF16); /* Convert RTUTF16 units to bytes. */
                }
                break;
            }

            default:
            {
                rc = VERR_INVALID_PARAMETER;
                break;
            }
        }
    }
    else if (pReq->uFmtVBox == VBOX_SHCL_FMT_BITMAP)
    {
        /* In which format is the clipboard data? */
        switch (clipRealFormatForX11Format(pReq->idxFmtX11))
        {
            case SHCLX11FMT_BMP:
            {
                const void *pDib;
                size_t cbDibSize;
                rc = ShClBmpGetDib((const void *)pvSrc, cbSrc,
                                   &pDib, &cbDibSize);
                if (RT_SUCCESS(rc))
                {
                    pvDst = RTMemAlloc(cbDibSize);
                    if (!pvDst)
                        rc = VERR_NO_MEMORY;
                    else
                    {
                        memcpy(pvDst, pDib, cbDibSize);
                        cbDst = cbDibSize;
                    }
                }
                break;
            }

            default:
            {
                rc = VERR_INVALID_PARAMETER;
                break;
            }
        }
    }
    else if (pReq->uFmtVBox == VBOX_SHCL_FMT_HTML)
    {
        /* In which format is the clipboard data? */
        switch (clipRealFormatForX11Format(pReq->idxFmtX11))
        {
            case SHCLX11FMT_HTML:
            {
                /*
                 * The common VBox HTML encoding will be - UTF-8
                 * because it more general for HTML formats then UTF-16
                 * X11 clipboard returns UTF-16, so before sending it we should
                 * convert it to UTF-8.
                 */
                pvDst = NULL;
                cbDst = 0;

                /*
                 * Some applications sends data in UTF-16, some in UTF-8,
                 * without indication it in MIME.
                 *
                 * In case of UTF-16, at least [Open|Libre] Office adds an byte order mark (0xfeff)
                 * at the start of the clipboard data.
                 */
                if (   cbSrc >= sizeof(RTUTF16)
                    && *(PRTUTF16)pvSrc == VBOX_SHCL_UTF16LEMARKER)
                {
                    rc = ShClConvUtf16ToUtf8HTML((PRTUTF16)pvSrc, cbSrc / sizeof(RTUTF16), (char**)&pvDst, &cbDst);
                    if (RT_SUCCESS(rc))
                    {
                        LogFlowFunc(("UTF-16 Unicode source (%u bytes):\n%ls\n\n", cbSrc, pvSrc));
                        LogFlowFunc(("Byte Order Mark = %hx", ((PRTUTF16)pvSrc)[0]));
                        LogFlowFunc(("UTF-8 Unicode dest (%u bytes):\n%s\n\n", cbDst, pvDst));
                    }
                    else
                        LogRel(("Shared Clipboard: Converting UTF-16 Unicode failed with %Rrc\n", rc));
                }
                else /* Raw data. */
                {
                    pvDst = RTMemAllocZ(cbSrc + 1 /* '\0' */);
                    if(pvDst)
                    {
                         memcpy(pvDst, pvSrc, cbSrc);
                         cbDst = cbSrc + 1 /* '\0' */;
                    }
                    else
                    {
                         rc = VERR_NO_MEMORY;
                         break;
                    }
                }

                rc = VINF_SUCCESS;
                break;
            }

            default:
            {
                rc = VERR_INVALID_PARAMETER;
                break;
            }
        }
    }
# ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    else if (pReq->uFmtVBox == VBOX_SHCL_FMT_URI_LIST)
    {
        /* In which format is the clipboard data? */
        switch (clipRealFormatForX11Format(pReq->idxFmtX11))
        {
            case SHCLX11FMT_URI_LIST:
            {
                /* For URI lists we only accept valid UTF-8 encodings. */
                if (RT_SUCCESS(RTStrValidateEncodingEx((char *)pvSrc, cbSrc, 0)))
                {
                    /* URI lists on X are strings separated with "\r\n". */
                    RTCList<RTCString> lstRootEntries = RTCString((char *)pvSrc, cbSrc).split("\r\n");
                    for (size_t i = 0; i < lstRootEntries.size(); ++i)
                    {
                        char *pszEntry = RTUriFilePath(lstRootEntries.at(i).c_str());
                        AssertPtrBreakStmt(pszEntry, VERR_INVALID_PARAMETER);

                        rc = RTStrAAppend((char **)&pvDst, "http://localhost");
                        AssertRCBreakStmt(rc, VERR_NO_MEMORY);
                        cbDst += (uint32_t)strlen(pszEntry);



                        /** @todo BUGBUG Fix port! */
                        /** @todo Add port + UUID (virtual path). */

                        rc = RTStrAAppend((char **)&pvDst, pszEntry);
                        AssertRCBreakStmt(rc, VERR_NO_MEMORY);
                        cbDst += (uint32_t)strlen(pszEntry);

                        LogFlowFunc(("URI list entry '%s'\n", (char *)pvDst));

                        rc = RTStrAAppend((char **)&pvDst, "\r\n");
                        AssertRCBreakStmt(rc, VERR_NO_MEMORY);
                        cbDst += (uint32_t)strlen("\r\n");

                        RTStrFree(pszEntry);
                    }

                    if (cbDst)
                        cbDst++; /* Include final (zero) termination. */

                    LogFlowFunc(("URI list: cbDst=%RU32\n", cbDst));
                }
                else
                    rc = VERR_INVALID_PARAMETER;
                break;
            }

            default:
            {
                rc = VERR_INVALID_PARAMETER;
                break;
            }
        }
    }
# endif /* VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS */
    else
        rc = VERR_NOT_SUPPORTED;

    if (RT_FAILURE(rc))
        LogRel(("Shared Clipboard: Converting X11 format '%s' (idxFmtX11=%u) to VBox format %#x failed, rc=%Rrc\n",
                g_aFormats[pReq->idxFmtX11].pcszAtom, pReq->idxFmtX11, pReq->uFmtVBox, rc));

    SHCLX11READDATAREQ SendData;
    RT_ZERO(SendData);
    SendData.pReq         = pReq->pReq;
    SendData.rcCompletion = rc;

    pCtx->Callbacks.pfnOnSendDataToDest(pReq->pCtx->pFrontend, pvDst, cbDst, &SendData);

    RTMemFree(pvDst);
    RTMemFree(pReq);

    LogFlowFuncLeaveRC(rc);
}

/**
 * Converts the data obtained from the X11 clipboard to the required format,
 * place it in the buffer supplied and signal that data has arrived.
 *
 * Converts the text obtained UTF-16LE with Windows EOLs.
 * Converts full BMP data to DIB format.
 */
SHCL_X11_DECL(void) clipConvertDataFromX11(Widget widget, XtPointer pClient,
                                           Atom * /* selection */, Atom *atomType,
                                           XtPointer pvSrc, long unsigned int *pcLen,
                                           int *piFormat)
{
    RT_NOREF(widget);
    if (*atomType == XT_CONVERT_FAIL) /* Xt timeout */
        clipConvertDataFromX11Worker(pClient, NULL, 0);
    else
    {
        CLIPREADX11CBREQ *pReq = (CLIPREADX11CBREQ *)pClient;
        if (pReq->pCtx->Callbacks.pfnOnClipboardRead)
        {
            void  *pvData = NULL;
            size_t cbData = 0;
            int rc = pReq->pCtx->Callbacks.pfnOnClipboardRead(pReq->pCtx->pFrontend, pReq->uFmtVBox, &pvData, &cbData, NULL);
            if (RT_SUCCESS(rc))
            {
                /* Feed to conversion worker. */
                clipConvertDataFromX11Worker(pClient, pvData, cbData);
                RTMemFree(pvData);
            }
            else
                clipConvertDataFromX11Worker(pClient, NULL, 0);
        }
        else /* Call with current data provided by X (default). */
            clipConvertDataFromX11Worker(pClient, pvSrc, (*pcLen) * (*piFormat) / 8);
    }

    XtFree((char *)pvSrc);
}

static int clipGetSelectionValue(PSHCLX11CTX pCtx, SHCLX11FMTIDX idxFmt,
                                 CLIPREADX11CBREQ *pReq)
{
#ifndef TESTCASE
    XtGetSelectionValue(pCtx->pWidget, clipGetAtom(pCtx, "CLIPBOARD"),
                        clipAtomForX11Format(pCtx, idxFmt),
                        clipConvertDataFromX11,
                        reinterpret_cast<XtPointer>(pReq),
                        CurrentTime);
#else
    tstClipRequestData(pCtx, idxFmt, (void *)pReq);
#endif

    return VINF_SUCCESS; /** @todo Return real rc. */
}

/**
 * Worker function for ShClX11ReadDataFromX11 which runs on the event thread.
 *
 * @param pvUserData            Pointer to a CLIPREADX11CBREQ structure containing
 *                              information about the clipboard read request.
 *                              Must be free'd by the worker.
 */
static void ShClX11ReadDataFromX11Worker(void *pvUserData, void * /* interval */)
{
    AssertPtrReturnVoid(pvUserData);

    CLIPREADX11CBREQ *pReq = (CLIPREADX11CBREQ *)pvUserData;
    SHCLX11CTX       *pCtx = pReq->pCtx;
    AssertPtrReturnVoid(pCtx);

    LogFlowFunc(("pReq->uFmtVBox=%#x, idxFmtX11=%#x\n", pReq->uFmtVBox, pReq->idxFmtX11));

    int rc = VERR_NO_DATA; /* VBox thinks we have data and we don't. */

#ifdef VBOX_WITH_SHARED_CLIPBOARD_XT_BUSY
    const bool fXtBusy = clipGetXtBusy(pCtx);
    clipSetXtBusy(pCtx, true);
    if (fXtBusy)
    {
        /* If the clipboard is busy just fend off the request. */
        rc = VERR_TRY_AGAIN;
    }
    else
#endif
    if (pReq->uFmtVBox & VBOX_SHCL_FMT_UNICODETEXT)
    {
        pReq->idxFmtX11 = pCtx->idxFmtText;
        if (pReq->idxFmtX11 != SHCLX11FMT_INVALID)
        {
            /* Send out a request for the data to the current clipboard owner. */
            rc = clipGetSelectionValue(pCtx, pCtx->idxFmtText, pReq);
        }
    }
    else if (pReq->uFmtVBox & VBOX_SHCL_FMT_BITMAP)
    {
        pReq->idxFmtX11 = pCtx->idxFmtBmp;
        if (pReq->idxFmtX11 != SHCLX11FMT_INVALID)
        {
            /* Send out a request for the data to the current clipboard owner. */
            rc = clipGetSelectionValue(pCtx, pCtx->idxFmtBmp, pReq);
        }
    }
    else if (pReq->uFmtVBox & VBOX_SHCL_FMT_HTML)
    {
        pReq->idxFmtX11 = pCtx->idxFmtHTML;
        if (pReq->idxFmtX11 != SHCLX11FMT_INVALID)
        {
            /* Send out a request for the data to the current clipboard owner. */
            rc = clipGetSelectionValue(pCtx, pCtx->idxFmtHTML, pReq);
        }
    }
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    else if (pReq->uFmtVBox & VBOX_SHCL_FMT_URI_LIST)
    {
        pReq->idxFmtX11 = pCtx->idxFmtURI;
        if (pReq->idxFmtX11 != SHCLX11FMT_INVALID)
        {
            /* Send out a request for the data to the current clipboard owner. */
            rc = clipGetSelectionValue(pCtx, pCtx->idxFmtURI, pReq);
        }
    }
#endif
    else
    {
#ifdef VBOX_WITH_SHARED_CLIPBOARD_XT_BUSY
        clipSetXtBusy(pCtx, false);
#endif
        rc = VERR_NOT_IMPLEMENTED;
    }

    if (RT_FAILURE(rc))
    {
        /* The clipboard callback was never scheduled, so we must signal
         * that the request processing is finished and clean up ourselves. */
        SHCLX11READDATAREQ SendData;
        RT_ZERO(SendData);
        SendData.pReq         = pReq->pReq;
        SendData.rcCompletion = rc;

        pCtx->Callbacks.pfnOnSendDataToDest(pReq->pCtx->pFrontend, NULL /* pv */ ,0 /* cb */, &SendData);
        RTMemFree(pReq);
    }

    LogFlowFuncLeaveRC(rc);
}

/**
 * Called when VBox wants to read the X11 clipboard.
 *
 * @returns VBox status code.
 * @retval  VERR_NO_DATA if format is supported but no data is available currently.
 * @retval  VERR_NOT_IMPLEMENTED if the format is not implemented.
 * @param   pCtx                Context data for the clipboard backend.
 * @param   uFmt                The format that the VBox would like to receive the data in.
 * @param   pReq                Read callback request to use. Will be free'd in the callback on success.
 *                              Otherwise the caller has to free this again on error.
 *
 * @note   We allocate a request structure which must be freed by the worker.
 */
int ShClX11ReadDataFromX11(PSHCLX11CTX pCtx, SHCLFORMAT uFmt, CLIPREADCBREQ *pReq)
{
    AssertPtrReturn(pReq, VERR_INVALID_POINTER);
    /*
     * Immediately return if we are not connected to the X server.
     */
    if (!pCtx->fHaveX11)
        return VERR_NO_DATA;

    int rc = VINF_SUCCESS;

    CLIPREADX11CBREQ *pX11Req = (CLIPREADX11CBREQ *)RTMemAllocZ(sizeof(CLIPREADX11CBREQ));
    if (pX11Req)
    {
        pX11Req->pCtx     = pCtx;
        pX11Req->uFmtVBox = uFmt;
        pX11Req->pReq     = pReq;

        /* We use this to schedule a worker function on the event thread. */
        rc = clipThreadScheduleCall(pCtx, ShClX11ReadDataFromX11Worker, (XtPointer)pX11Req);
        if (RT_FAILURE(rc))
            RTMemFree(pX11Req);
    }
    else
        rc = VERR_NO_MEMORY;

    LogFlowFuncLeaveRC(rc);
    return rc;
}
