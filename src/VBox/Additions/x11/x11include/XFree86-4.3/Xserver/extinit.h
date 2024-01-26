/* $XFree86: xc/programs/Xserver/include/extinit.h,v 3.2 2001/08/01 00:44:58 tsi Exp $ */
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
#if NeedFunctionPrototypes
	void
#endif
	);


int
ProcIDispatch (
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

int
SProcIDispatch(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

void
SReplyIDispatch (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	int                    /* len */,
	xGrabDeviceReply *     /* rep */
#endif
	);

void
SEventIDispatch (
#if NeedFunctionPrototypes
	xEvent *               /* from */,
	xEvent *               /* to */
#endif
	);

void
SEventDeviceValuator (
#if NeedFunctionPrototypes
	deviceValuator *       /* from */,
	deviceValuator *       /* to */
#endif
	);

void
SEventFocus (
#if NeedFunctionPrototypes
	deviceFocus *          /* from */,
	deviceFocus *          /* to */
#endif
	);

void
SDeviceStateNotifyEvent (
#if NeedFunctionPrototypes
	deviceStateNotify *    /* from */,
	deviceStateNotify *    /* to */
#endif
	);

void
SDeviceKeyStateNotifyEvent (
#if NeedFunctionPrototypes
	deviceKeyStateNotify * /* from */,
	deviceKeyStateNotify * /* to */
#endif
	);

void
SDeviceButtonStateNotifyEvent (
#if NeedFunctionPrototypes
	deviceButtonStateNotify * /* from */,
	deviceButtonStateNotify * /* to */
#endif
	);

void
SChangeDeviceNotifyEvent (
#if NeedFunctionPrototypes
	changeDeviceNotify *   /* from */,
	changeDeviceNotify *   /* to */
#endif
	);

void
SDeviceMappingNotifyEvent (
#if NeedFunctionPrototypes
	deviceMappingNotify *  /* from */,
	deviceMappingNotify *  /* to */
#endif
	);

void
FixExtensionEvents (
#if NeedFunctionPrototypes
	ExtensionEntry 	*      /* extEntry */
#endif
	);

void
RestoreExtensionEvents (
#if NeedFunctionPrototypes
	void
#endif
	);

void
IResetProc(
#if NeedFunctionPrototypes
	ExtensionEntry *       /* unused */
#endif
	);

void
AssignTypeAndName (
#if NeedFunctionPrototypes
	DeviceIntPtr           /* dev */,
	Atom                   /* type */,
	char *                 /* name */
#endif
	);

void
MakeDeviceTypeAtoms (
#if NeedFunctionPrototypes
	void
#endif
);

DeviceIntPtr
LookupDeviceIntRec (
#if NeedFunctionPrototypes
	CARD8                  /* id */
#endif
	);

void
SetExclusiveAccess (
#if NeedFunctionPrototypes
	Mask                   /* mask */
#endif
	);

void
AllowPropagateSuppress (
#if NeedFunctionPrototypes
	Mask                   /* mask */
#endif
	);

Mask
GetNextExtEventMask (
#if NeedFunctionPrototypes
	void
#endif
);

void
SetMaskForExtEvent(
#if NeedFunctionPrototypes
	Mask                   /* mask */,
	int                    /* event */
#endif
	);

void
SetEventInfo(
#if NeedFunctionPrototypes
	Mask                   /* mask */,
	int                    /* constant */
#endif
	);

#endif /* EXTINIT_H */
