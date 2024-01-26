/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef DIX_H
#define DIX_H

#include "gc.h"
#include "window.h"
#include "input.h"

#define EARLIER -1
#define SAMETIME 0
#define LATER 1

#define NullClient ((ClientPtr) 0)
#define REQUEST(type) \
	register type *stuff = (type *)client->requestBuffer


#define REQUEST_SIZE_MATCH(req)\
    if ((sizeof(req) >> 2) != client->req_len)\
         return(BadLength)

#define REQUEST_AT_LEAST_SIZE(req) \
    if ((sizeof(req) >> 2) > client->req_len )\
         return(BadLength)

#define REQUEST_FIXED_SIZE(req, n)\
    if (((sizeof(req) >> 2) > client->req_len) || \
        (((sizeof(req) + (n) + 3) >> 2) != client->req_len)) \
         return(BadLength)

#define LEGAL_NEW_RESOURCE(id,client)\
    if (!LegalNewID(id,client)) \
    {\
	client->errorValue = id;\
        return(BadIDChoice);\
    }

#define VALIDATE_DRAWABLE_AND_GC(drawID, pDraw, mode)\
    {\
	int rc = dixLookupDrawable(&(pDraw), drawID, client, M_ANY, mode);\
	if (rc != Success)\
	    return rc;\
	rc = dixLookupGC(&(pGC), stuff->gc, client, DixUseAccess);\
	if (rc != Success)\
	    return rc;\
	if ((pGC->depth != pDraw->depth) || (pGC->pScreen != pDraw->pScreen))\
	    return (BadMatch);\
    }\
    if (pGC->serialNumber != pDraw->serialNumber)\
	ValidateGC(pDraw, pGC);


#define WriteReplyToClient(pClient, size, pReply) { \
   if ((pClient)->swapped) \
      (*ReplySwapVector[((xReq *)(pClient)->requestBuffer)->reqType]) \
           (pClient, (int)(size), pReply); \
      else (void) WriteToClient(pClient, (int)(size), (char *)(pReply)); }

#define WriteSwappedDataToClient(pClient, size, pbuf) \
   if ((pClient)->swapped) \
      (*(pClient)->pSwapReplyFunc)(pClient, (int)(size), pbuf); \
   else (void) WriteToClient (pClient, (int)(size), (char *)(pbuf));

typedef struct _TimeStamp *TimeStampPtr;

#ifndef _XTYPEDEF_CLIENTPTR
typedef struct _Client *ClientPtr; /* also in misc.h */
#define _XTYPEDEF_CLIENTPTR
#endif

typedef struct _WorkQueue	*WorkQueuePtr;

#ifdef XPRINT
extern ClientPtr requestingClient;
#endif
extern ClientPtr *clients;
extern ClientPtr serverClient;
extern int currentMaxClients;
extern char dispatchExceptionAtReset;

typedef int HWEventQueueType;
typedef HWEventQueueType* HWEventQueuePtr;

extern HWEventQueuePtr checkForInput[2];

typedef struct _TimeStamp {
    CARD32 months;	/* really ~49.7 days */
    CARD32 milliseconds;
}           TimeStamp;

/* dispatch.c */

extern void SetInputCheck(
    HWEventQueuePtr /*c0*/,
    HWEventQueuePtr /*c1*/);

extern void CloseDownClient(
    ClientPtr /*client*/);

extern void UpdateCurrentTime(void);

extern void UpdateCurrentTimeIf(void);

extern int dixDestroyPixmap(
    pointer /*value*/,
    XID /*pid*/);

extern void InitClient(
    ClientPtr /*client*/,
    int /*i*/,
    pointer /*ospriv*/);

extern ClientPtr NextAvailableClient(
    pointer /*ospriv*/);

extern void SendErrorToClient(
    ClientPtr /*client*/,
    unsigned int /*majorCode*/,
    unsigned int /*minorCode*/,
    XID /*resId*/,
    int /*errorCode*/);

extern void MarkClientException(
    ClientPtr /*client*/);

#if defined(DDXBEFORERESET)
extern void ddxBeforeReset (void);
#endif

/* dixutils.c */

extern void CopyISOLatin1Lowered(
    unsigned char * /*dest*/,
    unsigned char * /*source*/,
    int /*length*/);

extern int CompareISOLatin1Lowered(
    unsigned char * /*a*/,
    int alen,
    unsigned char * /*b*/,
    int blen);

extern int dixLookupWindow(
    WindowPtr *result,
    XID id,
    ClientPtr client,
    Mask access_mode);

extern int dixLookupDrawable(
    DrawablePtr *result,
    XID id,
    ClientPtr client,
    Mask type_mask,
    Mask access_mode);

extern int dixLookupGC(
    GCPtr *result,
    XID id,
    ClientPtr client,
    Mask access_mode);

extern int dixLookupClient(
    ClientPtr *result,
    XID id,
    ClientPtr client,
    Mask access_mode);

extern void NoopDDA(void);

extern int AlterSaveSetForClient(
    ClientPtr /*client*/,
    WindowPtr /*pWin*/,
    unsigned /*mode*/,
    Bool /*toRoot*/,
    Bool /*remap*/);
  
extern void DeleteWindowFromAnySaveSet(
    WindowPtr /*pWin*/);

extern void BlockHandler(
    pointer /*pTimeout*/,
    pointer /*pReadmask*/);

extern void WakeupHandler(
    int /*result*/,
    pointer /*pReadmask*/);

typedef void (* WakeupHandlerProcPtr)(
    pointer /* blockData */,
    int /* result */,
    pointer /* pReadmask */);

extern Bool RegisterBlockAndWakeupHandlers(
    BlockHandlerProcPtr /*blockHandler*/,
    WakeupHandlerProcPtr /*wakeupHandler*/,
    pointer /*blockData*/);

extern void RemoveBlockAndWakeupHandlers(
    BlockHandlerProcPtr /*blockHandler*/,
    WakeupHandlerProcPtr /*wakeupHandler*/,
    pointer /*blockData*/);

extern void InitBlockAndWakeupHandlers(void);

extern void ProcessWorkQueue(void);

extern void ProcessWorkQueueZombies(void);

extern Bool QueueWorkProc(
    Bool (* /*function*/)(
        ClientPtr /*clientUnused*/,
        pointer /*closure*/),
    ClientPtr /*client*/,
    pointer /*closure*/
);

typedef Bool (* ClientSleepProcPtr)(
    ClientPtr /*client*/,
    pointer /*closure*/);

extern Bool ClientSleep(
    ClientPtr /*client*/,
    ClientSleepProcPtr /* function */,
    pointer /*closure*/);

#ifndef ___CLIENTSIGNAL_DEFINED___
#define ___CLIENTSIGNAL_DEFINED___
extern Bool ClientSignal(
    ClientPtr /*client*/);
#endif /* ___CLIENTSIGNAL_DEFINED___ */

extern void ClientWakeup(
    ClientPtr /*client*/);

extern Bool ClientIsAsleep(
    ClientPtr /*client*/);

/* atom.c */

extern Atom MakeAtom(
    char * /*string*/,
    unsigned /*len*/,
    Bool /*makeit*/);

extern Bool ValidAtom(
    Atom /*atom*/);

extern char *NameForAtom(
    Atom /*atom*/);

extern void AtomError(void);

extern void FreeAllAtoms(void);

extern void InitAtoms(void);

/* main.c */

extern void SetVendorRelease(int release);

extern void SetVendorString(char *string);

/* events.c */

extern void SetMaskForEvent(
    Mask /* mask */,
    int /* event */);


extern Bool IsParent(
    WindowPtr /* maybeparent */,
    WindowPtr /* child */);

extern WindowPtr GetCurrentRootWindow(void);

extern WindowPtr GetSpriteWindow(void);


extern void NoticeEventTime(xEventPtr /* xE */);

extern void EnqueueEvent(
    xEventPtr /* xE */,
    DeviceIntPtr /* device */,
    int	/* count */);

extern void ActivatePointerGrab(
    DeviceIntPtr /* mouse */,
    GrabPtr /* grab */,
    TimeStamp /* time */,
    Bool /* autoGrab */);

extern void DeactivatePointerGrab(
    DeviceIntPtr /* mouse */);

extern void ActivateKeyboardGrab(
    DeviceIntPtr /* keybd */,
    GrabPtr /* grab */,
    TimeStamp /* time */,
    Bool /* passive */);

extern void DeactivateKeyboardGrab(
    DeviceIntPtr /* keybd */);

extern void AllowSome(
    ClientPtr	/* client */,
    TimeStamp /* time */,
    DeviceIntPtr /* thisDev */,
    int /* newState */);

extern void ReleaseActiveGrabs(
    ClientPtr client);

extern int DeliverEventsToWindow(
    WindowPtr /* pWin */,
    xEventPtr /* pEvents */,
    int /* count */,
    Mask /* filter */,
    GrabPtr /* grab */,
    int /* mskidx */);

extern int DeliverDeviceEvents(
    WindowPtr /* pWin */,
    xEventPtr /* xE */,
    GrabPtr /* grab */,
    WindowPtr /* stopAt */,
    DeviceIntPtr /* dev */,
    int /* count */);

extern void DefineInitialRootWindow(
    WindowPtr /* win */);

extern void UpdateSpriteForScreen(
     ScreenPtr /* pScreen */);

extern void WindowHasNewCursor(
    WindowPtr /* pWin */);

extern Bool CheckDeviceGrabs(
    DeviceIntPtr /* device */,
    xEventPtr /* xE */,
    int /* checkFirst */,
    int /* count */);

extern void DeliverFocusedEvent(
    DeviceIntPtr /* keybd */,
    xEventPtr /* xE */,
    WindowPtr /* window */,
    int /* count */);

extern void DeliverGrabbedEvent(
    xEventPtr /* xE */,
    DeviceIntPtr /* thisDev */,
    Bool /* deactivateGrab */,
    int /* count */);

#ifdef XKB
extern void FixKeyState(
    xEvent * /* xE */,
    DeviceIntPtr /* keybd */);
#endif /* XKB */

extern void RecalculateDeliverableEvents(
    WindowPtr /* pWin */);

extern int OtherClientGone(
    pointer /* value */,
    XID /* id */);

extern void DoFocusEvents(
    DeviceIntPtr /* dev */,
    WindowPtr /* fromWin */,
    WindowPtr /* toWin */,
    int /* mode */);

extern int SetInputFocus(
    ClientPtr /* client */,
    DeviceIntPtr /* dev */,
    Window /* focusID */,
    CARD8 /* revertTo */,
    Time /* ctime */,
    Bool /* followOK */);

extern int GrabDevice(
    ClientPtr /* client */,
    DeviceIntPtr /* dev */,
    unsigned /* this_mode */,
    unsigned /* other_mode */,
    Window /* grabWindow */,
    unsigned /* ownerEvents */,
    Time /* ctime */,
    Mask /* mask */,
    CARD8 * /* status */);

extern void InitEvents(void);

extern void CloseDownEvents(void);

extern void DeleteWindowFromAnyEvents(
    WindowPtr	/* pWin */,
    Bool /* freeResources */);


extern Mask EventMaskForClient(
    WindowPtr /* pWin */,
    ClientPtr /* client */);



extern int DeliverEvents(
    WindowPtr /*pWin*/,
    xEventPtr /*xE*/,
    int /*count*/,
    WindowPtr /*otherParent*/);


extern void WriteEventsToClient(
    ClientPtr /*pClient*/,
    int	     /*count*/,
    xEventPtr /*events*/);

extern int TryClientEvents(
    ClientPtr /*client*/,
    xEventPtr /*pEvents*/,
    int /*count*/,
    Mask /*mask*/,
    Mask /*filter*/,
    GrabPtr /*grab*/);

extern void WindowsRestructured(void);

#ifdef PANORAMIX
extern void ReinitializeRootWindow(WindowPtr win, int xoff, int yoff);
#endif

#ifdef RANDR
void
ScreenRestructured (ScreenPtr pScreen);
#endif

extern int ffs(int i);

/*
 *  callback manager stuff
 */

#ifndef _XTYPEDEF_CALLBACKLISTPTR
typedef struct _CallbackList *CallbackListPtr; /* also in misc.h */
#define _XTYPEDEF_CALLBACKLISTPTR
#endif

typedef void (*CallbackProcPtr) (
    CallbackListPtr *, pointer, pointer);

extern Bool AddCallback(
    CallbackListPtr * /*pcbl*/,
    CallbackProcPtr /*callback*/,
    pointer /*data*/);

extern Bool DeleteCallback(
    CallbackListPtr * /*pcbl*/,
    CallbackProcPtr /*callback*/,
    pointer /*data*/);

extern void CallCallbacks(
    CallbackListPtr * /*pcbl*/,
    pointer /*call_data*/);

extern void DeleteCallbackList(
    CallbackListPtr * /*pcbl*/);

extern void InitCallbackManager(void);

/*
 *  ServerGrabCallback stuff
 */

extern CallbackListPtr ServerGrabCallback;

typedef enum {SERVER_GRABBED, SERVER_UNGRABBED,
	      CLIENT_PERVIOUS, CLIENT_IMPERVIOUS } ServerGrabState;

typedef struct {
    ClientPtr client;
    ServerGrabState grabstate;
} ServerGrabInfoRec;

/*
 *  EventCallback stuff
 */

extern CallbackListPtr EventCallback;

typedef struct {
    ClientPtr client;
    xEventPtr events;
    int count;
} EventInfoRec;

/*
 *  DeviceEventCallback stuff
 */

extern CallbackListPtr DeviceEventCallback;

typedef struct {
    xEventPtr events;
    int count;
} DeviceEventInfoRec;

/* strcasecmp.c */
#if NEED_STRCASECMP
#define strcasecmp xstrcasecmp
extern int xstrcasecmp(const char *s1, const char *s2);
#endif

#if NEED_STRNCASECMP
#define strncasecmp xstrncasecmp
extern int xstrncasecmp(const char *s1, const char *s2, size_t n);
#endif

#if NEED_STRCASESTR
#define strcasestr xstrcasestr
extern char *xstrcasestr(const char *s, const char *find);
#endif

/*
 * These are deprecated compatibility functions and will be removed soon!
 * Please use the noted replacements instead.
 */

/* replaced by dixLookupWindow */
extern WindowPtr SecurityLookupWindow(
    XID id,
    ClientPtr client,
    Mask access_mode);

/* replaced by dixLookupWindow */
extern WindowPtr LookupWindow(
    XID id,
    ClientPtr client);

/* replaced by dixLookupDrawable */
extern pointer SecurityLookupDrawable(
    XID id,
    ClientPtr client,
    Mask access_mode);

/* replaced by dixLookupDrawable */
extern pointer LookupDrawable(
    XID id,
    ClientPtr client);

/* replaced by dixLookupClient */
extern ClientPtr LookupClient(
    XID id,
    ClientPtr client);

#endif /* DIX_H */
