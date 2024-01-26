/*
  xfIOKit.h

  IOKit specific functions and definitions
*/
/*
 * Copyright (c) 2001-2002 Torrey T. Lyons. All Rights Reserved.
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

#ifndef _XFIOKIT_H
#define _XFIOKIT_H

#include <pthread.h>
#include <IOKit/graphics/IOFramebufferShared.h>
#include <X11/Xproto.h>
#include "screenint.h"
#include "darwin.h"

typedef struct {
    io_connect_t        fbService;
    StdFBShmem_t       *cursorShmem;
    unsigned char      *framebuffer;
    unsigned char      *shadowPtr;
} XFIOKitScreenRec, *XFIOKitScreenPtr;

#define XFIOKIT_SCREEN_PRIV(pScreen) \
    ((XFIOKitScreenPtr)pScreen->devPrivates[xfIOKitScreenIndex].ptr)

extern int xfIOKitScreenIndex; // index into pScreen.devPrivates
extern io_connect_t xfIOKitInputConnect;

Bool XFIOKitInitCursor(ScreenPtr pScreen);

#endif	/* _XFIOKIT_H */
