/* $XdotOrg: xserver/xorg/hw/xfree86/common/xf86Version.h,v 1.5 2005/08/24 11:18:35 daniels Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/xf86Version.h,v 3.566 2003/12/19 04:52:11 dawes Exp $ */

/*
 * Copyright (c) 1994-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifndef XF86_VERSION_CURRENT

#define XF86_VERSION_MAJOR	4
#define XF86_VERSION_MINOR	3
#define XF86_VERSION_PATCH	99
#define XF86_VERSION_SNAP	902

/* This has five arguments for compatibilty reasons */
#define XF86_VERSION_NUMERIC(major,minor,patch,snap,dummy) \
	(((major) * 10000000) + ((minor) * 100000) + ((patch) * 1000) + snap)

#define XF86_GET_MAJOR_VERSION(vers)	((vers) / 10000000)
#define XF86_GET_MINOR_VERSION(vers)	(((vers) % 10000000) / 100000)
#define XF86_GET_PATCH_VERSION(vers)	(((vers) % 100000) / 1000)
#define XF86_GET_SNAP_VERSION(vers)	((vers) % 1000)

/* Define these for compatibility.  They'll be removed at some point. */
#define XF86_VERSION_SUBMINOR	XF86_VERSION_PATCH
#define XF86_VERSION_BETA	0
#define XF86_VERSION_ALPHA	XF86_VERSION_SNAP

#define XF86_VERSION_CURRENT					\
   XF86_VERSION_NUMERIC(XF86_VERSION_MAJOR,			\
			XF86_VERSION_MINOR,			\
			XF86_VERSION_PATCH,			\
			XF86_VERSION_SNAP,			\
			0)

#endif

/* $XConsortium: xf86Version.h /main/78 1996/10/28 05:42:10 kaleb $ */
/* $XdotOrg: xserver/xorg/hw/xfree86/common/xf86Version.h,v 1.5 2005/08/24 11:18:35 daniels Exp $ */
