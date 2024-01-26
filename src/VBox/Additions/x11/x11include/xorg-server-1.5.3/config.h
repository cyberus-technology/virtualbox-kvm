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
 */

#ifdef HAVE_CONFIG_H
# include "xorg-config.h"
#endif

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xmu/SysUtil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#ifdef sun
#undef index
#undef rindex
#include <strings.h>
#endif
#include <unistd.h>

#include <stdarg.h>

/* Get PATH_MAX */
#ifndef PATH_MAX
# if defined(_POSIX_SOURCE)
#  include <limits.h>
# else
#  define _POSIX_SOURCE
#  include <limits.h>
#  undef _POSIX_SOURCE
# endif
# ifndef PATH_MAX
#  ifdef MAXPATHLEN
#   define PATH_MAX MAXPATHLEN
#  else
#   define PATH_MAX 1024
#  endif
# endif
#endif

#include <xf86Parser.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>
#include <X11/extensions/XKM.h>
#include <X11/extensions/XKBfile.h>
#include <X11/extensions/XKBui.h>
#include <X11/extensions/XKBrules.h>

#ifndef _xf86cfg_config_h
#define _xf86cfg_config_h

/* Must match the offset in the xf86info structure at config.c,
 * and is used also by interface.c
 */
#define MOUSE			0
#define KEYBOARD		1
#define CARD			2
#define MONITOR			3
#define SCREEN			4
#define SERVER			5

#define	UNUSED			0
#define	USED			1

#define CONFIG_LAYOUT	0
#define CONFIG_SCREEN	1
#define CONFIG_MODELINE	2
#define CONFIG_ACCESSX	3
extern int config_mode;

#define CONFPATH	"%A," "%R," \
			"/etc/X11/%R," "%P/etc/X11/%R," \
			"%E," "%F," \
			"/etc/X11/%F," "%P/etc/X11/%F," \
			"/etc/X11/%X-%M," "/etc/X11/%X," "/etc/%X," \
			"%P/etc/X11/%X.%H," "%P/etc/X11/%X-%M," \
			"%P/etc/X11/%X," \
			"%P/lib/X11/%X.%H," "%P/lib/X11/%X-%M," \
			"%P/lib/X11/%X"
#define USER_CONFPATH	"/etc/X11/%S," "%P/etc/X11/%S," \
                        "/etc/X11/%G," "%P/etc/X11/%G," \
			"%P/etc/X11/%X.%H," "%P/etc/X11/%X-%M," \
			"%P/etc/X11/%X," \
			"%P/lib/X11/%X.%H," "%P/lib/X11/%X-%M," \
			"%P/lib/X11/%X"

/*
 * Types
 */
typedef struct _XF86SetupInfo XF86SetupInfo;
typedef void (*XF86SetupFunction)(XF86SetupInfo*);

typedef struct _XF86SetupFunctionList {
    XF86SetupFunction *functions;
    int num_functions;
    int cur_function;
} XF86SetupFunctionList;

struct _XF86SetupInfo {
    int num_lists;
    int cur_list;
    XF86SetupFunctionList *lists;
};

typedef Bool (*ConfigCheckFunction)(void);

typedef struct _xf86cfgDevice xf86cfgDevice;

struct _xf86cfgDevice {
    XtPointer config;
    Widget widget;
    int type, state, refcount;
};

typedef struct {
    XF86ConfScreenPtr screen;
    Widget widget;
    int type, state, refcount;
    xf86cfgDevice *card;
    xf86cfgDevice *monitor;
    short row, column;
    XRectangle rect;
    short rotate;
} xf86cfgScreen;

/* this structure is used just to restore
   properly the monitors layout in the
   screen window configuration.
 */
typedef struct {
    XF86ConfLayoutPtr layout;
    xf86cfgScreen **screen;
    XPoint *position;
    int num_layouts;
} xf86cfgLayout;

/* The vidmode extension usage is controlled by this structure.
 * The information is read at startup, and added monitors cannot
 * be configured, since they are not attached to a particular screen.
 */
typedef struct _xf86cfgVidMode xf86cfgVidmode;

typedef struct {
    XF86ConfLayoutPtr layout;	/* current layout */
    Widget cpu;
    xf86cfgLayout **layouts;
    Cardinal num_layouts;
    xf86cfgScreen **screens;
    Cardinal num_screens;
    xf86cfgDevice **devices;
    Cardinal num_devices;
    xf86cfgVidmode **vidmodes;
    Cardinal num_vidmodes;
} xf86cfgComputer;

/*
 * Prototypes
 */
void StartConfig(void);
Bool ConfigLoop(ConfigCheckFunction);
void ConfigError(void);
void ChangeScreen(XF86ConfMonitorPtr, XF86ConfMonitorPtr,
		  XF86ConfDevicePtr, XF86ConfDevicePtr);
void SetTip(xf86cfgDevice*);
Bool startx(void);
void endx(void);
void startaccessx(void);
void ConfigCancelAction(Widget, XEvent*, String*, Cardinal*);
void ExpertConfigureStart(void);
void ExpertConfigureEnd(void);
void ExpertCloseAction(Widget, XEvent*, String*, Cardinal*);
void ExpertCallback(Widget, XtPointer, XtPointer);

/*
 * Initialization
 */
extern Widget toplevel, configp, current, back, next;
extern XtAppContext appcon;
extern XF86SetupInfo xf86info;
extern Widget ident_widget;
extern char *ident_string;
extern XF86ConfigPtr XF86Config;
extern char *XF86Config_path;
extern char *XF86Module_path;
extern char *XFree86_path;
extern char *XF86Font_path;
extern char *XF86RGB_path;
extern char *XFree86Dir;
extern xf86cfgComputer computer;
extern Atom wm_delete_window;
extern Display *DPY;
extern Pixmap menuPixmap;
#ifdef USE_MODULES
extern int nomodules;
#endif

#endif /* _xf86cfg_config_h */
