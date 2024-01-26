/* $XFree86: xc/programs/Xserver/include/dixevents.h,v 3.4 2001/09/04 14:03:27 dawes Exp $ */
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

#ifndef DIXEVENTS_H
#define DIXEVENTS_H

extern void
SetCriticalEvent(
#if NeedFunctionPrototypes
	int                    /* event */
#endif
	);

extern CursorPtr
GetSpriteCursor(
#if NeedFunctionPrototypes
	void
#endif
	);

extern int
ProcAllowEvents(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

extern int
MaybeDeliverEventsToClient(
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	xEvent *               /* pEvents */,
	int                    /* count */,
	Mask                   /* filter */,
	ClientPtr              /* dontClient */
#endif
	);

extern int
ProcWarpPointer(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

#if 0
extern void
#ifdef XKB
CoreProcessKeyboardEvent (
#else
ProcessKeyboardEvent (
#endif
#if NeedFunctionPrototypes
	xEvent *               /* xE */,
	DeviceIntPtr           /* keybd */,
	int                    /* count */
#endif
	);

extern void
#ifdef XKB
CoreProcessPointerEvent (
#else
ProcessPointerEvent (
#endif
#if NeedFunctionPrototypes
	xEvent *               /* xE */,
	DeviceIntPtr           /* mouse */,
	int                    /* count */
#endif
	);
#endif

extern int
EventSelectForWindow(
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */
#endif
	);

extern int
EventSuppressForWindow(
#if NeedFunctionPrototypes
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	Bool *                 /* checkOptional */
#endif
	);

extern int
ProcSetInputFocus(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

extern int
ProcGetInputFocus(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

extern int
ProcGrabPointer(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

extern int
ProcChangeActivePointerGrab(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

extern int
ProcUngrabPointer(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

extern int
ProcGrabKeyboard(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

extern int
ProcUngrabKeyboard(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

extern int
ProcQueryPointer(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

extern int
ProcSendEvent(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

extern int
ProcUngrabKey(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

extern int
ProcGrabKey(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

extern int
ProcGrabButton(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

extern int
ProcUngrabButton(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

extern int
ProcRecolorCursor(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

#endif /* DIXEVENTS_H */
