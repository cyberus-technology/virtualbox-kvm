/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86OSKbd.h,v 1.3 2003/02/17 15:11:55 dawes Exp $ */

/*
 * Copyright (c) 2002 by The XFree86 Project, Inc.
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
    PROT_UNKNOWN
} KbdProtocolId;

typedef struct {
    const char		*name;
    KbdProtocolId	id;
} KbdProtocolRec;

Bool xf86OSKbdPreInit(InputInfoPtr pInfo);
