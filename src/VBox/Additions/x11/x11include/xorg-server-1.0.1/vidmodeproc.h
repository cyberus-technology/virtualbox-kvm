/* $XFree86: xc/programs/Xserver/Xext/vidmodeproc.h,v 1.4 1999/12/13 01:39:40 robin Exp $ */

/* Prototypes for DGA functions that the DDX must provide */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _VIDMODEPROC_H_
#define _VIDMODEPROC_H_


typedef enum {
    VIDMODE_H_DISPLAY,
    VIDMODE_H_SYNCSTART,
    VIDMODE_H_SYNCEND,
    VIDMODE_H_TOTAL,
    VIDMODE_H_SKEW,
    VIDMODE_V_DISPLAY,
    VIDMODE_V_SYNCSTART,
    VIDMODE_V_SYNCEND,
    VIDMODE_V_TOTAL,
    VIDMODE_FLAGS,
    VIDMODE_CLOCK
} VidModeSelectMode;

typedef enum {
    VIDMODE_MON_VENDOR,
    VIDMODE_MON_MODEL,
    VIDMODE_MON_NHSYNC,
    VIDMODE_MON_NVREFRESH,
    VIDMODE_MON_HSYNC_LO,
    VIDMODE_MON_HSYNC_HI,
    VIDMODE_MON_VREFRESH_LO,
    VIDMODE_MON_VREFRESH_HI
} VidModeSelectMonitor;

typedef union {
  pointer ptr;
  int i;
  float f;
} vidMonitorValue;

void XFree86VidModeExtensionInit(void);

Bool VidModeAvailable(int scrnIndex);
Bool VidModeGetCurrentModeline(int scrnIndex, pointer *mode, int *dotClock);
Bool VidModeGetFirstModeline(int scrnIndex, pointer *mode, int *dotClock);
Bool VidModeGetNextModeline(int scrnIndex, pointer *mode, int *dotClock);
Bool VidModeDeleteModeline(int scrnIndex, pointer mode);
Bool VidModeZoomViewport(int scrnIndex, int zoom);
Bool VidModeGetViewPort(int scrnIndex, int *x, int *y);
Bool VidModeSetViewPort(int scrnIndex, int x, int y);
Bool VidModeSwitchMode(int scrnIndex, pointer mode);
Bool VidModeLockZoom(int scrnIndex, Bool lock);
Bool VidModeGetMonitor(int scrnIndex, pointer *monitor);
int VidModeGetNumOfClocks(int scrnIndex, Bool *progClock);
Bool VidModeGetClocks(int scrnIndex, int *Clocks);
ModeStatus VidModeCheckModeForMonitor(int scrnIndex, pointer mode);
ModeStatus VidModeCheckModeForDriver(int scrnIndex, pointer mode);
void VidModeSetCrtcForMode(int scrnIndex, pointer mode);
Bool VidModeAddModeline(int scrnIndex, pointer mode);
int VidModeGetDotClock(int scrnIndex, int Clock);
int VidModeGetNumOfModes(int scrnIndex);
Bool VidModeSetGamma(int scrnIndex, float red, float green, float blue);
Bool VidModeGetGamma(int scrnIndex, float *red, float *green, float *blue);
pointer VidModeCreateMode(void);
void VidModeCopyMode(pointer modefrom, pointer modeto);
int VidModeGetModeValue(pointer mode, int valtyp);
void VidModeSetModeValue(pointer mode, int valtyp, int val);
vidMonitorValue VidModeGetMonitorValue(pointer monitor, int valtyp, int indx);
Bool VidModeSetGammaRamp(int, int, CARD16 *, CARD16 *, CARD16 *);
Bool VidModeGetGammaRamp(int, int, CARD16 *, CARD16 *, CARD16 *);
int VidModeGetGammaRampSize(int scrnIndex);

#endif


