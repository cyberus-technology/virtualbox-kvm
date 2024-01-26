/* $XFree86: xc/programs/Xserver/Xi/chgfctl.h,v 3.1 1996/04/15 11:18:26 dawes Exp $ */
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef CHGFCTL_H
#define CHGFCTL_H 1

int
SProcXChangeFeedbackControl(
	ClientPtr              /* client */
	);

int
ProcXChangeFeedbackControl(
	ClientPtr              /* client */
	);

int
ChangeKbdFeedback (
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned long          /* mask */,
	KbdFeedbackPtr         /* k */,
	xKbdFeedbackCtl *      /* f */
	);

int
ChangePtrFeedback (
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned long          /* mask */,
	PtrFeedbackPtr         /* p */,
	xPtrFeedbackCtl *      /* f */
	);

int
ChangeIntegerFeedback (
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned long          /* mask */,
	IntegerFeedbackPtr     /* i */,
	xIntegerFeedbackCtl *  /* f */
	);

int
ChangeStringFeedback (
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned long          /* mask */,
	StringFeedbackPtr      /* s */,
	xStringFeedbackCtl *   /* f */
	);

int
ChangeBellFeedback (
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned long          /* mask */,
	BellFeedbackPtr        /* b */,
	xBellFeedbackCtl *     /* f */
	);

int
ChangeLedFeedback (
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	unsigned long          /* mask */,
	LedFeedbackPtr         /* l */,
	xLedFeedbackCtl *      /* f */
	);

#endif /* CHGFCTL_H */
