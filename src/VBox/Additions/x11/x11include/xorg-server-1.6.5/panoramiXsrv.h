
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _PANORAMIXSRV_H_
#define _PANORAMIXSRV_H_

#include "panoramiX.h"

extern int PanoramiXNumScreens;
extern PanoramiXData *panoramiXdataPtr;
extern int PanoramiXPixWidth;
extern int PanoramiXPixHeight;

extern VisualID PanoramiXTranslateVisualID(int screen, VisualID orig);
extern void PanoramiXConsolidate(void);
extern Bool PanoramiXCreateConnectionBlock(void);
extern PanoramiXRes * PanoramiXFindIDByScrnum(RESTYPE, XID, int);
extern Bool XineramaRegisterConnectionBlockCallback(void (*func)(void));
extern int XineramaDeleteResource(pointer, XID);

extern void XineramaReinitData(ScreenPtr);

extern RegionRec XineramaScreenRegions[MAXSCREENS];

extern unsigned long XRC_DRAWABLE;
extern unsigned long XRT_WINDOW;
extern unsigned long XRT_PIXMAP;
extern unsigned long XRT_GC;
extern unsigned long XRT_COLORMAP;

/*
 * Drivers are allowed to wrap this function.  Each wrapper can decide that the
 * two visuals are unequal, but if they are deemed equal, the wrapper must call
 * down and return FALSE if the wrapped function does.  This ensures that all
 * layers agree that the visuals are equal.  The first visual is always from
 * screen 0.
 */
typedef Bool (*XineramaVisualsEqualProcPtr)(VisualPtr, ScreenPtr, VisualPtr);
extern XineramaVisualsEqualProcPtr XineramaVisualsEqualPtr;

extern void XineramaGetImageData(
    DrawablePtr *pDrawables,
    int left,
    int top,
    int width, 
    int height,
    unsigned int format,
    unsigned long planemask,
    char *data,
    int pitch,
    Bool isRoot
);

#endif /* _PANORAMIXSRV_H_ */
