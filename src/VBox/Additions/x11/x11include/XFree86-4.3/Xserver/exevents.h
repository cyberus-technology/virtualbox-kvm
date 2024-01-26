/* $XFree86: xc/programs/Xserver/include/exevents.h,v 3.1 1996/04/15 11:34:29 dawes Exp $ */
/************************************************************

Copyright 1996 by Thomas E. Dickey <dickey@clark.net>

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of the above listed
copyright holder(s) not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

/********************************************************************
 * Interface of 'exevents.c'
 */

#ifndef EXEVENTS_H
#define EXEVENTS_H

void
RegisterOtherDevice (
#if NeedFunctionPrototypes
	DeviceIntPtr           /* device */
#endif
	);

void
ProcessOtherEvent (
#if NeedFunctionPrototypes
	xEventPtr /* FIXME deviceKeyButtonPointer * xE */,
	DeviceIntPtr           /* other */,
	int                    /* count */
#endif
	);

int
InitProximityClassDeviceStruct(
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */
#endif
	);

void
InitValuatorAxisStruct(
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */,
	int                    /* axnum */,
	int                    /* minval */,
	int                    /* maxval */,
	int                    /* resolution */,
	int                    /* min_res */,
	int                    /* max_res */
#endif
	);

void
DeviceFocusEvent(
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */,
	int                    /* type */,
	int                    /* mode */,
	int                    /* detail */,
	WindowPtr              /* pWin */
#endif
	);

int
GrabButton(
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	BYTE                   /* this_device_mode */,
	BYTE                   /* other_devices_mode */,
	CARD16                 /* modifiers */,
	DeviceIntPtr           /* modifier_device */,
	CARD8                  /* button */,
	Window                 /* grabWindow */,
	BOOL                   /* ownerEvents */,
	Cursor                 /* rcursor */,
	Window                 /* rconfineTo */,
	Mask                   /* eventMask */
#endif
	);

int
GrabKey(
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	BYTE                   /* this_device_mode */,
	BYTE                   /* other_devices_mode */,
	CARD16                 /* modifiers */,
	DeviceIntPtr           /* modifier_device */,
	CARD8                  /* key */,
	Window                 /* grabWindow */,
	BOOL                   /* ownerEvents */,
	Mask                   /* mask */
#endif
	);

int
SelectForWindow(
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */,
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	Mask                   /* exclusivemasks */,
	Mask                   /* validmasks */
#endif
	);

int 
AddExtensionClient (
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	int                    /* mskidx */
#endif
	);

void
RecalculateDeviceDeliverableEvents(
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */
#endif
	);

int
InputClientGone(
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	XID                    /* id */
#endif
	);

int
SendEvent (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* d */,
	Window                 /* dest */,
	Bool                   /* propagate */,
	xEvent *               /* ev */,
	Mask                   /* mask */,
	int                    /* count */
#endif
	);

int
SetButtonMapping (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	int                    /* nElts */,
	BYTE *                 /* map */
#endif
	);

int 
SetModifierMapping(
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	int                    /* len */,
	int                    /* rlen */,
	int                    /* numKeyPerModifier */,
	KeyCode *              /* inputMap */,
	KeyClassPtr *          /* k */
#endif
	);

void
SendDeviceMappingNotify(
#if NeedFunctionPrototypes
	CARD8                  /* request, */,
	KeyCode                /* firstKeyCode */,
	CARD8                  /* count */,
	DeviceIntPtr           /* dev */
#endif
);

int
ChangeKeyMapping(
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned               /* len */,
	int                    /* type */,
	KeyCode                /* firstKeyCode */,
	CARD8                  /* keyCodes */,
	CARD8                  /* keySymsPerKeyCode */,
	KeySym *               /* map */
#endif
	);

void
DeleteWindowFromAnyExtEvents(
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	Bool                   /* freeResources */
#endif
);

void
DeleteDeviceFromAnyExtEvents(
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	DeviceIntPtr           /* dev */
#endif
	);

int
MaybeSendDeviceMotionNotifyHint (
#if NeedFunctionPrototypes
	deviceKeyButtonPointer * /* pEvents */,
	Mask                   /* mask */
#endif
);

void
CheckDeviceGrabAndHintWindow (
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	int                    /* type */,
	deviceKeyButtonPointer * /* xE */,
	GrabPtr                /* grab */,
	ClientPtr              /* client */,
	Mask                   /* deliveryMask */
#endif
	);

Mask
DeviceEventMaskForClient(
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */,
	WindowPtr              /* pWin */,
	ClientPtr              /* client */
#endif
);

void
MaybeStopDeviceHint(
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */,
	ClientPtr              /* client */
#endif
	);

int
DeviceEventSuppressForWindow(
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	int                    /* maskndx */
#endif
	);

#endif /* EXEVENTS_H */
