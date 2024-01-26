
/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
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
    int			consoleFd;
    int			vtno;
    char *		vtinit;
    Bool		vtSysreq;
    SpecialKeysInDDX	ddxSpecialKeys;

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
#if defined(CSRG_BASED) || defined(__FreeBSD_kernel__)
    int			screenFd;	/* fd for memory mapped access to
					 * vga card */
    int			consType;	/* Which console driver? */
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
    Bool		aiglx;
    MessageType		aiglxFrom;
    Bool		useDefaultFontPath;
    MessageType		useDefaultFontPathFrom;
    Bool        ignoreABI;
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

    Bool        allowEmptyInput;  /* Allow the server to start with no input
                                   * devices. */
    Bool        autoAddDevices; /* Whether to succeed NIDR, or ignore. */
    Bool        autoEnableDevices; /* Whether to enable, or let the client
                                    * control. */
} xf86InfoRec, *xf86InfoPtr;

#ifdef DPMSExtension
/* Private info for DPMS */
typedef struct {
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

/* Information for root window properties. */
typedef struct _RootWinProp {
    struct _RootWinProp *	next;
    char *			name;
    Atom			type;
    short			format;
    long			size;
    pointer			data;
} RootWinProp, *RootWinPropPtr;

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
#if defined(CSRG_BASED) || defined(__FreeBSD_kernel__)
#define PCCONS		   0
#define CODRV011	   1
#define CODRV01X	   2
#define SYSCONS		   8
#define PCVT		  16
#define WSCONS		  32
#endif

#endif /* _XF86PRIVSTR_H */
