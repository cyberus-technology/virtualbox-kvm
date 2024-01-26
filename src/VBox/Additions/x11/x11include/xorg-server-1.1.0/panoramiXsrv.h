/* $XFree86: xc/programs/Xserver/Xext/panoramiXsrv.h,v 1.8 2001/08/01 00:44:44 tsi Exp $ */

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
extern RegionRec PanoramiXScreenRegion;
extern XID *PanoramiXVisualTable;

extern void PanoramiXConsolidate(void);
extern Bool PanoramiXCreateConnectionBlock(void);
extern PanoramiXRes * PanoramiXFindIDByScrnum(RESTYPE, XID, int);
extern PanoramiXRes * PanoramiXFindIDOnAnyScreen(RESTYPE, XID);
extern WindowPtr PanoramiXChangeWindow(int, WindowPtr);
extern Bool XineramaRegisterConnectionBlockCallback(void (*func)(void));
extern int XineramaDeleteResource(pointer, XID);

extern void XineramaReinitData(ScreenPtr);

extern RegionRec XineramaScreenRegions[MAXSCREENS];

extern unsigned long XRC_DRAWABLE;
extern unsigned long XRT_WINDOW;
extern unsigned long XRT_PIXMAP;
extern unsigned long XRT_GC;
extern unsigned long XRT_COLORMAP;

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
