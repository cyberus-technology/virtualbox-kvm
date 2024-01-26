/* $XFree86: xc/programs/Xserver/hw/xfree86/ramdac/xf86RamDacPriv.h,v 1.3 1998/12/06 06:08:37 dawes Exp $ */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86RamDac.h"
#include "xf86cmap.h"

void RamDacGetRecPrivate(void);
Bool RamDacGetRec(ScrnInfoPtr pScrn);
int  RamDacGetScreenIndex(void);
void RamDacLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
    			LOCO *colors, VisualPtr pVisual);
