/* $XConsortium: xf86Xinput.h /main/11 1996/10/27 11:05:29 kaleb $ */
/*
 * Copyright 1995-1999 by Frederic Lepied, France. <Lepied@XFree86.org>
 *                                                                            
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is  hereby granted without fee, provided that
 * the  above copyright   notice appear  in   all  copies and  that both  that
 * copyright  notice   and   this  permission   notice  appear  in  supporting
 * documentation, and that   the  name of  Frederic   Lepied not  be  used  in
 * advertising or publicity pertaining to distribution of the software without
 * specific,  written      prior  permission.     Frederic  Lepied   makes  no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.                   
 *                                                                            
 * FREDERIC  LEPIED DISCLAIMS ALL   WARRANTIES WITH REGARD  TO  THIS SOFTWARE,
 * INCLUDING ALL IMPLIED   WARRANTIES OF MERCHANTABILITY  AND   FITNESS, IN NO
 * EVENT  SHALL FREDERIC  LEPIED BE   LIABLE   FOR ANY  SPECIAL, INDIRECT   OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA  OR PROFITS, WHETHER  IN  AN ACTION OF  CONTRACT,  NEGLIGENCE OR OTHER
 * TORTIOUS  ACTION, ARISING    OUT OF OR   IN  CONNECTION  WITH THE USE    OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*
 * Copyright (c) 2000-2002 by The XFree86 Project, Inc.
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

/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Xinput.h,v 3.36 2003/08/24 17:36:55 dawes Exp $ */

#ifndef _xf86Xinput_h
#define _xf86Xinput_h

#ifndef NEED_EVENTS
#define NEED_EVENTS
#endif
#include "xf86str.h"
#include "inputstr.h"
#ifdef XINPUT
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "XIstubs.h"
#endif

/* Input device flags */
#define XI86_OPEN_ON_INIT       0x01 /* open the device at startup time */
#define XI86_CONFIGURED         0x02 /* the device has been configured */
#define XI86_ALWAYS_CORE	0x04 /* device always controls the pointer */
/* the device sends Xinput and core pointer events */
#define XI86_SEND_CORE_EVENTS	XI86_ALWAYS_CORE
/* if the device is the core pointer or is sending core events, and
 * SEND_DRAG_EVENTS is false, and a buttons is done, then no motion events
 * (mouse drag action) are sent. This is mainly to allow a touch screen to be
 * used with netscape and other browsers which do strange things if the mouse
 * moves between button down and button up. With a touch screen, this motion
 * is common due to the user's finger moving slightly.
 */
#define XI86_SEND_DRAG_EVENTS	0x08
#define XI86_CORE_POINTER	0x10 /* device is the core pointer */
#define XI86_CORE_KEYBOARD	0x20 /* device is the core keyboard */
#define XI86_POINTER_CAPABLE	0x40 /* capable of being a core pointer */
#define XI86_KEYBOARD_CAPABLE	0x80 /* capable of being a core keyboard */

#define XI_PRIVATE(dev) \
	(((LocalDevicePtr)((dev)->public.devicePrivate))->private)

#ifdef DBG
#undef DBG
#endif
#define DBG(lvl, f) {if ((lvl) <= xf86GetVerbosity()) f;}

#ifdef HAS_MOTION_HISTORY
#undef HAS_MOTION_HISTORY
#endif
#define HAS_MOTION_HISTORY(local) ((local)->dev->valuator && (local)->dev->valuator->numMotionEvents)

#ifdef XINPUT
/* This holds the input driver entry and module information. */
typedef struct _InputDriverRec {
    int			    driverVersion;
    char *		    driverName;
    void		    (*Identify)(int flags);
    struct _LocalDeviceRec *(*PreInit)(struct _InputDriverRec *drv,
				       IDevPtr dev, int flags);
    void		    (*UnInit)(struct _InputDriverRec *drv,
				      struct _LocalDeviceRec *pInfo,
				      int flags);
    pointer		    module;
    int			    refCount;
} InputDriverRec, *InputDriverPtr;
#endif

/* This is to input devices what the ScrnInfoRec is to screens. */

typedef struct _LocalDeviceRec {
    struct _LocalDeviceRec *next;
    char *		    name;
    int			    flags;
    
    Bool		    (*device_control)(DeviceIntPtr device, int what);
    void		    (*read_input)(struct _LocalDeviceRec *local);
    int			    (*control_proc)(struct _LocalDeviceRec *local,
					   xDeviceCtl *control);
    void		    (*close_proc)(struct _LocalDeviceRec *local);
    int			    (*switch_mode)(ClientPtr client, DeviceIntPtr dev,
					  int mode);
    Bool		    (*conversion_proc)(struct _LocalDeviceRec *local,
					      int first, int num, int v0,
					      int v1, int v2, int v3, int v4,
					      int v5, int *x, int *y);
    Bool		    (*reverse_conversion_proc)(
					struct _LocalDeviceRec *local,
					int x, int y, int *valuators);
    
    int			    fd;
    Atom		    atom;
    DeviceIntPtr	    dev;
    pointer		    private;
    int			    private_flags;
    pointer		    motion_history;
    ValuatorMotionProcPtr   motion_history_proc;
    unsigned int	    history_size;   /* only for configuration purpose */
    unsigned int	    first;
    unsigned int	    last;
    int			    old_x;
    int			    old_y;
    float		    dxremaind;
    float		    dyremaind;
    char *		    type_name;
    IntegerFeedbackPtr	    always_core_feedback;
    IDevPtr		    conf_idev;
    InputDriverPtr	    drv;
    pointer		    module;
    pointer		    options;
} LocalDeviceRec, *LocalDevicePtr, InputInfoRec, *InputInfoPtr;

typedef struct _DeviceAssocRec 
{
    char *		    config_section_name;
    LocalDevicePtr	    (*device_allocate)(void);
} DeviceAssocRec, *DeviceAssocPtr;

/* xf86Globals.c */
extern InputInfoPtr xf86InputDevs;

/* xf86Xinput.c */
int xf86IsCorePointer(DeviceIntPtr dev);
int xf86IsCoreKeyboard(DeviceIntPtr dev);
void xf86XInputSetSendCoreEvents(LocalDevicePtr local, Bool always);
#define xf86AlwaysCore(a,b) xf86XInputSetSendCoreEvents(a,b)

void InitExtInput(void);
Bool xf86eqInit(DevicePtr pKbd, DevicePtr pPtr);
void xf86eqEnqueue(struct _xEvent *event);
void xf86eqProcessInputEvents (void);
void xf86eqSwitchScreen(ScreenPtr pScreen, Bool fromDIX);
void xf86PostMotionEvent(DeviceIntPtr device, int is_absolute,
			 int first_valuator, int num_valuators, ...);
void xf86PostProximityEvent(DeviceIntPtr device, int is_in,
			    int first_valuator, int num_valuators, ...);
void xf86PostButtonEvent(DeviceIntPtr device, int is_absolute, int button,
		    	 int is_down, int first_valuator, int num_valuators,
			 ...);
void xf86PostKeyEvent(DeviceIntPtr device, unsigned int key_code, int is_down,
		      int is_absolute, int first_valuator, int num_valuators,
		      ...);
void xf86PostKeyboardEvent(DeviceIntPtr device, unsigned int key_code,
                           int is_down);
void xf86MotionHistoryAllocate(LocalDevicePtr local);
int xf86GetMotionEvents(DeviceIntPtr dev, xTimecoord *buff,
			unsigned long start, unsigned long stop,
			ScreenPtr pScreen);
void xf86XinputFinalizeInit(DeviceIntPtr dev);
void xf86ActivateDevice(LocalDevicePtr local);
Bool xf86CheckButton(int button, int down);
void xf86SwitchCoreDevice(LocalDevicePtr device, DeviceIntPtr core);
LocalDevicePtr xf86FirstLocalDevice(void);
int xf86ScaleAxis(int Cx, int Sxhigh, int Sxlow, int Rxhigh, int Rxlow);
void xf86XInputSetScreen(LocalDevicePtr local, int screen_number, int x, int y);
void xf86ProcessCommonOptions(InputInfoPtr pInfo, pointer options);
void xf86InitValuatorAxisStruct(DeviceIntPtr dev, int axnum, int minval,
				int maxval, int resolution, int min_res,
				int max_res);
void xf86InitValuatorDefaults(DeviceIntPtr dev, int axnum);
void xf86AddEnabledDevice(InputInfoPtr pInfo);
void xf86RemoveEnabledDevice(InputInfoPtr pInfo);

/* xf86Helper.c */
void xf86AddInputDriver(InputDriverPtr driver, pointer module, int flags);
void xf86DeleteInputDriver(int drvIndex);
InputInfoPtr xf86AllocateInput(InputDriverPtr drv, int flags);
void xf86DeleteInput(InputInfoPtr pInp, int flags);

/* xf86Option.c */
void xf86CollectInputOptions(InputInfoPtr pInfo, const char **defaultOpts,
			     pointer extraOpts);

#endif /* _xf86Xinput_h */
