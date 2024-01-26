/*
 * Xplugin rootless implementation
 */
/*
 * Copyright (c) 2003 Torrey T. Lyons. All Rights Reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */
/* $XdotOrg: xc/programs/Xserver/hw/darwin/quartz/xpr/xpr.h,v 1.2 2004/04/23 19:16:52 eich Exp $ */
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/xpr/xpr.h,v 1.4 2003/11/12 20:21:52 torrey Exp $ */

#ifndef XPR_H
#define XPR_H

#include "screenint.h"

extern Bool QuartzModeBundleInit(void);

void AppleDRIExtensionInit(void);
void xprAppleWMInit(void);
Bool xprInit(ScreenPtr pScreen);
Bool xprIsX11Window(void *nsWindow, int windowNumber);
void xprHideWindows(Bool hide);

Bool QuartzInitCursor(ScreenPtr pScreen);
void QuartzSuspendXCursor(ScreenPtr pScreen);
void QuartzResumeXCursor(ScreenPtr pScreen, int x, int y);

#endif /* XPR_H */
