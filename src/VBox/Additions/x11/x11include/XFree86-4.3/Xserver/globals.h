/* $XFree86: xc/programs/Xserver/include/globals.h,v 1.4 2000/06/28 18:21:22 tsi Exp $ */

#ifndef _XSERV_GLOBAL_H_
#define _XSERV_GLOBAL_H_

#include "window.h"	/* for WindowPtr */

/* Global X server variables that are visible to mi, dix, os, and ddx */

extern CARD32 defaultScreenSaverTime;
extern CARD32 defaultScreenSaverInterval;
extern CARD32 ScreenSaverTime;
extern CARD32 ScreenSaverInterval;

extern char *defaultFontPath;
extern char *rgbPath;
extern int monitorResolution;
extern Bool loadableFonts;
extern int defaultColorVisualClass;

extern Bool Must_have_memory;
extern WindowPtr *WindowTable;
extern int GrabInProgress;
extern Bool noTestExtensions;

extern DDXPointRec dixScreenOrigins[MAXSCREENS];

#ifdef DPMSExtension
extern CARD32 defaultDPMSStandbyTime;
extern CARD32 defaultDPMSSuspendTime;
extern CARD32 defaultDPMSOffTime;
extern CARD32 DPMSStandbyTime;
extern CARD32 DPMSSuspendTime;
extern CARD32 DPMSOffTime;
extern CARD16 DPMSPowerLevel;
extern Bool defaultDPMSEnabled;
extern Bool DPMSEnabled;
extern Bool DPMSEnabledSwitch;
extern Bool DPMSDisabledSwitch;
extern Bool DPMSCapableFlag;
#endif

#ifdef PANORAMIX
extern Bool noPanoramiXExtension;
extern Bool PanoramiXMapped;
extern Bool PanoramiXVisibilityNotifySent;
extern Bool PanoramiXWindowExposureSent;
extern Bool PanoramiXOneExposeRequest;
#endif


#endif /* _XSERV_GLOBAL_H_ */
