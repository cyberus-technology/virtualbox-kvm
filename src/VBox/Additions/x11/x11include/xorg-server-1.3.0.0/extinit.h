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
 * Interface of extinit.c
 */

#ifndef EXTINIT_H
#define EXTINIT_H

#include "extnsionst.h"

void
XInputExtensionInit(
	void
	);


int
ProcIDispatch (
	ClientPtr              /* client */
	);

int
SProcIDispatch(
	ClientPtr              /* client */
	);

void
SReplyIDispatch (
	ClientPtr              /* client */,
	int                    /* len */,
	xGrabDeviceReply *     /* rep */
	);

void
SEventIDispatch (
	xEvent *               /* from */,
	xEvent *               /* to */
	);

void
SEventDeviceValuator (
	deviceValuator *       /* from */,
	deviceValuator *       /* to */
	);

void
SEventFocus (
	deviceFocus *          /* from */,
	deviceFocus *          /* to */
	);

void
SDeviceStateNotifyEvent (
	deviceStateNotify *    /* from */,
	deviceStateNotify *    /* to */
	);

void
SDeviceKeyStateNotifyEvent (
	deviceKeyStateNotify * /* from */,
	deviceKeyStateNotify * /* to */
	);

void
SDeviceButtonStateNotifyEvent (
	deviceButtonStateNotify * /* from */,
	deviceButtonStateNotify * /* to */
	);

void
SChangeDeviceNotifyEvent (
	changeDeviceNotify *   /* from */,
	changeDeviceNotify *   /* to */
	);

void
SDeviceMappingNotifyEvent (
	deviceMappingNotify *  /* from */,
	deviceMappingNotify *  /* to */
	);

void
FixExtensionEvents (
	ExtensionEntry 	*      /* extEntry */
	);

void
RestoreExtensionEvents (
	void
	);

void
IResetProc(
	ExtensionEntry *       /* unused */
	);

void
AssignTypeAndName (
	DeviceIntPtr           /* dev */,
	Atom                   /* type */,
	char *                 /* name */
	);

void
MakeDeviceTypeAtoms (
	void
);

DeviceIntPtr
LookupDeviceIntRec (
	CARD8                  /* id */
	);

void
SetExclusiveAccess (
	Mask                   /* mask */
	);

void
AllowPropagateSuppress (
	Mask                   /* mask */
	);

Mask
GetNextExtEventMask (
	void
);

void
SetMaskForExtEvent(
	Mask                   /* mask */,
	int                    /* event */
	);

void
SetEventInfo(
	Mask                   /* mask */,
	int                    /* constant */
	);

#endif /* EXTINIT_H */
