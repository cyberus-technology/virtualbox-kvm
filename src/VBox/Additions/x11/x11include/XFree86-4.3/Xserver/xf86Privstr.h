/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Privstr.h,v 1.37 2003/02/20 04:05:14 dawes Exp $ */

/*
 * Copyright (c) 1997,1998 by The XFree86 Project, Inc.
 */

/*
 * This file contains definitions of the private XFree86 data structures/types.
 * None of the data structures here should be used by video drivers.
 */ 

#ifndef _XF86PRIVSTR_H
#define _XF86PRIVSTR_H

#include "xf86Pci.h"
#include "xf86str.h"

/* PCI probe flags */

typedef enum {
    PCIProbe1		= 0,
    PCIProbe2,
    PCIForceConfig1,
    PCIForceConfig2,
    PCIForceNone,
    PCIOsConfig
} PciProbeType;

typedef enum {
    LogNone,
    LogFlush,
    LogSync
} Log;

typedef enum {
    SKNever,
    SKWhenNeeded,
    SKAlways
} SpecialKeysInDDX;

/*
 * xf86InfoRec contains global parameters which the video drivers never
 * need to access.  Global parameters which the video drivers do need
 * should be individual globals.
 */

typedef struct {

    /* keyboard part */
    DeviceIntPtr	pKeyboard;
    DeviceProc		kbdProc;		/* procedure for initializing */
    void		(* kbdEvents)(void);	/* proc for processing events */
    int			consoleFd;
    int			kbdFd;
    int			vtno;
    int			kbdType;		/* AT84 / AT101 */
    int			kbdRate;
    int			kbdDelay;
    int			bell_pitch;
    int			bell_duration;
    Bool		autoRepeat;
    unsigned long	leds;
    unsigned long	xleds;
    char *		vtinit;
    int			scanPrefix;		/* scancode-state */
    Bool		capsLock;
    Bool		numLock;
    Bool		scrollLock;
    Bool		modeSwitchLock;
    Bool		composeLock;
    Bool		vtSysreq;
    SpecialKeysInDDX	ddxSpecialKeys;
    Bool		ActionKeyBindingsSet;
#if defined(SVR4) && defined(i386)
    Bool		panix106;
#endif  /* SVR4 && i386 */
#if defined(__OpenBSD__) || defined(__NetBSD__)
    int                 wsKbdType;
#endif

    /* mouse part */
    DeviceIntPtr	pMouse;
#ifdef XINPUT
    pointer		mouseLocal;
#endif

    /* event handler part */
    int			lastEventTime;
    Bool		vtRequestsPending;
    Bool		inputPending;
    Bool		dontVTSwitch;
    Bool		dontZap;
    Bool		dontZoom;
    Bool		notrapSignals;	/* don't exit cleanly - die at fault */
    Bool		caughtSignal;

    /* graphics part */
    Bool		sharedMonitor;
    ScreenPtr		currentScreen;
#ifdef CSRG_BASED
    int			screenFd;	/* fd for memory mapped access to
					 * vga card */
    int			consType;	/* Which console driver? */
#endif

#ifdef XKB
    /* 
     * would like to use an XkbComponentNamesRec here but can't without
     * pulling in a bunch of header files. :-(
     */
    char *		xkbkeymap;
    char *		xkbkeycodes;
    char *		xkbtypes;
    char *		xkbcompat;
    char *		xkbsymbols;
    char *		xkbgeometry;
    Bool		xkbcomponents_specified;
    char *		xkbrules;
    char *		xkbmodel;
    char *		xkblayout;
    char *		xkbvariant;
    char *		xkboptions;
#endif

    /* Other things */
    Bool		allowMouseOpenFail;
    Bool		vidModeEnabled;		/* VidMode extension enabled */
    Bool		vidModeAllowNonLocal;	/* allow non-local VidMode
						 * connections */
    Bool		miscModInDevEnabled;	/* Allow input devices to be
						 * changed */
    Bool		miscModInDevAllowNonLocal;
    PciProbeType	pciFlags;
    Pix24Flags		pixmap24;
    MessageType		pix24From;
#if defined(i386) || defined(__i386__)
    Bool		pc98;
#endif
    Bool		pmFlag;
    Log			log;
    int			estimateSizesAggressively;
    Bool		kbdCustomKeycodes;
    Bool		disableRandR;
    MessageType		randRFrom;
    struct {
	Bool		disabled;		/* enable/disable deactivating
						 * grabs or closing the
						 * connection to the grabbing
						 * client */
	ClientPtr	override;		/* client that disabled
						 * grab deactivation.
						 */
	Bool		allowDeactivate;
	Bool		allowClosedown;
	ServerGrabInfoRec server;
    } grabInfo;
} xf86InfoRec, *xf86InfoPtr;

#ifdef DPMSExtension
/* Private info for DPMS */
typedef struct {
    DPMSSetProcPtr	Set;
    CloseScreenProcPtr	CloseScreen;
    Bool		Enabled;
    int			Flags;
} DPMSRec, *DPMSPtr;
#endif

#ifdef XF86VIDMODE
/* Private info for Video Mode Extentsion */
typedef struct {
    DisplayModePtr	First;
    DisplayModePtr	Next;
    int			Flags;
    CloseScreenProcPtr	CloseScreen;
} VidModeRec, *VidModePtr;
#endif

/* private resource types */
#define ResNoAvoid  ResBios

/* ISC's cc can't handle ~ of UL constants, so explicitly type cast them. */
#define XLED1   ((unsigned long) 0x00000001)
#define XLED2   ((unsigned long) 0x00000002)
#define XLED3   ((unsigned long) 0x00000004)
#define XLED4	((unsigned long) 0x00000008)
#define XCAPS   ((unsigned long) 0x20000000)
#define XNUM    ((unsigned long) 0x40000000)
#define XSCR    ((unsigned long) 0x80000000)
#define XCOMP	((unsigned long) 0x00008000)

/* BSD console driver types (consType) */
#ifdef CSRG_BASED
#define PCCONS		   0
#define CODRV011	   1
#define CODRV01X	   2
#define SYSCONS		   8
#define PCVT		  16
#define WSCONS		  32
#endif

/* Prefix strings for driver messages */
#ifndef X_UNKNOWN_STRING
#define X_UNKNOWN_STRING	"(\?\?)"
#endif
#ifndef X_PROBE_STRING
#define X_PROBE_STRING		"(--)"
#endif
#ifndef X_CONFIG_STRING
#define X_CONFIG_STRING		"(**)"
#endif
#ifndef X_DEFAULT_STRING
#define X_DEFAULT_STRING	"(==)"
#endif
#ifndef X_CMDLINE_STRING
#define X_CMDLINE_STRING	"(++)"
#endif
#ifndef X_NOTICE_STRING
#define X_NOTICE_STRING		"(!!)"
#endif
#ifndef X_ERROR_STRING
#define X_ERROR_STRING		"(EE)"
#endif
#ifndef X_WARNING_STRING
#define X_WARNING_STRING	"(WW)"
#endif
#ifndef X_INFO_STRING
#define X_INFO_STRING		"(II)"
#endif
#ifndef X_NOT_IMPLEMENTED_STRING
#define X_NOT_IMPLEMENTED_STRING	"(NI)"
#endif

#endif /* _XF86PRIVSTR_H */
