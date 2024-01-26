/*
 * Copyright (c) 2002-2003 by The XFree86 Project, Inc.
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
 *
 * Author: Ivan Pascal.
 */

#include "xf86Xinput.h"

Bool ATScancode(InputInfoPtr pInfo, int *scanCode);

/* Public interface to OS-specific keyboard support. */

typedef	int	(*KbdInitProc)(InputInfoPtr pInfo, int what);
typedef	int	(*KbdOnProc)(InputInfoPtr pInfo, int what);
typedef	int	(*KbdOffProc)(InputInfoPtr pInfo, int what);
typedef	void	(*BellProc)(InputInfoPtr pInfo,
                            int loudness, int pitch, int duration);
typedef	void	(*SetLedsProc)(InputInfoPtr pInfo, int leds);
typedef	int	(*GetLedsProc)(InputInfoPtr pInfo);
typedef	void	(*SetKbdRepeatProc)(InputInfoPtr pInfo, char rad);
typedef	void	(*KbdGetMappingProc)(InputInfoPtr pInfo,
                                     KeySymsPtr pKeySyms, CARD8* pModMap);
typedef	int	(*GetSpecialKeyProc)(InputInfoPtr pInfo, int scanCode);
typedef	Bool	(*SpecialKeyProc)(InputInfoPtr pInfo,
                                     int key, Bool down, int modifiers);
typedef	int	(*RemapScanCodeProc)(InputInfoPtr pInfo, int *scanCode);
typedef	Bool	(*OpenKeyboardProc)(InputInfoPtr pInfo);
typedef	void	(*PostEventProc)(InputInfoPtr pInfo,
                                 unsigned int key, Bool down);
typedef struct {
    int                 begin;
    int                 end;
    unsigned char       *map;
} TransMapRec, *TransMapPtr;

typedef struct {
    KbdInitProc		KbdInit;
    KbdOnProc		KbdOn;
    KbdOffProc		KbdOff;
    BellProc		Bell;
    SetLedsProc		SetLeds;
    GetLedsProc		GetLeds;
    SetKbdRepeatProc	SetKbdRepeat;
    KbdGetMappingProc	KbdGetMapping;
    RemapScanCodeProc	RemapScanCode;
    GetSpecialKeyProc	GetSpecialKey;
    SpecialKeyProc	SpecialKey;

    OpenKeyboardProc	OpenKeyboard;
    PostEventProc	PostEvent;

    int			rate;
    int			delay;
    int			bell_pitch;
    int			bell_duration;
    Bool		autoRepeat;
    unsigned long	leds;
    unsigned long	xledsMask;
    unsigned long	keyLeds;
    int			scanPrefix;
    Bool		vtSwitchSupported;
    Bool		CustomKeycodes;
    Bool		noXkb;
    Bool		isConsole;
    TransMapPtr         scancodeMap;
    TransMapPtr         specialMap;

    /* os specific */
    pointer		private;
    int			kbdType;
    int			consType;
    int			wsKbdType;
    Bool		sunKbd;
    Bool		Panix106;

} KbdDevRec, *KbdDevPtr;

typedef enum {
    PROT_STD,
    PROT_XQUEUE,
    PROT_WSCONS,
    PROT_USB,
    PROT_UNKNOWN_KBD
} KbdProtocolId;

typedef struct {
    const char		*name;
    KbdProtocolId	id;
} KbdProtocolRec;

Bool xf86OSKbdPreInit(InputInfoPtr pInfo);

/* Adjust this when the kbd interface changes. */

/*
 * History:
 *
 *  1.0.0 - Initial version.
 */

#define OS_KBD_VERSION_MAJOR 1
#define OS_KBD_VERSION_MINOR 0
#define OS_KBD_VERSION_PATCH 0

#define OS_KBD_VERSION_CURRENT						\
	BUILTIN_INTERFACE_VERSION_NUMERIC(OS_KBD_VERSION_MAJOR,		\
					  OS_KBD_VERSION_MINOR,		\
					  OS_KBD_VERSION_PATCH)

