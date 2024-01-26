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

extern void SetCriticalEvent(int /* event */);

extern CursorPtr GetSpriteCursor(void);

extern int ProcAllowEvents(ClientPtr /* client */);

extern int MaybeDeliverEventsToClient(
	WindowPtr              /* pWin */,
	xEvent *               /* pEvents */,
	int                    /* count */,
	Mask                   /* filter */,
	ClientPtr              /* dontClient */);

extern int ProcWarpPointer(ClientPtr /* client */);

#if 0
extern void
#ifdef XKB
CoreProcessKeyboardEvent (
#else
ProcessKeyboardEvent (
#endif
	xEvent *               /* xE */,
	DeviceIntPtr           /* keybd */,
	int                    /* count */);

extern void
#ifdef XKB
CoreProcessPointerEvent (
#else
ProcessPointerEvent (
#endif
	xEvent *               /* xE */,
	DeviceIntPtr           /* mouse */,
	int                    /* count */);
#endif

extern int EventSelectForWindow(
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */);

extern int EventSuppressForWindow(
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
	Mask                   /* mask */,
	Bool *                 /* checkOptional */);

extern int ProcSetInputFocus(ClientPtr /* client */);

extern int ProcGetInputFocus(ClientPtr /* client */);

extern int ProcGrabPointer(ClientPtr /* client */);

extern int ProcChangeActivePointerGrab(ClientPtr /* client */);

extern int ProcUngrabPointer(ClientPtr /* client */);

extern int ProcGrabKeyboard(ClientPtr /* client */);

extern int ProcUngrabKeyboard(ClientPtr /* client */);

extern int ProcQueryPointer(ClientPtr /* client */);

extern int ProcSendEvent(ClientPtr /* client */);

extern int ProcUngrabKey(ClientPtr /* client */);

extern int ProcGrabKey(ClientPtr /* client */);

extern int ProcGrabButton(ClientPtr /* client */);

extern int ProcUngrabButton(ClientPtr /* client */);

extern int ProcRecolorCursor(ClientPtr /* client */);

#ifdef PANORAMIX
extern void PostSyntheticMotion(int x, int y, int screen, unsigned long time);
#endif

#endif /* DIXEVENTS_H */
