/* $XFree86: xc/programs/Xserver/Xext/xtest1dd.h,v 3.2 2001/08/01 00:44:44 tsi Exp $ */
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

#ifndef XTEST1DD_H
#define XTEST1DD_H 1

extern	short		xtest_mousex;
extern	short		xtest_mousey;
extern	int		playback_on;
extern	ClientPtr	current_xtest_client;
extern	ClientPtr	playback_client;
extern	KeyCode		xtest_command_key;

extern void stop_stealing_input(
	void
);

extern void
steal_input(
	ClientPtr              /* client */,
	CARD32                 /* mode */
);

extern void
flush_input_actions(
	void
);

extern void
XTestStealJumpData(
	int                    /* jx */,
	int                    /* jy */,
	int                    /* dev_type */
);

extern void
XTestStealMotionData(
	int                    /* dx */,
	int                    /* dy */,
	int                    /* dev_type */,
	int                    /* mx */,
	int                    /* my */
);

extern Bool
XTestStealKeyData(
	unsigned               /* keycode */,
	int                    /* keystate */,
	int                    /* dev_type */,
	int                    /* locx */,
	int                    /* locy */
);

extern void
parse_fake_input(
	ClientPtr              /* client */,
	char *                 /* req */
);

extern void
XTestComputeWaitTime(
	struct timeval *       /* waittime */
);

extern int
XTestProcessInputAction(
	int                    /* readable */,
	struct timeval *       /* waittime */
);

extern void
abort_play_back(
	void
);

extern void
return_input_array_size(
	ClientPtr              /* client */
);

extern void XTestGenerateEvent(
	int                    /* dev_type */,
	int                    /* keycode */,
	int                    /* keystate */,
	int                    /* mousex */,
	int                    /* mousey */
);

extern void XTestGetPointerPos(
	short *                /* fmousex */,
	short *                /* fmousey */
);

extern void XTestJumpPointer(
	int                    /* jx */,
	int                    /* jy */,
	int                    /* dev_type */
);

#endif /* XTEST1DD_H */
