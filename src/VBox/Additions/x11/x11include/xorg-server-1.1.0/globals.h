/* $XdotOrg: xserver/xorg/include/globals.h,v 1.10.10.1 2006/04/05 21:23:06 fredrik Exp $ */
/* $XFree86: xc/programs/Xserver/include/globals.h,v 1.3 1999/09/25 14:38:21 dawes Exp $ */

#ifndef _XSERV_GLOBAL_H_
#define _XSERV_GLOBAL_H_

#include "window.h"	/* for WindowPtr */

/* Global X server variables that are visible to mi, dix, os, and ddx */

extern CARD32 defaultScreenSaverTime;
extern CARD32 defaultScreenSaverInterval;
extern CARD32 ScreenSaverTime;
extern CARD32 ScreenSaverInterval;

#ifdef SCREENSAVER
extern Bool screenSaverSuspended;
#endif

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
extern Bool PanoramiXMapped;
extern Bool PanoramiXVisibilityNotifySent;
extern Bool PanoramiXWindowExposureSent;
extern Bool PanoramiXOneExposeRequest;
#endif

#ifdef BIGREQS
extern Bool noBigReqExtension;
#endif

#ifdef COMPOSITE
extern Bool noCompositeExtension;
#endif

#ifdef DAMAGE
extern Bool noDamageExtension;
#endif

#ifdef DBE
extern Bool noDbeExtension;
#endif

#ifdef DPMSExtension
extern Bool noDPMSExtension;
#endif

#ifdef EVI
extern Bool noEVIExtension;
#endif

#ifdef FONTCACHE
extern Bool noFontCacheExtension;
#endif

#ifdef GLXEXT
extern Bool noGlxExtension;
#endif

#ifdef LBX
extern Bool noLbxExtension;
#endif

#ifdef SCREENSAVER
extern Bool noScreenSaverExtension;
#endif

#ifdef MITSHM
extern Bool noMITShmExtension;
#endif

#ifdef MITMISC
extern Bool noMITMiscExtension;
#endif

#ifdef MULTIBUFFER
extern Bool noMultibufferExtension;
#endif

#ifdef RANDR
extern Bool noRRExtension;
#endif

#ifdef RENDER
extern Bool noRenderExtension;
#endif

#ifdef SHAPE
extern Bool noShapeExtension;
#endif

#ifdef XCSECURITY
extern Bool noSecurityExtension;
#endif

#ifdef XSYNC
extern Bool noSyncExtension;
#endif

#ifdef TOGCUP
extern Bool noXcupExtension;
#endif

#ifdef RES
extern Bool noResExtension;
#endif

#ifdef XAPPGROUP
extern Bool noXagExtension;
#endif

#ifdef XCMISC
extern Bool noXCMiscExtension;
#endif

#ifdef XEVIE
extern Bool noXevieExtension;
#endif

#ifdef XF86BIGFONT
extern Bool noXFree86BigfontExtension;
#endif

#ifdef XFreeXDGA
extern Bool noXFree86DGAExtension;
#endif

#ifdef XF86DRI
extern Bool noXFree86DRIExtension;
#endif

#ifdef XF86MISC
extern Bool noXFree86MiscExtension;
#endif

#ifdef XF86VIDMODE
extern Bool noXFree86VidModeExtension;
#endif

#ifdef XFIXES
extern Bool noXFixesExtension;
#endif

#ifdef XKB
/* |noXkbExtension| is defined in xc/programs/Xserver/xkb/xkbInit.c */
extern Bool noXkbExtension;
#endif

#ifdef PANORAMIX
extern Bool noPanoramiXExtension;
#endif

#ifdef XINPUT
extern Bool noXInputExtension;
#endif

#ifdef XIDLE
extern Bool noXIdleExtension;
#endif

#ifdef XV
extern Bool noXvExtension;
#endif

#endif /* !_XSERV_GLOBAL_H_ */
