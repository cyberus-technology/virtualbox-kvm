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

#ifndef INPUT_H
#define INPUT_H

#include "misc.h"
#include "screenint.h"
#include <X11/Xmd.h>
#include <X11/Xproto.h>
#include "window.h"     /* for WindowPtr */
#include "xkbrules.h"
#include "events.h"

#define DEVICE_INIT	0
#define DEVICE_ON	1
#define DEVICE_OFF	2
#define DEVICE_CLOSE	3

#define POINTER_RELATIVE (1 << 1)
#define POINTER_ABSOLUTE (1 << 2)
#define POINTER_ACCELERATE (1 << 3)
#define POINTER_SCREEN (1 << 4) /* Data in screen coordinates */

/*int constants for pointer acceleration schemes*/
#define PtrAccelNoOp            0
#define PtrAccelPredictable     1
#define PtrAccelLightweight     2
#define PtrAccelDefault         PtrAccelPredictable

#define MAX_VALUATORS 36
/* Maximum number of valuators, divided by six, rounded up, to get number
 * of events. */
#define MAX_VALUATOR_EVENTS 6
#define MAX_BUTTONS 256 /* completely arbitrarily chosen */

#define NO_AXIS_LIMITS -1

#define MAP_LENGTH	256
#define DOWN_LENGTH	32	/* 256/8 => number of bytes to hold 256 bits */
#define NullGrab ((GrabPtr)NULL)
#define PointerRootWin ((WindowPtr)PointerRoot)
#define NoneWin ((WindowPtr)None)
#define NullDevice ((DevicePtr)NULL)

#ifndef FollowKeyboard
#define FollowKeyboard 		3
#endif
#ifndef FollowKeyboardWin
#define FollowKeyboardWin  ((WindowPtr) FollowKeyboard)
#endif
#ifndef RevertToFollowKeyboard
#define RevertToFollowKeyboard	3
#endif

typedef unsigned long Leds;
typedef struct _OtherClients *OtherClientsPtr;
typedef struct _InputClients *InputClientsPtr;
typedef struct _DeviceIntRec *DeviceIntPtr;
typedef struct _ClassesRec *ClassesPtr;
typedef union _GrabMask GrabMask;

typedef struct _EventList {
    xEvent* event;
    int evlen; /* length of allocated memory for event in bytes.  This is not
                  the actual length of the event. The event's actual length is
                  32 for standard events or 32 +
                  ((xGenericEvent*)event)->length * 4 for GenericEvents.
                  For events in the EQ, the length is
                  ((InternalEvent*)event)->u.any.length */
} EventList, *EventListPtr;

/* The DIX stores incoming input events in this list */
extern EventListPtr InputEventList;
extern int InputEventListLen;

typedef int (*DeviceProc)(
    DeviceIntPtr /*device*/,
    int /*what*/);

typedef void (*ProcessInputProc)(
    InternalEvent * /*event*/,
    DeviceIntPtr /*device*/);

typedef Bool (*DeviceHandleProc)(
    DeviceIntPtr /*device*/,
    void* /*data*/
    );

typedef void (*DeviceUnwrapProc)(
    DeviceIntPtr /*device*/,
    DeviceHandleProc /*proc*/,
    void* /*data*/
    );

/* pointer acceleration handling */
typedef void (*PointerAccelSchemeProc)(
    DeviceIntPtr /*pDev*/,
    int /*first_valuator*/,
    int /*num_valuators*/,
    int* /*valuators*/,
    int /*evtime*/);

typedef void (*DeviceCallbackProc)(
              DeviceIntPtr /*pDev*/);

typedef struct _DeviceRec {
    pointer	devicePrivate;
    ProcessInputProc processInputProc;	/* current */
    ProcessInputProc realInputProc;	/* deliver */
    ProcessInputProc enqueueInputProc;	/* enqueue */
    Bool	on;			/* used by DDX to keep state */
} DeviceRec, *DevicePtr;

typedef struct {
    int			click, bell, bell_pitch, bell_duration;
    Bool		autoRepeat;
    unsigned char	autoRepeats[32];
    Leds		leds;
    unsigned char	id;
} KeybdCtrl;

typedef struct {
    KeySym  *map;
    KeyCode minKeyCode,
	    maxKeyCode;
    int     mapWidth;
} KeySymsRec, *KeySymsPtr;

typedef struct {
    int		num, den, threshold;
    unsigned char id;
} PtrCtrl;

typedef struct {
    int         resolution, min_value, max_value;
    int         integer_displayed;
    unsigned char id;
} IntegerCtrl;

typedef struct {
    int         max_symbols, num_symbols_supported;
    int         num_symbols_displayed;
    KeySym      *symbols_supported;
    KeySym      *symbols_displayed;
    unsigned char id;
} StringCtrl;

typedef struct {
    int         percent, pitch, duration;
    unsigned char id;
} BellCtrl;

typedef struct {
    Leds        led_values;
    Mask        led_mask;
    unsigned char id;
} LedCtrl;

extern _X_EXPORT KeybdCtrl	defaultKeyboardControl;
extern _X_EXPORT PtrCtrl	defaultPointerControl;

typedef struct _InputOption {
    char                *key;
    char                *value;
    struct _InputOption *next;
} InputOption;

/* Key has been run through all input processing and events sent to clients. */
#define KEY_PROCESSED 1
/* Key has not been fully processed, no events have been sent. */
#define KEY_POSTED 2

extern void set_key_down(DeviceIntPtr pDev, int key_code, int type);
extern void set_key_up(DeviceIntPtr pDev, int key_code, int type);
extern int key_is_down(DeviceIntPtr pDev, int key_code, int type);

extern void InitCoreDevices(void);
extern void InitXTestDevices(void);

extern _X_EXPORT DeviceIntPtr AddInputDevice(
    ClientPtr /*client*/,
    DeviceProc /*deviceProc*/,
    Bool /*autoStart*/);

extern _X_EXPORT Bool EnableDevice(
    DeviceIntPtr /*device*/,
    BOOL /* sendevent */);

extern _X_EXPORT Bool ActivateDevice(
    DeviceIntPtr /*device*/,
    BOOL /* sendevent */);

extern _X_EXPORT Bool DisableDevice(
    DeviceIntPtr /*device*/,
    BOOL /* sendevent */);

extern int InitAndStartDevices(void);

extern void CloseDownDevices(void);

extern void UndisplayDevices(void);

extern _X_EXPORT int RemoveDevice(
    DeviceIntPtr /*dev*/,
    BOOL /* sendevent */);

extern _X_EXPORT int NumMotionEvents(void);

extern void RegisterPointerDevice(
    DeviceIntPtr /*device*/);

extern void RegisterKeyboardDevice(
    DeviceIntPtr /*device*/);

extern _X_EXPORT int dixLookupDevice(
    DeviceIntPtr *         /* dev */,
    int                    /* id */,
    ClientPtr              /* client */,
    Mask                   /* access_mode */);

extern _X_EXPORT void QueryMinMaxKeyCodes(
    KeyCode* /*minCode*/,
    KeyCode* /*maxCode*/);

extern _X_EXPORT Bool SetKeySymsMap(
    KeySymsPtr /*dst*/,
    KeySymsPtr /*src*/);

extern _X_EXPORT Bool InitButtonClassDeviceStruct(
    DeviceIntPtr /*device*/,
    int /*numButtons*/,
    Atom* /* labels */,
    CARD8* /*map*/);

extern _X_EXPORT Bool InitValuatorClassDeviceStruct(
    DeviceIntPtr /*device*/,
    int /*numAxes*/,
    Atom* /* labels */,
    int /*numMotionEvents*/,
    int /*mode*/);

extern _X_EXPORT Bool InitPointerAccelerationScheme(
    DeviceIntPtr /*dev*/,
    int /*scheme*/);

extern _X_EXPORT Bool InitAbsoluteClassDeviceStruct(
    DeviceIntPtr /*device*/);

extern _X_EXPORT Bool InitFocusClassDeviceStruct(
    DeviceIntPtr /*device*/);

typedef void (*BellProcPtr)(
    int /*percent*/,
    DeviceIntPtr /*device*/,
    pointer /*ctrl*/,
    int);

typedef void (*KbdCtrlProcPtr)(
    DeviceIntPtr /*device*/,
    KeybdCtrl * /*ctrl*/);

typedef void (*PtrCtrlProcPtr)(
    DeviceIntPtr /*device*/,
    PtrCtrl * /*ctrl*/);

extern _X_EXPORT Bool InitPtrFeedbackClassDeviceStruct(
    DeviceIntPtr /*device*/,
    PtrCtrlProcPtr /*controlProc*/);

typedef void (*StringCtrlProcPtr)(
    DeviceIntPtr /*device*/,
    StringCtrl * /*ctrl*/);

extern _X_EXPORT Bool InitStringFeedbackClassDeviceStruct(
    DeviceIntPtr /*device*/,
    StringCtrlProcPtr /*controlProc*/,
    int /*max_symbols*/,
    int /*num_symbols_supported*/,
    KeySym* /*symbols*/);

typedef void (*BellCtrlProcPtr)(
    DeviceIntPtr /*device*/,
    BellCtrl * /*ctrl*/);

extern _X_EXPORT Bool InitBellFeedbackClassDeviceStruct(
    DeviceIntPtr /*device*/,
    BellProcPtr /*bellProc*/,
    BellCtrlProcPtr /*controlProc*/);

typedef void (*LedCtrlProcPtr)(
    DeviceIntPtr /*device*/,
    LedCtrl * /*ctrl*/);

extern _X_EXPORT Bool InitLedFeedbackClassDeviceStruct(
    DeviceIntPtr /*device*/,
    LedCtrlProcPtr /*controlProc*/);

typedef void (*IntegerCtrlProcPtr)(
    DeviceIntPtr /*device*/,
    IntegerCtrl * /*ctrl*/);


extern _X_EXPORT Bool InitIntegerFeedbackClassDeviceStruct(
    DeviceIntPtr /*device*/,
    IntegerCtrlProcPtr /*controlProc*/);

extern _X_EXPORT Bool InitPointerDeviceStruct(
    DevicePtr /*device*/,
    CARD8* /*map*/,
    int /*numButtons*/,
    Atom* /* btn_labels */,
    PtrCtrlProcPtr /*controlProc*/,
    int /*numMotionEvents*/,
    int /*numAxes*/,
    Atom* /* axes_labels */);

extern _X_EXPORT Bool InitKeyboardDeviceStruct(
    DeviceIntPtr /*device*/,
    XkbRMLVOSet * /*rmlvo*/,
    BellProcPtr /*bellProc*/,
    KbdCtrlProcPtr /*controlProc*/);

extern int ApplyPointerMapping(
    DeviceIntPtr /* pDev */,
    CARD8 *      /* map */,
    int          /* len */,
    ClientPtr	/* client */);

extern Bool BadDeviceMap(
    BYTE* /*buff*/,
    int /*length*/,
    unsigned /*low*/,
    unsigned /*high*/,
    XID* /*errval*/);

extern void NoteLedState(
    DeviceIntPtr /*keybd*/,
    int /*led*/,
    Bool /*on*/);

extern void MaybeStopHint(
    DeviceIntPtr /*device*/,
    ClientPtr /*client*/);

extern void ProcessPointerEvent(
    InternalEvent* /* ev */,
    DeviceIntPtr /*mouse*/);

extern void ProcessKeyboardEvent(
    InternalEvent* /*ev*/,
    DeviceIntPtr   /*keybd*/);

extern Bool LegalModifier(
    unsigned int /*key*/, 
    DeviceIntPtr /*pDev*/);

extern _X_EXPORT void ProcessInputEvents(void);

extern _X_EXPORT void InitInput(
    int  /*argc*/,
    char ** /*argv*/);

extern _X_EXPORT int GetMaximumEventsNum(void);

extern _X_EXPORT int GetEventList(EventListPtr* list);
extern _X_EXPORT EventListPtr InitEventList(int num_events);
extern _X_EXPORT void FreeEventList(EventListPtr list, int num_events);

extern void CreateClassesChangedEvent(EventListPtr event,
                                      DeviceIntPtr master,
                                      DeviceIntPtr slave,
                                      int type);
extern int GetPointerEvents(
    EventListPtr events,
    DeviceIntPtr pDev,
    int type,
    int buttons,
    int flags,
    int first_valuator,
    int num_valuators,
    int *valuators);

extern int GetKeyboardEvents(
    EventListPtr events,
    DeviceIntPtr pDev,
    int type,
    int key_code);

extern int GetKeyboardValuatorEvents(
    EventListPtr events,
    DeviceIntPtr pDev,
    int type,
    int key_code,
    int first_valuator,
    int num_valuator,
    int *valuators);

extern int GetProximityEvents(
    EventListPtr events,
    DeviceIntPtr pDev,
    int type,
    int first_valuator,
    int num_valuators,
    int *valuators);

extern void PostSyntheticMotion(
    DeviceIntPtr pDev,
    int x,
    int y,
    int screen,
    unsigned long time);

extern _X_EXPORT int GetMotionHistorySize(
    void);

extern _X_EXPORT void AllocateMotionHistory(
    DeviceIntPtr pDev);

extern _X_EXPORT int GetMotionHistory(
    DeviceIntPtr pDev,
    xTimecoord **buff,
    unsigned long start,
    unsigned long stop,
    ScreenPtr pScreen,
    BOOL core);

extern int AttachDevice(ClientPtr client,
                        DeviceIntPtr slave,
                        DeviceIntPtr master);

extern _X_EXPORT DeviceIntPtr GetPairedDevice(DeviceIntPtr kbd);
extern DeviceIntPtr GetMaster(DeviceIntPtr dev, int type);

extern int AllocDevicePair(ClientPtr client,
                             char* name,
                             DeviceIntPtr* ptr,
                             DeviceIntPtr* keybd,
                             DeviceProc ptr_proc,
                             DeviceProc keybd_proc,
                             Bool master);
extern void DeepCopyDeviceClasses(DeviceIntPtr from,
                                  DeviceIntPtr to,
                                  DeviceChangedEvent *dce);

/* Helper functions. */
extern int generate_modkeymap(ClientPtr client, DeviceIntPtr dev,
                              KeyCode **modkeymap, int *max_keys_per_mod);
extern int change_modmap(ClientPtr client, DeviceIntPtr dev, KeyCode *map,
                         int max_keys_per_mod);
extern int AllocXTestDevice(ClientPtr client,
                             char* name,
                             DeviceIntPtr* ptr,
                             DeviceIntPtr* keybd,
                             DeviceIntPtr master_ptr,
                             DeviceIntPtr master_keybd);
extern BOOL IsXTestDevice(DeviceIntPtr dev, DeviceIntPtr master);
extern DeviceIntPtr GetXTestDevice(DeviceIntPtr master);
extern void SendDevicePresenceEvent(int deviceid, int type);

/* misc event helpers */
extern Mask GetEventFilter(DeviceIntPtr dev, xEvent *event);
extern Mask GetWindowXI2Mask(DeviceIntPtr dev, WindowPtr win, xEvent* ev);
void FixUpEventFromWindow(DeviceIntPtr pDev,
                          xEvent *xE,
                          WindowPtr pWin,
                          Window child,
                          Bool calcChild);

/* Implemented by the DDX. */
extern _X_EXPORT int NewInputDeviceRequest(
    InputOption *options,
    DeviceIntPtr *dev);
extern  _X_EXPORT void DeleteInputDeviceRequest(
    DeviceIntPtr dev);

extern _X_EXPORT void DDXRingBell(
    int volume,
    int pitch,
    int duration);

/* Set to TRUE by default - os/utils.c sets it to FALSE on user request,
   xfixes/cursor.c uses it to determine if the cursor is enabled */
extern Bool EnableCursor;

#endif /* INPUT_H */
