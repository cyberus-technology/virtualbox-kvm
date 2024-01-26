/* $Id: seamless-x11.cpp $ */
/** @file
 * X11 Seamless mode.
 */

/*
 * Copyright (C) 2008-2023 Oracle and/or its affiliates.
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
*   Header files                                                                                                                 *
*********************************************************************************************************************************/

#include <iprt/errcore.h>
#include <iprt/assert.h>
#include <iprt/vector.h>
#include <iprt/thread.h>
#include <VBox/log.h>

#include "seamless-x11.h"
#include "VBoxClient.h"

#include <X11/Xatom.h>
#include <X11/Xmu/WinUtil.h>

#include <limits.h>

#ifdef TESTCASE
#undef DefaultRootWindow
#define DefaultRootWindow XDefaultRootWindow
#endif

/*****************************************************************************
* Static functions                                                           *
*****************************************************************************/

static unsigned char *XXGetProperty(Display *aDpy, Window aWnd, Atom aPropType,
                                    const char *aPropName, unsigned long *nItems)
{
    LogRelFlowFuncEnter();
    Atom propNameAtom = XInternAtom (aDpy, aPropName,
                                     True /* only_if_exists */);
    if (propNameAtom == None)
    {
        return NULL;
    }

    Atom actTypeAtom = None;
    int actFmt = 0;
    unsigned long nBytesAfter = 0;
    unsigned char *propVal = 0;
    int rc = XGetWindowProperty (aDpy, aWnd, propNameAtom,
                                 0, LONG_MAX, False /* delete */,
                                 aPropType, &actTypeAtom, &actFmt,
                                 nItems, &nBytesAfter, &propVal);
    if (rc != Success)
        return NULL;

    LogRelFlowFuncLeave();
    return propVal;
}

int SeamlessX11::init(PFNSENDREGIONUPDATE pHostCallback)
{
    int rc = VINF_SUCCESS;

    LogRelFlowFuncEnter();
    if (mHostCallback != NULL)  /* Assertion */
    {
        VBClLogError("Attempting to initialise seamless guest object twice!\n");
        return VERR_INTERNAL_ERROR;
    }
    if (!(mDisplay = XOpenDisplay(NULL)))
    {
        VBClLogError("Seamless guest object failed to acquire a connection to the display\n");
        return VERR_ACCESS_DENIED;
    }
    mHostCallback = pHostCallback;
    mEnabled = false;
    unmonitorClientList();
    LogRelFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Shutdown seamless event monitoring.
 */
void SeamlessX11::uninit(void)
{
    if (mHostCallback)
        stop();
    mHostCallback = NULL;

    /* Before closing a Display, make sure X11 is still running. The indicator
     * that is when XOpenDisplay() returns non NULL. If it is not a
     * case, XCloseDisplay() will hang on internal X11 mutex forever. */
    Display *pDisplay = XOpenDisplay(NULL);
    if (pDisplay)
    {
        XCloseDisplay(pDisplay);
        if (mDisplay)
        {
            XCloseDisplay(mDisplay);
            mDisplay = NULL;
        }
    }

    if (mpRects)
    {
        RTMemFree(mpRects);
        mpRects = NULL;
    }
}

/**
 * Read information about currently visible windows in the guest and subscribe to X11
 * events about changes to this information.
 *
 * @note This class does not contain its own event thread, so an external thread must
 *       call nextConfigurationEvent() for as long as events are wished.
 * @todo This function should switch the guest to fullscreen mode.
 */
int SeamlessX11::start(void)
{
    int rc = VINF_SUCCESS;
    /** Dummy values for XShapeQueryExtension */
    int error, event;

    LogRelFlowFuncEnter();
    if (mEnabled)
        return VINF_SUCCESS;
    mSupportsShape = XShapeQueryExtension(mDisplay, &event, &error);
    mEnabled = true;
    monitorClientList();
    rebuildWindowTree();
    LogRelFlowFuncLeaveRC(rc);
    return rc;
}

/** Stop reporting seamless events to the host.  Free information about guest windows
    and stop requesting updates. */
void SeamlessX11::stop(void)
{
    LogRelFlowFuncEnter();
    if (!mEnabled)
        return;
    mEnabled = false;
    unmonitorClientList();
    freeWindowTree();
    LogRelFlowFuncLeave();
}

void SeamlessX11::monitorClientList(void)
{
    LogRelFlowFuncEnter();
    XSelectInput(mDisplay, DefaultRootWindow(mDisplay), PropertyChangeMask | SubstructureNotifyMask);
}

void SeamlessX11::unmonitorClientList(void)
{
    LogRelFlowFuncEnter();
    XSelectInput(mDisplay, DefaultRootWindow(mDisplay), PropertyChangeMask);
}

/**
 * Recreate the table of toplevel windows of clients on the default root window of the
 * X server.
 */
void SeamlessX11::rebuildWindowTree(void)
{
    LogRelFlowFuncEnter();
    freeWindowTree();
    addClients(DefaultRootWindow(mDisplay));
    mChanged = true;
}


/**
 * Look at the list of children of a virtual root window and add them to the list of clients
 * if they belong to a client which is not a virtual root.
 *
 * @param hRoot the virtual root window to be examined
 */
void SeamlessX11::addClients(const Window hRoot)
{
    /** Unused out parameters of XQueryTree */
    Window hRealRoot, hParent;
    /** The list of children of the root supplied, raw pointer */
    Window *phChildrenRaw = NULL;
    /** The list of children of the root supplied, auto-pointer */
    Window *phChildren;
    /** The number of children of the root supplied */
    unsigned cChildren;

    LogRelFlowFuncEnter();
    if (!XQueryTree(mDisplay, hRoot, &hRealRoot, &hParent, &phChildrenRaw, &cChildren))
        return;
    phChildren = phChildrenRaw;
    for (unsigned i = 0; i < cChildren; ++i)
        addClientWindow(phChildren[i]);
    XFree(phChildrenRaw);
    LogRelFlowFuncLeave();
}


void SeamlessX11::addClientWindow(const Window hWin)
{
    LogRelFlowFuncEnter();
    XWindowAttributes winAttrib;
    bool fAddWin = true;
    Window hClient = XmuClientWindow(mDisplay, hWin);

    if (isVirtualRoot(hClient))
        fAddWin = false;
    if (fAddWin && !XGetWindowAttributes(mDisplay, hWin, &winAttrib))
    {
        VBClLogError("Failed to get the window attributes for window %d\n", hWin);
        fAddWin = false;
    }
    if (fAddWin && (winAttrib.map_state == IsUnmapped))
        fAddWin = false;
    XSizeHints dummyHints;
    long dummyLong;
    /* Apparently (?) some old kwin versions had unwanted client windows
     * without normal hints. */
    if (fAddWin && (!XGetWMNormalHints(mDisplay, hClient, &dummyHints,
                                       &dummyLong)))
    {
        LogRelFlowFunc(("window %lu, client window %lu has no size hints\n", hWin, hClient));
        fAddWin = false;
    }
    if (fAddWin)
    {
        XRectangle *pRects = NULL;
        int cRects = 0, iOrdering;
        bool hasShape = false;

        LogRelFlowFunc(("adding window %lu, client window %lu\n", hWin,
                     hClient));
        if (mSupportsShape)
        {
            XShapeSelectInput(mDisplay, hWin, ShapeNotifyMask);
            pRects = XShapeGetRectangles(mDisplay, hWin, ShapeBounding, &cRects, &iOrdering);
            if (!pRects)
                cRects = 0;
            else
            {
                if (   (cRects > 1)
                    || (pRects[0].x != 0)
                    || (pRects[0].y != 0)
                    || (pRects[0].width != winAttrib.width)
                    || (pRects[0].height != winAttrib.height)
                   )
                    hasShape = true;
            }
        }
        mGuestWindows.addWindow(hWin, hasShape, winAttrib.x, winAttrib.y,
                                winAttrib.width, winAttrib.height, cRects,
                                pRects);
    }
    LogRelFlowFuncLeave();
}


/**
 * Checks whether a window is a virtual root.
 * @returns true if it is, false otherwise
 * @param hWin the window to be examined
 */
bool SeamlessX11::isVirtualRoot(Window hWin)
{
    unsigned char *windowTypeRaw = NULL;
    Atom *windowType;
    unsigned long ulCount;
    bool rc = false;

    LogRelFlowFuncEnter();
    windowTypeRaw = XXGetProperty(mDisplay, hWin, XA_ATOM, WM_TYPE_PROP, &ulCount);
    if (windowTypeRaw != NULL)
    {
        windowType = (Atom *)(windowTypeRaw);
        if (   (ulCount != 0)
            && (*windowType == XInternAtom(mDisplay, WM_TYPE_DESKTOP_PROP, True)))
            rc = true;
    }
    if (windowTypeRaw)
        XFree(windowTypeRaw);
    LogRelFlowFunc(("returning %RTbool\n", rc));
    return rc;
}

DECLCALLBACK(int) VBoxGuestWinFree(VBoxGuestWinInfo *pInfo, void *pvParam)
{
    Display *pDisplay = (Display *)pvParam;

    XShapeSelectInput(pDisplay, pInfo->Core.Key, 0);
    delete pInfo;
    return VINF_SUCCESS;
}

/**
 * Free all information in the tree of visible windows
 */
void SeamlessX11::freeWindowTree(void)
{
    /* We use post-increment in the operation to prevent the iterator from being invalidated. */
    LogRelFlowFuncEnter();
    mGuestWindows.detachAll(VBoxGuestWinFree, mDisplay);
    LogRelFlowFuncLeave();
}


/**
 * Waits for a position or shape-related event from guest windows
 *
 * @note Called from the guest event thread.
 */
void SeamlessX11::nextConfigurationEvent(void)
{
    XEvent event;

    LogRelFlowFuncEnter();
    /* Start by sending information about the current window setup to the host.  We do this
       here because we want to send all such information from a single thread. */
    if (mChanged && mEnabled)
    {
        updateRects();
        mHostCallback(mpRects, mcRects);
    }
    mChanged = false;

    if (XPending(mDisplay) > 0)
    {
        /* We execute this even when seamless is disabled, as it also waits for
         * enable and disable notification. */
        XNextEvent(mDisplay, &event);
    } else
    {
        /* This function is called in a loop by upper layer. In order to
         * prevent CPU spinning, sleep a bit before returning. */
        RTThreadSleep(300 /* ms */);
        return;
    }

    if (!mEnabled)
        return;
    switch (event.type)
    {
    case ConfigureNotify:
        {
            XConfigureEvent *pConf = &event.xconfigure;
            LogRelFlowFunc(("configure event, window=%lu, x=%i, y=%i, w=%i, h=%i, send_event=%RTbool\n",
                           (unsigned long) pConf->window, (int) pConf->x,
                           (int) pConf->y, (int) pConf->width,
                           (int) pConf->height, pConf->send_event));
        }
        doConfigureEvent(event.xconfigure.window);
        break;
    case MapNotify:
        LogRelFlowFunc(("map event, window=%lu, send_event=%RTbool\n",
                       (unsigned long) event.xmap.window,
                       event.xmap.send_event));
        rebuildWindowTree();
        break;
    case PropertyNotify:
        if (   event.xproperty.atom != XInternAtom(mDisplay, "_NET_CLIENT_LIST", True /* only_if_exists */)
            || event.xproperty.window != DefaultRootWindow(mDisplay))
            break;
        LogRelFlowFunc(("_NET_CLIENT_LIST property event on root window\n"));
        rebuildWindowTree();
        break;
    case VBoxShapeNotify:  /* This is defined wrong in my X11 header files! */
        LogRelFlowFunc(("shape event, window=%lu, send_event=%RTbool\n",
                       (unsigned long) event.xany.window,
                       event.xany.send_event));
    /* the window member in xany is in the same place as in the shape event */
        doShapeEvent(event.xany.window);
        break;
    case UnmapNotify:
        LogRelFlowFunc(("unmap event, window=%lu, send_event=%RTbool\n",
                       (unsigned long) event.xunmap.window,
                       event.xunmap.send_event));
        rebuildWindowTree();
        break;
    default:
        break;
    }
    LogRelFlowFunc(("processed event\n"));
}

/**
 * Handle a configuration event in the seamless event thread by setting the new position.
 *
 * @param hWin the window to be examined
 */
void SeamlessX11::doConfigureEvent(Window hWin)
{
    VBoxGuestWinInfo *pInfo = mGuestWindows.find(hWin);
    if (pInfo)
    {
        XWindowAttributes winAttrib;

        if (!XGetWindowAttributes(mDisplay, hWin, &winAttrib))
            return;
        pInfo->mX = winAttrib.x;
        pInfo->mY = winAttrib.y;
        pInfo->mWidth = winAttrib.width;
        pInfo->mHeight = winAttrib.height;
        mChanged = true;
    }
}

/**
 * Handle a window shape change event in the seamless event thread.
 *
 * @param hWin the window to be examined
 */
void SeamlessX11::doShapeEvent(Window hWin)
{
    LogRelFlowFuncEnter();
    VBoxGuestWinInfo *pInfo = mGuestWindows.find(hWin);
    if (pInfo)
    {
        XRectangle *pRects;
        int cRects = 0, iOrdering;

        pRects = XShapeGetRectangles(mDisplay, hWin, ShapeBounding, &cRects,
                                     &iOrdering);
        if (!pRects)
            cRects = 0;
        pInfo->mhasShape = true;
        if (pInfo->mpRects)
            XFree(pInfo->mpRects);
        pInfo->mcRects = cRects;
        pInfo->mpRects = pRects;
        mChanged = true;
    }
    LogRelFlowFuncLeave();
}

/**
 * Gets the list of visible rectangles
 */
RTRECT *SeamlessX11::getRects(void)
{
    return mpRects;
}

/**
 * Gets the number of rectangles in the visible rectangle list
 */
size_t SeamlessX11::getRectCount(void)
{
    return mcRects;
}

RTVEC_DECL(RectList, RTRECT)

static DECLCALLBACK(int) getRectsCallback(VBoxGuestWinInfo *pInfo, struct RectList *pRects)
{
    if (pInfo->mhasShape)
    {
        for (int i = 0; i < pInfo->mcRects; ++i)
        {
            RTRECT *pRect;

            pRect = RectListPushBack(pRects);
            if (!pRect)
                return VERR_NO_MEMORY;
            pRect->xLeft   =   pInfo->mX
                             + pInfo->mpRects[i].x;
            pRect->yBottom =   pInfo->mY
                             + pInfo->mpRects[i].y
                             + pInfo->mpRects[i].height;
            pRect->xRight  =   pInfo->mX
                             + pInfo->mpRects[i].x
                             + pInfo->mpRects[i].width;
            pRect->yTop    =   pInfo->mY
                             + pInfo->mpRects[i].y;
        }
    }
    else
    {
        RTRECT *pRect;

        pRect = RectListPushBack(pRects);
        if (!pRect)
            return VERR_NO_MEMORY;
        pRect->xLeft   =  pInfo->mX;
        pRect->yBottom =  pInfo->mY
                        + pInfo->mHeight;
        pRect->xRight  =  pInfo->mX
                        + pInfo->mWidth;
        pRect->yTop    =  pInfo->mY;
    }
    return VINF_SUCCESS;
}

/**
 * Updates the list of seamless rectangles
 */
int SeamlessX11::updateRects(void)
{
    LogRelFlowFuncEnter();
    struct RectList rects = RTVEC_INITIALIZER;

    if (mcRects != 0)
    {
        int rc = RectListReserve(&rects, mcRects * 2);
        if (RT_FAILURE(rc))
            return rc;
    }
    mGuestWindows.doWithAll((PFNVBOXGUESTWINCALLBACK)getRectsCallback, &rects);
    if (mpRects)
        RTMemFree(mpRects);
    mcRects = RectListSize(&rects);
    mpRects = RectListDetach(&rects);
    LogRelFlowFuncLeave();
    return VINF_SUCCESS;
}

/**
 * Send a client event to wake up the X11 seamless event loop prior to stopping it.
 *
 * @note This function should only be called from the host event thread.
 */
bool SeamlessX11::interruptEventWait(void)
{
    bool rc = false;
    Display *pDisplay = XOpenDisplay(NULL);

    LogRelFlowFuncEnter();
    if (pDisplay == NULL)
    {
        VBClLogError("Failed to open X11 display\n");
        return false;
    }

    /* Message contents set to zero. */
    XClientMessageEvent clientMessage =
        { ClientMessage, 0, 0, 0, 0, XInternAtom(pDisplay, "VBOX_CLIENT_SEAMLESS_HEARTBEAT", false), 8 };

    if (XSendEvent(pDisplay, DefaultRootWindow(mDisplay), false,
                   PropertyChangeMask, (XEvent *)&clientMessage))
        rc = true;
    XCloseDisplay(pDisplay);
    LogRelFlowFunc(("returning %RTbool\n", rc));
    return rc;
}
