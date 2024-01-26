/* $XFree86: xc/programs/Xserver/hw/xfree86/xf8_16bpp/cfb8_16.h,v 1.1 1999/01/31 12:22:16 dawes Exp $ */

#ifndef _CFB8_16_H
#define _CFB8_16_H

#include "regionstr.h"
#include "windowstr.h"

typedef struct {
   pointer 		pix8;
   int			width8;
   pointer 		pix16;
   int			width16;
   unsigned char	key;
} cfb8_16ScreenRec, *cfb8_16ScreenPtr;

extern int cfb8_16ScreenPrivateIndex; /* XXX */
extern int cfb8_16GetScreenPrivateIndex(void);

Bool
cfb8_16ScreenInit (
    ScreenPtr pScreen,
    pointer pbits16,
    pointer pbits8,
    int xsize, int ysize,
    int dpix, int dpiy,	
    int width16,
    int width8
);

void
cfb8_16PaintWindow (
    WindowPtr   pWin,
    RegionPtr   pRegion,
    int         what
);

Bool cfb8_16CreateWindow(WindowPtr pWin);
Bool cfb8_16DestroyWindow(WindowPtr pWin);

Bool
cfb8_16PositionWindow(
    WindowPtr pWin,
    int x, int y
);

void
cfb8_16CopyWindow(
    WindowPtr pWin,
    DDXPointRec ptOldOrg,
    RegionPtr prgnSrc
);

Bool
cfb8_16ChangeWindowAttributes(
    WindowPtr pWin,
    unsigned long mask
);

void
cfb8_16WindowExposures(
   WindowPtr pWin,
   RegionPtr pReg,
   RegionPtr pOtherReg
);

#define CFB8_16_GET_SCREEN_PRIVATE(pScreen)\
   (cfb8_16ScreenPtr)((pScreen)->devPrivates[cfb8_16GetScreenPrivateIndex()].ptr)

#endif /* _CFB8_16_H */
