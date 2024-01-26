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

extern void RegisterOtherDevice (
	DeviceIntPtr           /* device */);

extern void ProcessOtherEvent (
	xEventPtr /* FIXME deviceKeyButtonPointer * xE */,
	DeviceIntPtr           /* other */,
	int                    /* count */);

extern int InitProximityClassDeviceStruct(
	DeviceIntPtr           /* dev */);

extern void InitValuatorAxisStruct(
	DeviceIntPtr           /* dev */,
	int                    /* axnum */,
	int                    /* minval */,
	int                    /* maxval */,
	int                    /* resolution */,
	int                    /* min_res */,
	int                    /* max_res */);

extern void DeviceFocusEvent(
	DeviceIntPtr           /* dev */,
	int                    /* type */,
	int                    /* mode */,
	int                    /* detail */,
	WindowPtr              /* pWin */);

extern int GrabButton(
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
	Mask                   /* eventMask */);

extern int GrabKey(
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	BYTE                   /* this_device_mode */,
	BYTE                   /* other_devices_mode */,
	CARD16                 /* modifiers */,
	DeviceIntPtr           /* modifier_device */,
	CARD8                  /* key */,
	Window                 /* grabWindow */,
	BOOL                   /* ownerEvents */,
	Mask                   /* mask */);

extern int SelectForWindow(
	DeviceIntPtr           /* dev */,
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	Mask                   /* exclusivemasks */,
	Mask                   /* validmasks */);

extern int AddExtensionClient (
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	int                    /* mskidx */);

extern void RecalculateDeviceDeliverableEvents(
	WindowPtr              /* pWin */);

extern int InputClientGone(
	WindowPtr              /* pWin */,
	XID                    /* id */);

extern int SendEvent (
	ClientPtr              /* client */,
	DeviceIntPtr           /* d */,
	Window                 /* dest */,
	Bool                   /* propagate */,
	xEvent *               /* ev */,
	Mask                   /* mask */,
	int                    /* count */);

extern int SetButtonMapping (
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	int                    /* nElts */,
	BYTE *                 /* map */);

extern int SetModifierMapping(
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	int                    /* len */,
	int                    /* rlen */,
	int                    /* numKeyPerModifier */,
	KeyCode *              /* inputMap */,
	KeyClassPtr *          /* k */);

extern void SendDeviceMappingNotify(
	CARD8                  /* request, */,
	KeyCode                /* firstKeyCode */,
	CARD8                  /* count */,
	DeviceIntPtr           /* dev */);

extern int ChangeKeyMapping(
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned               /* len */,
	int                    /* type */,
	KeyCode                /* firstKeyCode */,
	CARD8                  /* keyCodes */,
	CARD8                  /* keySymsPerKeyCode */,
	KeySym *               /* map */);

extern void DeleteWindowFromAnyExtEvents(
	WindowPtr              /* pWin */,
	Bool                   /* freeResources */);

extern void DeleteDeviceFromAnyExtEvents(
	WindowPtr              /* pWin */,
	DeviceIntPtr           /* dev */);

extern int MaybeSendDeviceMotionNotifyHint (
	deviceKeyButtonPointer * /* pEvents */,
	Mask                   /* mask */);

extern void CheckDeviceGrabAndHintWindow (
	WindowPtr              /* pWin */,
	int                    /* type */,
	deviceKeyButtonPointer * /* xE */,
	GrabPtr                /* grab */,
	ClientPtr              /* client */,
	Mask                   /* deliveryMask */);

extern Mask DeviceEventMaskForClient(
	DeviceIntPtr           /* dev */,
	WindowPtr              /* pWin */,
	ClientPtr              /* client */);

extern void MaybeStopDeviceHint(
	DeviceIntPtr           /* dev */,
	ClientPtr              /* client */);

extern int DeviceEventSuppressForWindow(
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	int                    /* maskndx */);

#endif /* EXEVENTS_H */
