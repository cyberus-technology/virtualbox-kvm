
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
extern int monitorResolution;
extern int defaultColorVisualClass;

extern WindowPtr WindowTable[MAXSCREENS];
extern int GrabInProgress;
extern Bool noTestExtensions;

extern DDXPointRec dixScreenOrigins[MAXSCREENS];

extern char *ConnectionInfo;

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
extern Bool PanoramiXExtensionDisabledHack;
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

#ifdef GLXEXT
extern Bool noGlxExtension;
#endif

#ifdef SCREENSAVER
extern Bool noScreenSaverExtension;
#endif

#ifdef MITSHM
extern Bool noMITShmExtension;
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

#ifdef XCSECURITY
extern Bool noSecurityExtension;
#endif

#ifdef RES
extern Bool noResExtension;
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

#ifdef XSELINUX
extern Bool noSELinuxExtension;

#define SELINUX_MODE_DEFAULT    0
#define SELINUX_MODE_DISABLED   1
#define SELINUX_MODE_PERMISSIVE 2
#define SELINUX_MODE_ENFORCING  3
extern int selinuxEnforcingState;
#endif

#ifdef XV
extern Bool noXvExtension;
#endif

#endif /* !_XSERV_GLOBAL_H_ */
