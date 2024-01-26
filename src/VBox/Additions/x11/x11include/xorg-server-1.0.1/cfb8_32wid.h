/* $XFree86$ */

#ifndef _CFB8_32WID_H
#define _CFB8_32WID_H

#include "regionstr.h"
#include "windowstr.h"

typedef struct {
	unsigned int (*WidGet)(WindowPtr);
	Bool (*WidAlloc)(WindowPtr);
	void (*WidFree)(WindowPtr);
	void (*WidFillBox)(DrawablePtr, DrawablePtr, int, BoxPtr);
	void (*WidCopyArea)(DrawablePtr, RegionPtr, DDXPointPtr);
} cfb8_32WidOps;

typedef struct {
	pointer 		pix8;
	int			width8;
	pointer 		pix32;
	int			width32;

	/* WID information */
	pointer			pixWid;
	int			widthWid;
	int			bitsPerWid;
	cfb8_32WidOps		*WIDOps;
} cfb8_32WidScreenRec, *cfb8_32WidScreenPtr;

extern int cfb8_32WidScreenPrivateIndex; /* XXX */
extern int cfb8_32WidGetScreenPrivateIndex(void);

Bool
cfb8_32WidScreenInit (
    ScreenPtr pScreen,
    pointer pbits32,
    pointer pbits8,
    pointer pbitsWid,
    int xsize, int ysize,
    int dpix, int dpiy,	
    int width32,
    int width8,
    int widthWid,
    int bitsPerWid,
    cfb8_32WidOps *WIDOps
);

/* cfbwindow.c */

void
cfb8_32WidPaintWindow (
    WindowPtr   pWin,
    RegionPtr   pRegion,
    int         what
);

Bool cfb8_32WidCreateWindow(WindowPtr pWin);
Bool cfb8_32WidDestroyWindow(WindowPtr pWin);

Bool
cfb8_32WidPositionWindow(
    WindowPtr pWin,
    int x, int y
);

void
cfb8_32WidCopyWindow(
    WindowPtr pWin,
    DDXPointRec ptOldOrg,
    RegionPtr prgnSrc
);

Bool
cfb8_32WidChangeWindowAttributes(
    WindowPtr pWin,
    unsigned long mask
);

void
cfb8_32WidWindowExposures(
   WindowPtr pWin,
   RegionPtr pReg,
   RegionPtr pOtherReg
);

/* cfbwid.c */

Bool
cfb8_32WidGenericOpsInit(cfb8_32WidScreenPtr pScreenPriv);

#define CFB8_32WID_GET_SCREEN_PRIVATE(pScreen)\
   (cfb8_32WidScreenPtr)((pScreen)->devPrivates[cfb8_32WidGetScreenPrivateIndex()].ptr)

#endif /* _CFB8_32WID_H */
