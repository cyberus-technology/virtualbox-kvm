/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86cmap.h,v 1.7 2001/05/06 00:49:12 mvojkovi Exp $ */

#ifndef _XF86CMAP_H
#define _XF86CMAP_H

#include "xf86str.h"
#include "colormapst.h"

#define CMAP_PALETTED_TRUECOLOR		0x0000001
#define CMAP_RELOAD_ON_MODE_SWITCH	0x0000002
#define CMAP_LOAD_EVEN_IF_OFFSCREEN	0x0000004

typedef void (*LoadPaletteFuncPtr)(
    ScrnInfoPtr pScrn, 
    int numColors, 
    int *indicies,
    LOCO *colors,
    VisualPtr pVisual
);

typedef void (*SetOverscanFuncPtr)(
    ScrnInfoPtr pScrn,
    int Index
);

Bool xf86HandleColormaps(
    ScreenPtr pScreen,
    int maxCol,
    int sigRGBbits,
    LoadPaletteFuncPtr loadPalette,
    SetOverscanFuncPtr setOverscan,
    unsigned int flags
);

int
xf86ChangeGamma(
   ScreenPtr pScreen,
   Gamma newGamma
);

int
xf86ChangeGammaRamp(
   ScreenPtr pScreen,
   int size,
   unsigned short *red,
   unsigned short *green,
   unsigned short *blue
);

int xf86GetGammaRampSize(ScreenPtr pScreen);

int
xf86GetGammaRamp(
   ScreenPtr pScreen,
   int size,
   unsigned short *red,
   unsigned short *green,
   unsigned short *blue
);

#endif /* _XF86CMAP_H */

