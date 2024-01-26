/*
 * Copyright (c) 2001-2004 Torrey T. Lyons. All Rights Reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#ifndef _DARWIN_H
#define _DARWIN_H

#include <IOKit/IOTypes.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include <X11/extensions/XKB.h>
#include <assert.h>

typedef struct {
    void                *framebuffer;
    int                 x;
    int                 y;
    int                 width;
    int                 height;
    int                 pitch;
    int                 colorType;
    int                 bitsPerPixel;
    int                 colorBitsPerPixel;
    int                 bitsPerComponent;
} DarwinFramebufferRec, *DarwinFramebufferPtr;


// From darwin.c
void DarwinPrintBanner();
int DarwinParseModifierList(const char *constmodifiers);
void DarwinAdjustScreenOrigins(ScreenInfo *pScreenInfo);
void xf86SetRootClip (ScreenPtr pScreen, BOOL enable);

// From darwinEvents.c
Bool DarwinEQInit(DevicePtr pKbd, DevicePtr pPtr);
void DarwinEQEnqueue(const xEvent *e);
void DarwinEQPointerPost(xEvent *e);
void DarwinEQSwitchScreen(ScreenPtr pScreen, Bool fromDIX);
void DarwinPokeEQ(void);
void DarwinSendPointerEvents(int ev_type, int ev_button, int pointer_x, int pointer_y);
void DarwinSendKeyboardEvents(int ev_type, int keycode);
void DarwinSendScrollEvents(float count, int pointer_x, int pointer_y);

// From darwinKeyboard.c
int DarwinModifierNXKeyToNXKeycode(int key, int side);
void DarwinKeyboardInit(DeviceIntPtr pDev);
int DarwinModifierNXKeycodeToNXKey(unsigned char keycode, int *outSide);
int DarwinModifierNXKeyToNXMask(int key);
int DarwinModifierNXMaskToNXKey(int mask);
int DarwinModifierStringToNXKey(const char *string);

// Mode specific functions
Bool DarwinModeAddScreen(int index, ScreenPtr pScreen);
Bool DarwinModeSetupScreen(int index, ScreenPtr pScreen);
void DarwinModeInitOutput(int argc,char **argv);
void DarwinModeInitInput(int argc, char **argv);
int DarwinModeProcessArgument(int argc, char *argv[], int i);
void DarwinModeProcessEvent(xEvent *xe);
void DarwinModeGiveUp(void);
void DarwinModeBell(int volume, DeviceIntPtr pDevice, pointer ctrl, int class);


#undef assert
#define assert(x) { if ((x) == 0) \
    FatalError("assert failed on line %d of %s!\n", __LINE__, __FILE__); }
#define kern_assert(x) { if ((x) != KERN_SUCCESS) \
    FatalError("assert failed on line %d of %s with kernel return 0x%x!\n", \
                __LINE__, __FILE__, x); }
#define SCREEN_PRIV(pScreen) \
    ((DarwinFramebufferPtr)pScreen->devPrivates[darwinScreenIndex].ptr)


#define MIN_KEYCODE XkbMinLegalKeyCode     // unfortunately, this isn't 0...


/*
 * Global variables from darwin.c
 */
extern int              darwinScreenIndex; // index into pScreen.devPrivates
extern int              darwinScreensFound;
extern io_connect_t     darwinParamConnect;
extern int              darwinEventReadFD;
extern int              darwinEventWriteFD;
extern DeviceIntPtr     darwinPointer;
extern DeviceIntPtr     darwinKeyboard;

// User preferences
extern int              darwinMouseAccelChange;
extern int              darwinFakeButtons;
extern int              darwinFakeMouse2Mask;
extern int              darwinFakeMouse3Mask;
extern int              darwinSwapAltMeta;
extern char            *darwinKeymapFile;
extern int              darwinSyncKeymap;
extern unsigned int     darwinDesiredWidth, darwinDesiredHeight;
extern int              darwinDesiredDepth;
extern int              darwinDesiredRefresh;

// location of X11's (0,0) point in global screen coordinates
extern int              darwinMainScreenX;
extern int              darwinMainScreenY;


/*
 * Special ddx events understood by the X server
 */
enum {
    kXDarwinUpdateModifiers   // update all modifier keys
            = LASTEvent+1,    // (from X.h list of event names)
    kXDarwinUpdateButtons,    // update state of mouse buttons 2 and up
    kXDarwinScrollWheel,      // scroll wheel event

    /*
     * Quartz-specific events -- not used in IOKit mode
     */
    kXDarwinActivate,         // restore X drawing and cursor
    kXDarwinDeactivate,       // clip X drawing and switch to Aqua cursor
    kXDarwinSetRootClip,      // enable or disable drawing to the X screen
    kXDarwinQuit,             // kill the X server and release the display
    kXDarwinReadPasteboard,   // copy Mac OS X pasteboard into X cut buffer
    kXDarwinWritePasteboard,  // copy X cut buffer onto Mac OS X pasteboard
    /*
     * AppleWM events
     */
    kXDarwinControllerNotify, // send an AppleWMControllerNotify event
    kXDarwinPasteboardNotify, // notify the WM to copy or paste
    /*
     * Xplugin notification events
     */
    kXDarwinDisplayChanged,   // display configuration has changed
    kXDarwinWindowState,      // window visibility state has changed
    kXDarwinWindowMoved       // window has moved on screen
};

#endif  /* _DARWIN_H */
