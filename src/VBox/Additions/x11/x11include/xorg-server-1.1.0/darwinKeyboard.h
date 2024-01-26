/*
 * Copyright (c) 2003-2004 Torrey T. Lyons. All Rights Reserved.
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
/* $XFree86: xc/programs/Xserver/hw/darwin/darwinKeyboard.c,v 1.18 2003/05/14 05:27:55 torrey Exp $ */

#ifndef DARWIN_KEYBOARD_H
#define DARWIN_KEYBOARD_H 1

#define XK_TECHNICAL		// needed to get XK_Escape
#define XK_PUBLISHING
#include "keysym.h"
#include "inputstr.h"

// Each key can generate 4 glyphs. They are, in order:
// unshifted, shifted, modeswitch unshifted, modeswitch shifted
#define GLYPHS_PER_KEY  4
#define NUM_KEYCODES    248	// NX_NUMKEYCODES might be better
#define MAX_KEYCODE     NUM_KEYCODES + MIN_KEYCODE - 1

typedef struct darwinKeyboardInfo_struct {
    CARD8 modMap[MAP_LENGTH];
    KeySym keyMap[MAP_LENGTH * GLYPHS_PER_KEY];
    unsigned char modifierKeycodes[32][2];
} darwinKeyboardInfo;

void DarwinKeyboardReload(DeviceIntPtr pDev);
unsigned int DarwinModeSystemKeymapSeed(void);
Bool DarwinModeReadSystemKeymap(darwinKeyboardInfo *info);

#endif /* DARWIN_KEYBOARD_H */
