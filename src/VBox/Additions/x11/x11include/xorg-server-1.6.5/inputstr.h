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
#include "cursorstr.h"
#include "geext.h"
#include "privates.h"

#define BitIsOn(ptr, bit) (((BYTE *) (ptr))[(bit)>>3] & (1 << ((bit) & 7)))

#define SameClient(obj,client) \
	(CLIENT_BITS((obj)->resource) == (client)->clientAsMask)

#define EMASKSIZE	MAXDEVICES + 1

extern DevPrivateKey CoreDevicePrivateKey;

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
    unsigned short      exact;
    Mask                *pMask;
} DetailRec;

/**
 * Central struct for device grabs. 
 * The same struct is used for both core grabs and device grabs, with
 * different fields being set. 
 * If the grab is a core grab (GrabPointer/GrabKeyboard), then the eventMask
 * is a combination of standard event masks (i.e. PointerMotionMask |
 * ButtonPressMask).
 * If the grab is a device grab (GrabDevice), then the eventMask is a
 * combination of event masks for a given XI event type (see SetEventInfo).
 *
 * If the grab is a result of a ButtonPress, then eventMask is the core mask
 * and deviceMask is set to the XI event mask for the grab.
 */
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
    Mask                deviceMask;     
    GenericMaskPtr      genericMasks;
} GrabRec;

typedef struct _KeyClassRec {
    CARD8		down[DOWN_LENGTH];
    CARD8		postdown[DOWN_LENGTH];
    KeyCode 		*modifierKeyMap;
    KeySymsRec		curKeySyms;
    int			modifierKeyCount[8];
    CARD8		modifierMap[MAP_LENGTH];
    CARD8		maxKeysPerModifier;
    unsigned short	state;
    unsigned short	prev_state;
#ifdef XKB
    struct _XkbSrvInfo *xkbInfo;
#else
    void               *pad0;
#endif
} KeyClassRec, *KeyClassPtr;

typedef struct _AxisInfo {
    int		resolution;
    int		min_resolution;
    int		max_resolution;
    int		min_value;
    int		max_value;
} AxisInfo, *AxisInfoPtr;

typedef struct _ValuatorAccelerationRec {
    int                         number;
    PointerAccelSchemeProc      AccelSchemeProc;
    void                       *accelData; /* at disposal of AccelScheme */
    DeviceCallbackProc          AccelCleanupProc;
} ValuatorAccelerationRec, *ValuatorAccelerationPtr;

typedef struct _ValuatorClassRec {
    int		 	  numMotionEvents;
    int                   first_motion;
    int                   last_motion;
    void                  *motion; /* motion history buffer. Different layout
                                      for MDs and SDs!*/
    WindowPtr             motionHintWindow;

    AxisInfoPtr 	  axes;
    unsigned short	  numAxes;
    int			  *axisVal; /* always absolute, but device-coord system */
    CARD8	 	  mode;
    ValuatorAccelerationRec	accelScheme;
} ValuatorClassRec, *ValuatorClassPtr;

typedef struct _ButtonClassRec {
    CARD8		numButtons;
    CARD8		buttonsDown;	/* number of buttons currently down
                                           This counts logical buttons, not
					   physical ones, i.e if some buttons
					   are mapped to 0, they're not counted
					   here */
    unsigned short	state;
    Mask		motionMask;
    CARD8		down[DOWN_LENGTH];
    CARD8		map[MAP_LENGTH];
#ifdef XKB
    union _XkbAction    *xkb_acts;
#else
    void                *pad0;
#endif
} ButtonClassRec, *ButtonClassPtr;

typedef struct _FocusClassRec {
    WindowPtr	win; /* May be set to a int constant (e.g. PointerRootWin)! */
    int		revert;
    TimeStamp	time;
    WindowPtr	*trace;
    int		traceSize;
    int		traceGood;
} FocusClassRec, *FocusClassPtr;

typedef struct _ProximityClassRec {
    char	pad;
} ProximityClassRec, *ProximityClassPtr;

typedef struct _AbsoluteClassRec {
    /* Calibration. */
    int         min_x;
    int         max_x;
    int         min_y;
    int         max_y;
    int         flip_x;
    int         flip_y;
    int		rotation;
    int         button_threshold;

    /* Area. */
    int         offset_x;
    int         offset_y;
    int         width;
    int         height;
    int         screen;
    XID		following;
} AbsoluteClassRec, *AbsoluteClassPtr;

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
#else
    void                *pad0;
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
#else
    void                *pad0;
#endif
} LedFeedbackClassRec;


typedef struct _ClassesRec {
    KeyClassPtr		key;
    ValuatorClassPtr	valuator;
    ButtonClassPtr	button;
    FocusClassPtr	focus;
    ProximityClassPtr	proximity;
    AbsoluteClassPtr    absolute;
    KbdFeedbackPtr	kbdfeed;
    PtrFeedbackPtr	ptrfeed;
    IntegerFeedbackPtr	intfeed;
    StringFeedbackPtr	stringfeed;
    BellFeedbackPtr	bell;
    LedFeedbackPtr	leds;
} ClassesRec;


/**
 * Sprite information for a device.
 */
typedef struct {
    CursorPtr	current;
    BoxRec	hotLimits;	/* logical constraints of hot spot */
    Bool	confined;	/* confined to screen */
    RegionPtr	hotShape;	/* additional logical shape constraint */
    BoxRec	physLimits;	/* physical constraints of hot spot */
    WindowPtr	win;		/* window of logical position */
    HotSpot	hot;		/* logical pointer position */
    HotSpot	hotPhys;	/* physical pointer position */
#ifdef PANORAMIX
    ScreenPtr	screen;		/* all others are in Screen 0 coordinates */
    RegionRec   Reg1;	        /* Region 1 for confining motion */
    RegionRec   Reg2;		/* Region 2 for confining virtual motion */
    WindowPtr   windows[MAXSCREENS];
    WindowPtr	confineWin;	/* confine window */ 
#endif
    /* The window trace information is used at dix/events.c to avoid having
     * to compute all the windows between the root and the current pointer
     * window each time a button or key goes down. The grabs on each of those
     * windows must be checked.
     * spriteTraces should only be used at dix/events.c! */
    WindowPtr *spriteTrace;
    int spriteTraceSize;
    int spriteTraceGood;

    ScreenPtr pEnqueueScreen; /* screen events are being delivered to */
    ScreenPtr pDequeueScreen; /* screen events are being dispatched to */

} SpriteRec, *SpritePtr;

/* Device properties */
typedef struct _XIPropertyValue
{
    Atom                type;           /* ignored by server */
    short               format;         /* format of data for swapping - 8,16,32 */
    long                size;           /* size of data in (format/8) bytes */
    pointer             data;           /* private to client */
} XIPropertyValueRec;

typedef struct _XIProperty
{
    struct _XIProperty   *next;
    Atom                  propertyName;
    BOOL                  deletable;    /* clients can delete this prop? */
    XIPropertyValueRec    value;
} XIPropertyRec;

typedef XIPropertyRec      *XIPropertyPtr;
typedef XIPropertyValueRec *XIPropertyValuePtr;


typedef struct _XIPropertyHandler
{
    struct _XIPropertyHandler* next;
    long id;
    int (*SetProperty) (DeviceIntPtr dev,
                        Atom property,
                        XIPropertyValuePtr prop,
                        BOOL checkonly);
    int (*GetProperty) (DeviceIntPtr dev,
                        Atom property);
    int (*DeleteProperty) (DeviceIntPtr dev,
                           Atom property);
} XIPropertyHandler, *XIPropertyHandlerPtr;

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


typedef struct _GrabInfoRec {
    TimeStamp	    grabTime;
    Bool            fromPassiveGrab;    /* true if from passive grab */
    Bool            implicitGrab;       /* implicit from ButtonPress */
    GrabRec         activeGrab;
    GrabPtr         grab;
    CARD8           activatingKey;
    void	    (*ActivateGrab) (
                    DeviceIntPtr /*device*/,
                    GrabPtr /*grab*/,
                    TimeStamp /*time*/,
                    Bool /*autoGrab*/);
    void	    (*DeactivateGrab)(
                    DeviceIntPtr /*device*/);
    struct {
	Bool		frozen;
	int		state;
	GrabPtr		other;		/* if other grab has this frozen */
	xEvent		*event;		/* saved to be replayed */
	int		evcount;
    } sync;
} GrabInfoRec, *GrabInfoPtr;

typedef struct _SpriteInfoRec {
    /* sprite must always point to a valid sprite. For devices sharing the
     * sprite, let sprite point to a paired spriteOwner's sprite. */
    SpritePtr           sprite;      /* sprite information */
    Bool                spriteOwner; /* True if device owns the sprite */
    DeviceIntPtr        paired;      /* The paired device. Keyboard if
                                        spriteOwner is TRUE, otherwise the
                                        pointer that owns the sprite. */ 
} SpriteInfoRec, *SpriteInfoPtr;

typedef struct _DeviceIntRec {
    DeviceRec	public;
    DeviceIntPtr next;
    Bool	startup;		/* true if needs to be turned on at
				          server intialization time */
    DeviceProc	deviceProc;		/* proc(DevicePtr, DEVICE_xx). It is
					  used to initialize, turn on, or
					  turn off the device */
    Bool	inited;			/* TRUE if INIT returns Success */
    Bool        enabled;                /* TRUE if ON returns Success */
    Bool        coreEvents;             /* TRUE if device also sends core */
    GrabInfoRec deviceGrab;             /* grab on the device */
    Bool        isMaster;               /* TRUE if device is master */
    Atom		type;
    char		*name;
    CARD8		id;
    KeyClassPtr		key;
    ValuatorClassPtr	valuator;
    ButtonClassPtr	button;
    FocusClassPtr	focus;
    ProximityClassPtr	proximity;
    AbsoluteClassPtr    absolute;
    KbdFeedbackPtr	kbdfeed;
    PtrFeedbackPtr	ptrfeed;
    IntegerFeedbackPtr	intfeed;
    StringFeedbackPtr	stringfeed;
    BellFeedbackPtr	bell;
    LedFeedbackPtr	leds;
#ifdef XKB
    struct _XkbInterest *xkb_interest;
#else
    void                *pad0;
#endif
    char                *config_info; /* used by the hotplug layer */
    PrivateRec		*devPrivates;
    int			nPrivates;
    DeviceUnwrapProc    unwrapProc;
    SpriteInfoPtr       spriteInfo;
    union {
    DeviceIntPtr        master;       /* master device */
    DeviceIntPtr        lastSlave;    /* last slave device used */
    } u;

    /* last valuator values recorded, not posted to client;
     * for slave devices, valuators is in device coordinates
     * for master devices, valuators is in screen coordinates
     * see dix/getevents.c
     * remainder supports acceleration
     */
    struct {
        int             valuators[MAX_VALUATORS];
        float           remainder[MAX_VALUATORS];
        int             numValuators;
    } last;

    /* Input device property handling. */
    struct {
        XIPropertyPtr   properties;
        XIPropertyHandlerPtr handlers; /* NULL-terminated */
    } properties;
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
