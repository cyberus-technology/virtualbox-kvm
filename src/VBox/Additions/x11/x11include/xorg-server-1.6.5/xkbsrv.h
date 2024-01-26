/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be 
used in advertising or publicity pertaining to distribution 
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability 
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, 
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifndef _XKBSRV_H_
#define	_XKBSRV_H_

#define XkbAllocClientMap		SrvXkbAllocClientMap
#define XkbAllocServerMap		SrvXkbAllocServerMap
#define XkbChangeTypesOfKey		SrvXkbChangeTypesOfKey
#define XkbCopyKeyTypes			SrvXkbCopyKeyTypes
#define XkbFreeClientMap		SrvXkbFreeClientMap
#define XkbFreeServerMap		SrvXkbFreeServerMap
#define	XkbKeyTypesForCoreSymbols	SrvXkbKeyTypesForCoreSymbols
#define	XkbApplyCompatMapToKey		SrvXkbApplyCompatMapToKey
#define XkbResizeKeyActions		SrvXkbResizeKeyActions
#define XkbResizeKeySyms		SrvXkbResizeKeySyms
#define XkbResizeKeyType		SrvXkbResizeKeyType
#define XkbAllocCompatMap		SrvXkbAllocCompatMap
#define XkbAllocControls		SrvXkbAllocControls
#define XkbAllocIndicatorMaps		SrvXkbAllocIndicatorMaps
#define XkbAllocKeyboard		SrvXkbAllocKeyboard
#define XkbAllocNames			SrvXkbAllocNames
#define XkbFreeCompatMap		SrvXkbFreeCompatMap
#define XkbFreeKeyboard			SrvXkbFreeKeyboard
#define XkbFreeNames			SrvXkbFreeNames
#define XkbLatchModifiers		SrvXkbLatchModifiers
#define XkbLatchGroup			SrvXkbLatchGroup
#define XkbVirtualModsToReal		SrvXkbVirtualModsToReal
#define	XkbChangeKeycodeRange		SrvXkbChangeKeycodeRange
#define	XkbApplyVirtualModChanges	SrvXkbApplyVirtualModChanges

#include <X11/extensions/XKBproto.h>
#include "xkbstr.h"
#include "inputstr.h"

typedef struct _XkbInterest {
	DeviceIntPtr		dev;
	ClientPtr		client;
	XID			resource;
	struct _XkbInterest *	next;
	CARD16			extDevNotifyMask;
	CARD16			stateNotifyMask;
	CARD16			namesNotifyMask;
	CARD32 			ctrlsNotifyMask;
	CARD8			compatNotifyMask;
	BOOL			bellNotifyMask;
	BOOL			actionMessageMask;
	CARD16			accessXNotifyMask;
	CARD32			iStateNotifyMask;
	CARD32			iMapNotifyMask;
	CARD16			altSymsNotifyMask;
	CARD32			autoCtrls;
	CARD32			autoCtrlValues;
} XkbInterestRec,*XkbInterestPtr;

typedef struct _XkbRadioGroup {
	CARD8		flags;
	CARD8		nMembers;
	CARD8		dfltDown;
	CARD8		currentDown;
	CARD8		members[XkbRGMaxMembers];
} XkbRadioGroupRec, *XkbRadioGroupPtr;

typedef struct	_XkbEventCause {
	CARD8		kc;
	CARD8		event;
	CARD8		mjr;
	CARD8		mnr;
	ClientPtr	client;
} XkbEventCauseRec,*XkbEventCausePtr;
#define	XkbSetCauseKey(c,k,e)	{ (c)->kc= (k),(c)->event= (e),\
				  (c)->mjr= (c)->mnr= 0; \
				  (c)->client= NULL; }
#define	XkbSetCauseReq(c,j,n,cl) { (c)->kc= (c)->event= 0,\
				  (c)->mjr= (j),(c)->mnr= (n);\
				  (c)->client= (cl); }
#define	XkbSetCauseCoreReq(c,e,cl) XkbSetCauseReq(c,e,0,cl)
#define	XkbSetCauseXkbReq(c,e,cl)  XkbSetCauseReq(c,XkbReqCode,e,cl)
#define	XkbSetCauseUnknown(c)	   XkbSetCauseKey(c,0,0)

#define	_OFF_TIMER		0
#define	_KRG_WARN_TIMER		1
#define	_KRG_TIMER		2
#define	_SK_TIMEOUT_TIMER	3
#define	_ALL_TIMEOUT_TIMER	4

#define	_BEEP_NONE		0
#define	_BEEP_FEATURE_ON	1
#define	_BEEP_FEATURE_OFF	2
#define	_BEEP_FEATURE_CHANGE	3
#define	_BEEP_SLOW_WARN		4
#define	_BEEP_SLOW_PRESS	5
#define	_BEEP_SLOW_ACCEPT	6
#define	_BEEP_SLOW_REJECT	7
#define	_BEEP_SLOW_RELEASE	8
#define	_BEEP_STICKY_LATCH	9
#define	_BEEP_STICKY_LOCK	10
#define	_BEEP_STICKY_UNLOCK	11
#define	_BEEP_LED_ON		12
#define	_BEEP_LED_OFF		13
#define	_BEEP_LED_CHANGE	14
#define	_BEEP_BOUNCE_REJECT	15

typedef struct _XkbFilter {
	CARD16			  keycode;
	CARD8			  what;
	CARD8			  active;
	CARD8			  filterOthers;
	CARD32			  priv;
	XkbAction		  upAction;
	int			(*filter)(
					struct _XkbSrvInfo* 	/* xkbi */,
					struct _XkbFilter *	/* filter */,
					unsigned		/* keycode */,
					XkbAction *		/* action */
				  );
	struct _XkbFilter	 *next;
} XkbFilterRec,*XkbFilterPtr;

typedef struct _XkbSrvInfo {
	XkbStateRec	 prev_state;
	XkbStateRec	 state;
	XkbDescPtr	 desc;

	DeviceIntPtr	 device;
	KbdCtrlProcPtr	 kbdProc;

	XkbRadioGroupPtr radioGroups;
	CARD8		 nRadioGroups;
	CARD8		 clearMods;
	CARD8		 setMods;
	INT16		 groupChange;

	CARD16		 dfltPtrDelta;

	double		 mouseKeysCurve;
	double		 mouseKeysCurveFactor;
	INT16		 mouseKeysDX;
	INT16		 mouseKeysDY;
	CARD8		 mouseKeysFlags;
	Bool		 mouseKeysAccel;
	CARD8		 mouseKeysCounter;

	CARD8		 lockedPtrButtons;
	CARD8		 shiftKeyCount;
	KeyCode		 mouseKey;
	KeyCode		 inactiveKey;
	KeyCode		 slowKey;
	KeyCode		 repeatKey;
	CARD8		 krgTimerActive;
	CARD8		 beepType;
	CARD8		 beepCount;

	CARD32		 flags;
	CARD32		 lastPtrEventTime;
	CARD32		 lastShiftEventTime;
	OsTimerPtr	 beepTimer;
	OsTimerPtr	 mouseKeyTimer;
	OsTimerPtr	 slowKeysTimer;
	OsTimerPtr	 bounceKeysTimer;
	OsTimerPtr	 repeatKeyTimer;
	OsTimerPtr	 krgTimer;

	int		 szFilters;
	XkbFilterPtr	 filters;
} XkbSrvInfoRec, *XkbSrvInfoPtr;

#define	XkbSLI_IsDefault	(1L<<0)
#define	XkbSLI_HasOwnState	(1L<<1)

typedef struct	_XkbSrvLedInfo {
	CARD16			flags;
	CARD16			class;
	CARD16			id;
	union {
	    KbdFeedbackPtr	kf;
	    LedFeedbackPtr	lf;
	} 			fb;

	CARD32			physIndicators;
	CARD32			autoState;
	CARD32			explicitState;
	CARD32			effectiveState;

	CARD32			mapsPresent;
	CARD32			namesPresent;
	XkbIndicatorMapPtr	maps;
	Atom *			names;

	CARD32			usesBase;
	CARD32			usesLatched;
	CARD32			usesLocked;
	CARD32			usesEffective;
	CARD32			usesCompat;
	CARD32			usesControls;

	CARD32			usedComponents;
} XkbSrvLedInfoRec, *XkbSrvLedInfoPtr;

/*
 * Settings for xkbClientFlags field (used by DIX)
 * These flags _must_ not overlap with XkbPCF_*
 */
#define	_XkbClientInitialized		(1<<15)

#define	_XkbWantsDetectableAutoRepeat(c)\
	((c)->xkbClientFlags&XkbPCF_DetectableAutoRepeatMask)

/*
 * Settings for flags field
 */
#define	_XkbStateNotifyInProgress	(1<<0)

typedef struct
{
    ProcessInputProc processInputProc;
    /* If processInputProc is set to something different than realInputProc,
     * UNWRAP and COND_WRAP will not touch processInputProc and update only
     * realInputProc.  This ensures that
     *   processInputProc == (frozen ? EnqueueEvent : realInputProc)
     *
     * WRAP_PROCESS_INPUT_PROC should only be called during initialization,
     * since it may destroy this invariant.
     */
    ProcessInputProc realInputProc;
    DeviceUnwrapProc unwrapProc;
} xkbDeviceInfoRec, *xkbDeviceInfoPtr;

#define WRAP_PROCESS_INPUT_PROC(device, oldprocs, proc, unwrapproc) \
	device->public.processInputProc = proc; \
	oldprocs->processInputProc = \
	oldprocs->realInputProc = device->public.realInputProc; \
	device->public.realInputProc = proc; \
	oldprocs->unwrapProc = device->unwrapProc; \
	device->unwrapProc = unwrapproc;

#define COND_WRAP_PROCESS_INPUT_PROC(device, oldprocs, proc, unwrapproc) \
	if (device->public.processInputProc == device->public.realInputProc)\
	    device->public.processInputProc = proc; \
	oldprocs->processInputProc = \
	oldprocs->realInputProc = device->public.realInputProc; \
	device->public.realInputProc = proc; \
	oldprocs->unwrapProc = device->unwrapProc; \
	device->unwrapProc = unwrapproc;

#define UNWRAP_PROCESS_INPUT_PROC(device, oldprocs, backupproc) \
        backupproc = device->public.realInputProc; \
	if (device->public.processInputProc == device->public.realInputProc)\
	    device->public.processInputProc = oldprocs->realInputProc; \
	device->public.realInputProc = oldprocs->realInputProc; \
	device->unwrapProc = oldprocs->unwrapProc;

extern DevPrivateKey xkbDevicePrivateKey;
#define XKBDEVICEINFO(dev) ((xkbDeviceInfoPtr)dixLookupPrivate(&(dev)->devPrivates, xkbDevicePrivateKey))

extern void xkbUnwrapProc(DeviceIntPtr, DeviceHandleProc, pointer);

/***====================================================================***/


/***====================================================================***/

#define XkbAX_KRGMask	 (XkbSlowKeysMask|XkbBounceKeysMask)
#define	XkbAllFilteredEventsMask \
	(XkbAccessXKeysMask|XkbRepeatKeysMask|XkbMouseKeysAccelMask|XkbAX_KRGMask)

/***====================================================================***/

extern int	XkbReqCode;
extern int	XkbEventBase;
extern int	XkbDisableLockActions;
extern int	XkbKeyboardErrorCode;
extern char *	XkbBaseDirectory;
extern char *	XkbBinDirectory;
extern char *	XkbInitialMap;
extern unsigned	int XkbXIUnsupported;

extern Bool	noXkbExtension;

extern pointer	XkbLastRepeatEvent;

extern CARD32	xkbDebugFlags;

#define	_XkbAlloc(s)		xalloc((s))
#define	_XkbCalloc(n,s)		Xcalloc((n)*(s))
#define	_XkbRealloc(o,s)	Xrealloc((o),(s))
#define	_XkbTypedAlloc(t)	((t *)xalloc(sizeof(t)))
#define	_XkbTypedCalloc(n,t)	((t *)Xcalloc((n)*sizeof(t)))
#define	_XkbTypedRealloc(o,n,t) \
	((o)?(t *)Xrealloc((o),(n)*sizeof(t)):_XkbTypedCalloc(n,t))
#define	_XkbClearElems(a,f,l,t)	bzero(&(a)[f],((l)-(f)+1)*sizeof(t))
#define	_XkbFree(p)		Xfree(p)

#define	_XkbLibError(c,l,d) /* Epoch fail */
#define	_XkbErrCode2(a,b) ((XID)((((unsigned int)(a))<<24)|((b)&0xffffff)))
#define	_XkbErrCode3(a,b,c)	_XkbErrCode2(a,(((unsigned int)(b))<<16)|(c))
#define	_XkbErrCode4(a,b,c,d) _XkbErrCode3(a,b,((((unsigned int)(c))<<8)|(d)))

extern	int	DeviceKeyPress,DeviceKeyRelease,DeviceMotionNotify;
extern	int	DeviceButtonPress,DeviceButtonRelease;
extern	int	DeviceEnterNotify,DeviceLeaveNotify;

#define	_XkbIsPressEvent(t)	(((t)==KeyPress)||((t)==DeviceKeyPress))
#define	_XkbIsReleaseEvent(t)	(((t)==KeyRelease)||((t)==DeviceKeyRelease))

#define	_XkbCoreKeycodeInRange(c,k)	(((k)>=(c)->curKeySyms.minKeyCode)&&\
					 ((k)<=(c)->curKeySyms.maxKeyCode))
#define	_XkbCoreNumKeys(c)	((c)->curKeySyms.maxKeyCode-\
				 (c)->curKeySyms.minKeyCode+1)

#define	XConvertCase(s,l,u)	XkbConvertCase(s,l,u)
#undef	IsKeypadKey
#define	IsKeypadKey(s)		XkbKSIsKeypad(s)

#define	Status		int

#ifndef True
#define	True	TRUE
#define	False	FALSE
#endif

_XFUNCPROTOBEGIN

extern void XkbUseMsg(
    void
);

extern int XkbProcessArguments(
    int				/* argc */,
    char **			/* argv */,
    int				/* i */
);

extern	void	XkbSetExtension(DeviceIntPtr device, ProcessInputProc proc);

extern	void	XkbFreeCompatMap(
    XkbDescPtr			/* xkb */,
    unsigned int		/* which */,
    Bool			/* freeMap */
);

extern	void XkbFreeNames(
	XkbDescPtr		/* xkb */,
	unsigned int		/* which */,
	Bool			/* freeMap */
);

extern int _XkbLookupAnyDevice(
    DeviceIntPtr *pDev,
    int id,
    ClientPtr client,
    Mask access_mode,
    int *xkb_err
);

extern int _XkbLookupKeyboard(
    DeviceIntPtr *pDev,
    int id,
    ClientPtr client,
    Mask access_mode,
    int *xkb_err
);

extern int _XkbLookupBellDevice(
    DeviceIntPtr *pDev,
    int id,
    ClientPtr client,
    Mask access_mode,
    int *xkb_err
);

extern int _XkbLookupLedDevice(
    DeviceIntPtr *pDev,
    int id,
    ClientPtr client,
    Mask access_mode,
    int *xkb_err
);

extern int _XkbLookupButtonDevice(
    DeviceIntPtr *pDev,
    int id,
    ClientPtr client,
    Mask access_mode,
    int *xkb_err
);

extern	XkbDescPtr XkbAllocKeyboard(
	void
);

extern	Status XkbAllocClientMap(
	XkbDescPtr		/* xkb */,
	unsigned int		/* which */,
	unsigned int		/* nTypes */
);

extern	Status XkbAllocServerMap(
	XkbDescPtr		/* xkb */,
	unsigned int		/* which */,
	unsigned int		/* nNewActions */
);

extern	void	XkbFreeClientMap(
    XkbDescPtr			/* xkb */,
    unsigned int		/* what */,
    Bool			/* freeMap */
);

extern	void	XkbFreeServerMap(
    XkbDescPtr			/* xkb */,
    unsigned int		/* what */,
    Bool			/* freeMap */
);

extern	Status XkbAllocIndicatorMaps(
	XkbDescPtr		/* xkb */
);

extern	Status	XkbAllocCompatMap(
    XkbDescPtr			/* xkb */,
    unsigned int		/* which */,
    unsigned int		/* nInterpret */
);

extern	Status XkbAllocNames(
	XkbDescPtr		/* xkb */,
	unsigned int		/* which */,
	int			/* nTotalRG */,
	int			/* nTotalAliases */
);

extern	Status	XkbAllocControls(
	XkbDescPtr		/* xkb */,
	unsigned int		/* which*/
);

extern	Status	XkbCopyKeyTypes(
    XkbKeyTypePtr		/* from */,
    XkbKeyTypePtr		/* into */,
    int				/* num_types */
);

extern	Status	XkbResizeKeyType(
    XkbDescPtr		/* xkb */,
    int			/* type_ndx */,
    int			/* map_count */,
    Bool		/* want_preserve */,
    int			/* new_num_lvls */
);

extern	void	XkbFreeKeyboard(
	XkbDescPtr		/* xkb */,
	unsigned int		/* which */,
	Bool			/* freeDesc */
);

extern  void XkbSetActionKeyMods(
	XkbDescPtr		/* xkb */,
	XkbAction *		/* act */,
	unsigned int 		/* mods */
);

extern Bool XkbCheckActionVMods(
	XkbDescPtr		/* xkb */,
	XkbAction *		/* act */,
	unsigned int 		/* changed */
);

extern	unsigned int XkbMaskForVMask(
    XkbDescPtr		/* xkb */,
    unsigned int	/* vmask */
);

extern Bool XkbVirtualModsToReal(
	XkbDescPtr	/* xkb */,
	unsigned int	/* virtua_mask */,
	unsigned int *	/* mask_rtrn */
);

extern	unsigned int	XkbAdjustGroup(
    int			/* group */,
    XkbControlsPtr	/* ctrls */
);

extern KeySym *XkbResizeKeySyms(
    XkbDescPtr		/* xkb */,
    int 		/* key */,
    int 		/* needed */
);

extern XkbAction *XkbResizeKeyActions(
    XkbDescPtr		/* xkb */,
    int 		/* key */,
    int 		/* needed */
);

extern void XkbUpdateKeyTypesFromCore(
    DeviceIntPtr	/* pXDev */,
    KeyCode 		/* first */,
    CARD8 		/* num */,
    XkbChangesPtr	/* pChanges */
);

extern	void XkbUpdateDescActions(	
    XkbDescPtr		/* xkb */,
    KeyCode		/* first */,
    CARD8		/* num */,
    XkbChangesPtr	/* changes */
);

extern void XkbUpdateActions(
    DeviceIntPtr	/* pXDev */,
    KeyCode 		/* first */,
    CARD8 		/* num */,
    XkbChangesPtr  	/* pChanges */,
    unsigned int *	/* needChecksRtrn */,
    XkbEventCausePtr	/* cause */
);

extern void XkbUpdateCoreDescription(
    DeviceIntPtr	/* keybd */,
    Bool		/* resize */
);

extern void XkbApplyMappingChange(
    DeviceIntPtr	/* pXDev */,
    CARD8 		/* request */,
    KeyCode 		/* firstKey */,
    CARD8 		/* num */,
    ClientPtr		/* client */
);

extern void XkbSetIndicators(
    DeviceIntPtr		/* pXDev */,
    CARD32			/* affect */,
    CARD32			/* values */,
    XkbEventCausePtr		/* cause */
);

extern void XkbUpdateIndicators(
    DeviceIntPtr		/* keybd */,
    CARD32		 	/* changed */,
    Bool			/* check_edevs */,
    XkbChangesPtr		/* pChanges */,
    XkbEventCausePtr		/* cause */
);

extern XkbSrvLedInfoPtr XkbAllocSrvLedInfo(
    DeviceIntPtr		/* dev */,
    KbdFeedbackPtr		/* kf */,
    LedFeedbackPtr		/* lf */,
    unsigned int		/* needed_parts */
);

extern XkbSrvLedInfoPtr XkbCopySrvLedInfo(
    DeviceIntPtr		/* dev */,
    XkbSrvLedInfoPtr		/* src */,
    KbdFeedbackPtr		/* kf */,
    LedFeedbackPtr		/* lf */
);


extern XkbSrvLedInfoPtr XkbFindSrvLedInfo(
    DeviceIntPtr		/* dev */,
    unsigned int		/* class */,
    unsigned int		/* id */,
    unsigned int		/* needed_parts */
);

extern void XkbApplyLedNameChanges(
    DeviceIntPtr		/* dev */,
    XkbSrvLedInfoPtr		/* sli */,
    unsigned int		/* changed_names */,
    xkbExtensionDeviceNotify *	/* ed */,
    XkbChangesPtr		/* changes */,
    XkbEventCausePtr		/* cause */
);

extern void XkbApplyLedMapChanges(
    DeviceIntPtr		/* dev */,
    XkbSrvLedInfoPtr		/* sli */,
    unsigned int		/* changed_maps */,
    xkbExtensionDeviceNotify *	/* ed */,
    XkbChangesPtr		/* changes */,
    XkbEventCausePtr		/* cause */
);

extern void XkbApplyLedStateChanges(
    DeviceIntPtr		/* dev */,
    XkbSrvLedInfoPtr		/* sli */,
    unsigned int		/* changed_leds */,
    xkbExtensionDeviceNotify *	/* ed */,
    XkbChangesPtr		/* changes */,
    XkbEventCausePtr		/* cause */
);

extern void XkbFlushLedEvents(	
    DeviceIntPtr		/* dev */,
    DeviceIntPtr		/* kbd */,
    XkbSrvLedInfoPtr		/* sli */,
    xkbExtensionDeviceNotify *	/* ed */,
    XkbChangesPtr		/* changes */,
    XkbEventCausePtr		/* cause */
);

extern unsigned int XkbIndicatorsToUpdate(
    DeviceIntPtr		/* dev */,
    unsigned long		/* state_changes */,
    Bool			/* enabled_ctrl_changes */
);

extern void XkbComputeDerivedState(
    XkbSrvInfoPtr		/* xkbi */
);

extern void XkbCheckSecondaryEffects(
    XkbSrvInfoPtr		/* xkbi */,
    unsigned int		/* which */,
    XkbChangesPtr		/* changes */,
    XkbEventCausePtr		/* cause */
);

extern void XkbCheckIndicatorMaps(
    DeviceIntPtr		/* dev */,
    XkbSrvLedInfoPtr		/* sli */,
    unsigned int		/* which */
);

extern unsigned int XkbStateChangedFlags(
    XkbStatePtr			/* old */,
    XkbStatePtr			/* new */
);

extern	void XkbSendStateNotify(
       DeviceIntPtr	/* kbd */,
       xkbStateNotify *	/* pSN */
);

extern	void XkbSendMapNotify(
       DeviceIntPtr	/* kbd */,
       xkbMapNotify *	/* ev */
);

extern	int  XkbComputeControlsNotify(
	DeviceIntPtr		/* kbd */,
	XkbControlsPtr		/* old */,
	XkbControlsPtr		/* new */,
	xkbControlsNotify *	/* pCN */,
	Bool			/* forceCtrlProc */
);

extern	void XkbSendControlsNotify(
       DeviceIntPtr		/* kbd */,
       xkbControlsNotify *	/* ev */
);

extern	void XkbSendCompatMapNotify(
	DeviceIntPtr		/* kbd */,
	xkbCompatMapNotify *	/* ev */
);

extern	void XkbHandleBell(
       BOOL		/* force */,
       BOOL		/* eventOnly */,
       DeviceIntPtr	/* kbd */,
       CARD8		/* percent */,
       pointer 		/* ctrl */,
       CARD8		/* class */,
       Atom		/* name */,
       WindowPtr	/* pWin */,
       ClientPtr	/* pClient */
);

extern	void XkbSendAccessXNotify(
       DeviceIntPtr		/* kbd */,
       xkbAccessXNotify *	/* pEv */
);

extern	void XkbSendNamesNotify(
       DeviceIntPtr	/* kbd */,
       xkbNamesNotify *	/* ev */
);

extern	void XkbSendCompatNotify(
       DeviceIntPtr		/* kbd */,
       xkbCompatMapNotify *	/* ev */
);

extern	void XkbSendActionMessage(
       DeviceIntPtr		/* kbd */,
       xkbActionMessage *	/* ev */
);

extern	void XkbSendExtensionDeviceNotify(
       DeviceIntPtr			/* kbd */,
       ClientPtr			/* client */,
       xkbExtensionDeviceNotify *	/* ev */
);

extern void XkbSendNotification(
    DeviceIntPtr		/* kbd */,
    XkbChangesPtr		/* pChanges */,
    XkbEventCausePtr		/* cause */
);

extern void XkbProcessKeyboardEvent(
    struct _xEvent * 		/* xE */,
    DeviceIntPtr		/* keybd */,
    int 			/* count */
);

extern void XkbHandleActions(
    DeviceIntPtr		/* dev */,
    DeviceIntPtr		/* kbd */,
    struct _xEvent * 		/* xE */,
    int 			/* count */
);

extern Bool XkbEnableDisableControls(
    XkbSrvInfoPtr	/* xkbi */,
    unsigned long	/* change */,
    unsigned long	/* newValues */,
    XkbChangesPtr	/* changes */,
    XkbEventCausePtr	/* cause */
);

extern void AccessXInit(
    DeviceIntPtr        /* dev */
);

extern Bool AccessXFilterPressEvent(
    struct _xEvent *	/* xE */,
    DeviceIntPtr	/* keybd */,
    int				/* count */
);

extern Bool AccessXFilterReleaseEvent(
    struct _xEvent *	/* xE */,
    DeviceIntPtr	/* keybd */,
    int				/* count */
);

extern void AccessXCancelRepeatKey(
    XkbSrvInfoPtr	/* xkbi */,
    KeyCode		/* key */
);

extern void AccessXComputeCurveFactor(
    XkbSrvInfoPtr	/* xkbi */,
    XkbControlsPtr	/* ctrls */
);

extern	XkbInterestPtr XkbFindClientResource(
       DevicePtr	/* inDev */,
       ClientPtr	/* client */
);

extern	XkbInterestPtr XkbAddClientResource(
       DevicePtr	/* inDev */,
       ClientPtr	/* client */,
       XID		/* id */
);

extern	int XkbRemoveResourceClient(
       DevicePtr	/* inDev */,
       XID		/* id */
);

extern int XkbDDXInitDevice(
    DeviceIntPtr        /* dev */
);

extern	int XkbDDXAccessXBeep(
    DeviceIntPtr        /* dev */,
    unsigned int	/* what */,
    unsigned int	/* which */
);

extern	void XkbDDXKeyClick(
    DeviceIntPtr	/* dev */,
    int			/* keycode */,
    int			/* synthetic */
);

extern 	int XkbDDXUsesSoftRepeat(
    DeviceIntPtr	/* dev */
);

extern	void XkbDDXKeybdCtrlProc(
	DeviceIntPtr	/* dev */,
	KeybdCtrl *	/* ctrl */
);

extern void XkbDDXChangeControls(
	DeviceIntPtr	/* dev */,
	XkbControlsPtr 	/* old */,
	XkbControlsPtr 	/* new */
);

extern void XkbDDXUpdateDeviceIndicators(
	DeviceIntPtr		/* dev */,
	XkbSrvLedInfoPtr	/* sli */,
	CARD32			/* newState */
);

extern void XkbDDXFakePointerButton(
	int 		/* event */,
	int		/* button */
);

extern void XkbDDXFakePointerMotion(
 	unsigned int	/* flags */,
	int		/* x */,
	int		/* y */
);

extern void XkbDDXFakeDeviceButton(
	DeviceIntPtr	/* dev */,
	Bool		/* press */,
	int		/* button */
);

extern int XkbDDXTerminateServer(
	DeviceIntPtr	/* dev */,
	KeyCode		/* key */,
	XkbAction *	/* act */
);

extern int XkbDDXSwitchScreen(
	DeviceIntPtr	/* dev */,
	KeyCode		/* key */,
	XkbAction *	/* act */
);

extern int XkbDDXPrivate(
	DeviceIntPtr	/* dev */,
	KeyCode		/* key */,
	XkbAction *	/* act */
);

extern void XkbDisableComputedAutoRepeats(
	DeviceIntPtr 	/* pXDev */,
	unsigned int	/* key */
);

extern void XkbSetRepeatKeys(
	DeviceIntPtr 	/* pXDev */,
	int		/* key */,
	int	 	/* onoff */
);

extern	int XkbLatchModifiers(
	DeviceIntPtr 	/* pXDev */,
	CARD8 		/* mask */,
	CARD8 		/* latches */
);

extern	int XkbLatchGroup(
	DeviceIntPtr  	/* pXDev */,
	int	  	/* group */
);

extern	void XkbClearAllLatchesAndLocks(
	DeviceIntPtr		/* dev */,
	XkbSrvInfoPtr		/* xkbi */,
	Bool			/* genEv */,
	XkbEventCausePtr	/* cause */
);

extern	void	XkbSetRulesDflts(
	char *			/* rulesFile */,
	char *			/* model */,
	char *			/* layout */,
	char *			/* variant */,
	char *			/* options */
);

extern	void	XkbDeleteRulesDflts(
	void
);

extern	void	XkbInitDevice(
	DeviceIntPtr 	/* pXDev */
);

extern	Bool	XkbInitKeyboardDeviceStruct(
	DeviceIntPtr 		/* pXDev */,
	XkbComponentNamesPtr	/* pNames */,
	KeySymsPtr		/* pSyms */,
	CARD8 			/* pMods */[],
	BellProcPtr		/* bellProc */,
	KbdCtrlProcPtr		/* ctrlProc */
);

extern	int SProcXkbDispatch(
	ClientPtr		/* client */
);

extern XkbGeometryPtr XkbLookupNamedGeometry(
	DeviceIntPtr		/* dev */,
	Atom			/* name */,
	Bool *			/* shouldFree */
);

extern char *	_XkbDupString(
	char *			/* str */
);

extern void	XkbConvertCase(
	KeySym 			/* sym */,
	KeySym *		/* lower */,
	KeySym *		/* upper */
);

extern	Status	 XkbChangeKeycodeRange(	
	XkbDescPtr		/* xkb */,
	int 			/* minKC */,
	int 			/* maxKC */,
	XkbChangesPtr		/* changes */
);

extern int XkbFinishDeviceInit(
	DeviceIntPtr		/* pXDev */
);

extern void XkbFreeSrvLedInfo(
	XkbSrvLedInfoPtr	/* sli */
);

extern void XkbFreeInfo(
	XkbSrvInfoPtr		/* xkbi */
);

extern Status XkbChangeTypesOfKey(
	XkbDescPtr		/* xkb */,
	int			/* key */,
	int			/* nGroups */,
	unsigned int		/* groups */,
	int *			/* newTypesIn */,
	XkbMapChangesPtr	/* changes */
);

extern int XkbKeyTypesForCoreSymbols(
	XkbDescPtr		/* xkb */,
	int			/* map_width */,
	KeySym *		/* core_syms */,
	unsigned int		/* protected */,
	int *			/* types_inout */,
	KeySym *		/* xkb_syms_rtrn */
);

extern Bool XkbApplyCompatMapToKey(
	XkbDescPtr		/* xkb */,
	KeyCode			/* key */,
	XkbChangesPtr		/* changes */
);

extern Bool XkbApplyVirtualModChanges(
	XkbDescPtr		/* xkb */,
	unsigned int		/* changed */,
	XkbChangesPtr		/* changes */
);

extern void XkbSendNewKeyboardNotify(
	DeviceIntPtr		/* kbd */,
	xkbNewKeyboardNotify *	/* pNKN */
);

#include "xkbfile.h"
#include <X11/extensions/XKMformat.h>
#include "xkbrules.h"

#define	_XkbListKeymaps		0
#define	_XkbListKeycodes	1
#define	_XkbListTypes		2
#define	_XkbListCompat		3
#define	_XkbListSymbols		4
#define	_XkbListGeometry	5
#define	_XkbListNumComponents	6

typedef struct _XkbSrvListInfo {
	int		szPool;
	int		nPool;
	char *		pool;

	int		maxRtrn;
	int		nTotal;

	char *		pattern[_XkbListNumComponents];
	int		nFound[_XkbListNumComponents];
} XkbSrvListInfoRec,*XkbSrvListInfoPtr;

extern	Status	XkbDDXList(
	DeviceIntPtr		/* dev */,
	XkbSrvListInfoPtr	/* listing */,
	ClientPtr		/* client */
);

extern	unsigned int XkbDDXLoadKeymapByNames(
	DeviceIntPtr		/* keybd */,
	XkbComponentNamesPtr	/* names */,
	unsigned int		/* want */,
	unsigned int		/* need */,
	XkbDescPtr *		/* finfoRtrn */,
	char *			/* keymapNameRtrn */,
	int 			/* keymapNameRtrnLen */
);

extern	Bool XkbDDXNamesFromRules(
	DeviceIntPtr		/* keybd */,
	char *			/* rules */,
	XkbRF_VarDefsPtr	/* defs */,
	XkbComponentNamesPtr	/* names */
);

extern	Bool XkbDDXApplyConfig(
	void *	/* cfg_in */,
	XkbSrvInfoPtr	/* xkbi */
);

extern	int _XkbStrCaseCmp(
	char *			/* str1 */,
	char *			/* str2 */
);

_XFUNCPROTOEND

#define	XkbAtomGetString(s)	NameForAtom(s)

#endif /* _XKBSRV_H_ */
