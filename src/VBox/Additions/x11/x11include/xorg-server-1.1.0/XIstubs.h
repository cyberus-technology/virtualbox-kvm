/* $XFree86: xc/programs/Xserver/include/XIstubs.h,v 3.1 1996/04/15 11:34:22 dawes Exp $ */
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

#ifndef XI_STUBS_H
#define XI_STUBS_H 1

int
ChangeKeyboardDevice (
	DeviceIntPtr           /* old_dev */,
	DeviceIntPtr           /* new_dev */);

int
ChangePointerDevice (
	DeviceIntPtr           /* old_dev */,
	DeviceIntPtr           /* new_dev */,
	unsigned char          /* x */,
	unsigned char          /* y */);

void
CloseInputDevice (
	DeviceIntPtr           /* d */,
	ClientPtr              /* client */);

void
AddOtherInputDevices (void);

void
OpenInputDevice (
	DeviceIntPtr           /* dev */,
	ClientPtr              /* client */,
	int *                  /* status */);

int
SetDeviceMode (
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	int                    /* mode */);

int
SetDeviceValuators (
	ClientPtr              /* client */,
	DeviceIntPtr           /* dev */,
	int *                  /* valuators */,
	int                    /* first_valuator */,
	int                    /* num_valuators */);

int
ChangeDeviceControl (
	ClientPtr             /* client */,
	DeviceIntPtr          /* dev */,
	xDeviceCtl *          /* control */);

#endif /* XI_STUBS_H */
