/* $Xorg: opaque.h,v 1.4 2001/02/09 02:05:15 xorgcvs Exp $ */
/*

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/programs/Xserver/include/opaque.h,v 1.11 2002/10/08 23:55:22 dawes Exp $ */

#ifndef OPAQUE_H
#define OPAQUE_H

#include <X11/Xmd.h>

#include "globals.h"

extern char *defaultTextFont;
extern char *defaultCursorFont;
extern int MaxClients;
extern char isItTimeToYield;
extern char dispatchException;

/* bit values for dispatchException */
#define DE_RESET     1
#define DE_TERMINATE 2
#define DE_PRIORITYCHANGE 4  /* set when a client's priority changes */

extern CARD32 TimeOutValue;
extern int ScreenSaverBlanking;
extern int ScreenSaverAllowExposures;
extern int defaultScreenSaverBlanking;
extern int defaultScreenSaverAllowExposures;
extern int argcGlobal;
extern char **argvGlobal;
extern char *display;

extern int defaultBackingStore;
extern Bool disableBackingStore;
extern Bool enableBackingStore;
extern Bool disableSaveUnders;
extern Bool PartialNetwork;
#ifndef NOLOGOHACK
extern int logoScreenSaver;
#endif
#ifdef RLIMIT_DATA
extern int limitDataSpace;
#endif
#ifdef RLIMIT_STACK
extern int limitStackSpace;
#endif
#ifdef RLIMIT_NOFILE
extern int limitNoFile;
#endif
extern Bool permitOldBugs;
extern Bool defeatAccessControl;
extern char* protNoListen;
extern Bool blackRoot;



#endif /* OPAQUE_H */
