/************************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/


#ifndef INPUTSTRUCT_H
#define INPUTSTRUCT_H

#include "input.h"
#include "window.h"
#include "dixstruct.h"

#define BitIsOn(ptr, bit) (((BYTE *) (ptr))[(bit)>>3] & (1 << ((bit) & 7)))

#define SameClient(obj,client) \
	(CLIENT_BITS((obj)->resource) == (client)->clientAsMask)

#define MAX_DEVICES	20

#define EMASKSIZE	MAX_DEVICES

/* Kludge: OtherClients and InputClients must be compatible, see code */

typedef struct _OtherClients {
    OtherClientsPtr	next;
    XID			resource; /* id for putting into resource manager */
    Mask		mask;
} OtherClients;

typedef struct _InputClients {
    InputClientsPtr	next;
    XID			resource; /* id for putting into resource manager */
    Mask		mask[EMASKSIZE];
} InputClients;

typedef struct _OtherInputMasks {
    Mask		deliverableEvents[EMASKSIZE];
    Mask		inputEvents[EMASKSIZE];
    Mask		dontPropagateMask[EMASKSIZE];
    InputClientsPtr	inputClients;
} OtherInputMasks;

/*
 * The following structure gets used for both active and passive grabs. For
 * active grabs some of the fields (e.g. modifiers) are not used. However,
 * that is not much waste since there aren't many active grabs (one per
 * keyboard/pointer device) going at once in the server.
 */

#define MasksPerDetailMask 8		/* 256 keycodes and 256 possible
						modifier combinations, but only	
						3 buttons. */

  typedef struct _DetailRec {		/* Grab details may be bit masks */
	unsigned short exact;
	Mask *pMask;
  } DetailRec;

  typedef struct _GrabRec {
    GrabPtr		next;		/* for chain of passive grabs */
    XID			resource;
    DeviceIntPtr	device;
    WindowPtr		window;
    unsigned		ownerEvents:1;
    unsigned		keyboardMode:1;
    unsigned		pointerMode:1;
    unsigned		coreGrab:1;	/* grab is on core device */
    unsigned		coreMods:1;	/* modifiers are on core keyboard */
    CARD8		type;		/* event type */
    DetailRec		modifiersDetail;
    DeviceIntPtr	modifierDevice;
    DetailRec		detail;		/* key or button */
    WindowPtr		confineTo;	/* always NULL for keyboards */
    CursorPtr		cursor;		/* always NULL for keyboards */
    Mask		eventMask;
} GrabRec;

typedef struct _KeyClassRec {
    CARD8		down[DOWN_LENGTH];
    KeyCode 		*modifierKeyMap;
    KeySymsRec		curKeySyms;
    int			modifierKeyCount[8];
    CARD8		modifierMap[MAP_LENGTH];
    CARD8		maxKeysPerModifier;
    unsigned short	state;
    unsigned short	prev_state;
#ifdef XKB
    struct _XkbSrvInfo *xkbInfo;
#endif
} KeyClassRec, *KeyClassPtr;

typedef struct _AxisInfo {
    int		resolution;
    int		min_resolution;
    int		max_resolution;
    int		min_value;
    int		max_value;
} AxisInfo, *AxisInfoPtr;

typedef struct _ValuatorClassRec {
    ValuatorMotionProcPtr GetMotionProc;
    int		 	numMotionEvents;
    WindowPtr    	motionHintWindow;
    AxisInfoPtr 	axes;
    unsigned short	numAxes;
    int			*axisVal;
    CARD8	 	mode;
} ValuatorClassRec, *ValuatorClassPtr;

typedef struct _ButtonClassRec {
    CARD8		numButtons;
    CARD8		buttonsDown;	/* number of buttons currently down */
    unsigned short	state;
    Mask		motionMask;
    CARD8		down[DOWN_LENGTH];
    CARD8		map[MAP_LENGTH];
#ifdef XKB
    union _XkbAction *	xkb_acts;
#endif
} ButtonClassRec, *ButtonClassPtr;

typedef struct _FocusClassRec {
    WindowPtr	win;
    int		revert;
    TimeStamp	time;
    WindowPtr	*trace;
    int		traceSize;
    int		traceGood;
} FocusClassRec, *FocusClassPtr;

typedef struct _ProximityClassRec {
    char	pad;
} ProximityClassRec, *ProximityClassPtr;

typedef struct _KbdFeedbackClassRec *KbdFeedbackPtr;
typedef struct _PtrFeedbackClassRec *PtrFeedbackPtr;
typedef struct _IntegerFeedbackClassRec *IntegerFeedbackPtr;
typedef struct _StringFeedbackClassRec *StringFeedbackPtr;
typedef struct _BellFeedbackClassRec *BellFeedbackPtr;
typedef struct _LedFeedbackClassRec *LedFeedbackPtr;

typedef struct _KbdFeedbackClassRec {
    BellProcPtr		BellProc;
    KbdCtrlProcPtr	CtrlProc;
    KeybdCtrl	 	ctrl;
    KbdFeedbackPtr	next;
#ifdef XKB
    struct _XkbSrvLedInfo *xkb_sli;
#endif
} KbdFeedbackClassRec;

typedef struct _PtrFeedbackClassRec {
    PtrCtrlProcPtr	CtrlProc;
    PtrCtrl		ctrl;
    PtrFeedbackPtr	next;
} PtrFeedbackClassRec;

typedef struct _IntegerFeedbackClassRec {
    IntegerCtrlProcPtr	CtrlProc;
    IntegerCtrl	 	ctrl;
    IntegerFeedbackPtr	next;
} IntegerFeedbackClassRec;

typedef struct _StringFeedbackClassRec {
    StringCtrlProcPtr	CtrlProc;
    StringCtrl	 	ctrl;
    StringFeedbackPtr	next;
} StringFeedbackClassRec;

typedef struct _BellFeedbackClassRec {
    BellProcPtr		BellProc;
    BellCtrlProcPtr	CtrlProc;
    BellCtrl	 	ctrl;
    BellFeedbackPtr	next;
} BellFeedbackClassRec;

typedef struct _LedFeedbackClassRec {
    LedCtrlProcPtr	CtrlProc;
    LedCtrl	 	ctrl;
    LedFeedbackPtr	next;
#ifdef XKB
    struct _XkbSrvLedInfo *xkb_sli;
#endif
} LedFeedbackClassRec;

/* states for devices */

#define NOT_GRABBED		0
#define THAWED			1
#define THAWED_BOTH		2	/* not a real state */
#define FREEZE_NEXT_EVENT	3
#define FREEZE_BOTH_NEXT_EVENT	4
#define FROZEN			5	/* any state >= has device frozen */
#define FROZEN_NO_EVENT		5
#define FROZEN_WITH_EVENT	6
#define THAW_OTHERS		7

typedef struct _DeviceIntRec {
    DeviceRec	public;
    DeviceIntPtr next;
    TimeStamp	grabTime;
    Bool	startup;		/* true if needs to be turned on at
				          server intialization time */
    DeviceProc	deviceProc;		/* proc(DevicePtr, DEVICE_xx). It is
					  used to initialize, turn on, or
					  turn off the device */
    Bool	inited;			/* TRUE if INIT returns Success */
    GrabPtr	grab;			/* the grabber - used by DIX */
    struct {
	Bool		frozen;
	int		state;
	GrabPtr		other;		/* if other grab has this frozen */
	xEvent		*event;		/* saved to be replayed */
	int		evcount;
    } sync;
    Atom		type;
    char		*name;
    CARD8		id;
    CARD8		activatingKey;
    Bool		fromPassiveGrab;
    GrabRec		activeGrab;
    void		(*ActivateGrab) (
			DeviceIntPtr /*device*/,
			GrabPtr /*grab*/,
			TimeStamp /*time*/,
			Bool /*autoGrab*/);
    void		(*DeactivateGrab)(
			DeviceIntPtr /*device*/);
    KeyClassPtr		key;
    ValuatorClassPtr	valuator;
    ButtonClassPtr	button;
    FocusClassPtr	focus;
    ProximityClassPtr	proximity;
    KbdFeedbackPtr	kbdfeed;
    PtrFeedbackPtr	ptrfeed;
    IntegerFeedbackPtr	intfeed;
    StringFeedbackPtr	stringfeed;
    BellFeedbackPtr	bell;
    LedFeedbackPtr	leds;
#ifdef XKB
    struct _XkbInterest *	xkb_interest;
#endif
    DevUnion		*devPrivates;
    int			nPrivates;
    DeviceUnwrapProc    unwrapProc;
} DeviceIntRec;

typedef struct {
    int			numDevices;	/* total number of devices */
    DeviceIntPtr	devices;	/* all devices turned on */
    DeviceIntPtr	off_devices;	/* all devices turned off */
    DeviceIntPtr	keyboard;	/* the main one for the server */
    DeviceIntPtr	pointer;
} InputInfo;

extern InputInfo inputInfo;

/* for keeping the events for devices grabbed synchronously */
typedef struct _QdEvent *QdEventPtr;
typedef struct _QdEvent {
    QdEventPtr		next;
    DeviceIntPtr	device;
    ScreenPtr		pScreen;	/* what screen the pointer was on */
    unsigned long	months;		/* milliseconds is in the event */
    xEvent		*event;
    int			evcount;
} QdEventRec;    

#endif /* INPUTSTRUCT_H */
