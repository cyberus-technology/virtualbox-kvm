
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _PANORAMIXSRV_H_
#define _PANORAMIXSRV_H_

#include "panoramiX.h"

extern _X_EXPORT int PanoramiXNumScreens;
extern _X_EXPORT PanoramiXData *panoramiXdataPtr;
extern _X_EXPORT int PanoramiXPixWidth;
extern _X_EXPORT int PanoramiXPixHeight;

extern _X_EXPORT VisualID PanoramiXTranslateVisualID(int screen, VisualID orig);
extern _X_EXPORT void PanoramiXConsolidate(void);
extern _X_EXPORT Bool PanoramiXCreateConnectionBlock(void);
extern _X_EXPORT PanoramiXRes * PanoramiXFindIDByScrnum(RESTYPE, XID, int);
extern _X_EXPORT Bool XineramaRegisterConnectionBlockCallback(void (*func)(void));
extern _X_EXPORT int XineramaDeleteResource(pointer, XID);

extern _X_EXPORT void XineramaReinitData(ScreenPtr);

extern _X_EXPORT RegionRec XineramaScreenRegions[MAXSCREENS];

extern _X_EXPORT unsigned long XRC_DRAWABLE;
extern _X_EXPORT unsigned long XRT_WINDOW;
extern _X_EXPORT unsigned long XRT_PIXMAP;
extern _X_EXPORT unsigned long XRT_GC;
extern _X_EXPORT unsigned long XRT_COLORMAP;

/*
 * Drivers are allowed to wrap this function.  Each wrapper can decide that the
 * two visuals are unequal, but if they are deemed equal, the wrapper must call
 * down and return FALSE if the wrapped function does.  This ensures that all
 * layers agree that the visuals are equal.  The first visual is always from
 * screen 0.
 */
typedef Bool (*XineramaVisualsEqualProcPtr)(VisualPtr, ScreenPtr, VisualPtr);
extern _X_EXPORT XineramaVisualsEqualProcPtr XineramaVisualsEqualPtr;

extern _X_EXPORT void XineramaGetImageData(
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
