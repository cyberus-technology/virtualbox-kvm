/*

Copyright 2007 Peter Hutterer <peter@cs.unisa.edu.au>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the author shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the author.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _GEEXT_H_
#define _GEEXT_H_
#include <X11/extensions/geproto.h>


/**
 * This struct is used both in the window and by grabs to determine the event
 * mask for a client.
 * A window will have a linked list of these structs, with one entry per
 * client per device, null-terminated.
 * A grab has only one instance of this struct.
 */
typedef struct _GenericMaskRec {
    struct _GenericMaskRec* next;
    XID             resource;                 /* id for the resource manager */
    DeviceIntPtr    dev;
    Mask            eventMask[MAXEXTENSIONS]; /* one mask per extension */
} GenericMaskRec, *GenericMaskPtr;


/* Struct to keep information about registered extensions
 *
 * evswap ... use to swap event fields for different byte ordered clients.
 * evfill ... use to fill various event fields from the given parameters.
 */
typedef struct _GEExtension {
    void (*evswap)(xGenericEvent* from, xGenericEvent* to);
    void (*evfill)(xGenericEvent* ev,
                    DeviceIntPtr pDev,  /* device */
                    WindowPtr pWin,     /* event window */
                    GrabPtr pGrab       /* current grab, may be NULL */
                    );
} GEExtension, *GEExtensionPtr;


/* All registered extensions and their handling functions. */
extern GEExtension GEExtensions[MAXEXTENSIONS];

/* Returns the extension offset from the event */
#define GEEXT(ev) (((xGenericEvent*)(ev))->extension)

#define GEEXTIDX(ev) (GEEXT(ev) & 0x7F)
/* Typecast to generic event */
#define GEV(ev) ((xGenericEvent*)(ev))
/* True if mask is set for extension on window */
#define GEMaskIsSet(pWin, extension, mask) \
    ((pWin)->optional && \
     (pWin)->optional->geMasks && \
     ((pWin)->optional->geMasks->eventMasks[(extension) & 0x7F] & (mask)))

/* Returns first client */
#define GECLIENT(pWin) \
    (((pWin)->optional) ? (pWin)->optional->geMasks->geClients : NULL)

/* Returns the event_fill for the given event */
#define GEEventFill(ev) \
    GEExtensions[GEEXTIDX(xE)].evfill

#define GEIsType(ev, ext, ev_type) \
        ((ev->u.u.type == GenericEvent) &&  \
         ((xGenericEvent*)(ev))->extension == ext && \
         ((xGenericEvent*)(ev))->evtype == ev_type)


/* Interface for other extensions */
void GEWindowSetMask(ClientPtr pClient, DeviceIntPtr pDev,
                     WindowPtr pWin, int extension, Mask mask);

void GERegisterExtension(
        int extension,
        void (*ev_dispatch)(xGenericEvent* from, xGenericEvent* to),
        void (*ev_fill)(xGenericEvent* ev, DeviceIntPtr pDev,
                        WindowPtr pWin, GrabPtr pGrab)
        );

void GEInitEvent(xGenericEvent* ev, int extension);
BOOL GEDeviceMaskIsSet(WindowPtr pWin, DeviceIntPtr pDev,
                       int extension, Mask mask);

void GEExtensionInit(void);

#endif /* _GEEXT_H_ */
