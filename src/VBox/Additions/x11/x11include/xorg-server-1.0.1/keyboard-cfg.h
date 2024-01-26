/*
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
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
 * CONECTIVA LINUX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Conectiva Linux shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Conectiva Linux.
 *
 * Author: Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/xf86cfg/keyboard-cfg.h,v 1.2 2000/06/13 23:15:51 dawes Exp $
 */

#include "config.h"
#include <X11/extensions/XKBconfig.h>

#ifndef _xf86cfg_keyboard_h
#define _xf86cfg_keyboard_h

/*
 * All file names are from XProjectRoot or XWINHOME environment variable.
 */
#define	XkbConfigDir		"lib/X11/xkb/"
#define	XkbConfigFile		"X0-config.keyboard"

/*
 * Types
 */
typedef struct {
    char **name;
    char **desc;
    int nelem;
} XF86XkbDescInfo;

typedef struct {
    XF86ConfInputPtr conf;
    XkbDescPtr xkb;
    XkbRF_VarDefsRec defs;
    XkbConfigRtrnRec config;
} XkbInfo;

/*
 * Prototypes
 */
XtPointer KeyboardConfig(XtPointer);
void KeyboardModelAndLayout(XF86SetupInfo*);
void InitializeKeyboard(void);
Bool UpdateKeyboard(Bool);
Bool WriteXKBConfiguration(char*, XkbConfigRtrnPtr);

/*
 * Initialization
 */
extern XkbInfo *xkb_info;

#endif /* _xf86cfg_keyboard_h */
