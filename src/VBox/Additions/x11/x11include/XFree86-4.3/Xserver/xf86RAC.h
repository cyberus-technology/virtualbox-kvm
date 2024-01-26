/* $XFree86: xc/programs/Xserver/hw/xfree86/rac/xf86RAC.h,v 1.3 1999/06/12 07:19:05 dawes Exp $ */

#ifndef __XF86RAC_H
#define __XF86RAC_H 1

#include "screenint.h"
#include "misc.h"
#include "xf86.h"

Bool xf86RACInit(ScreenPtr pScreen, unsigned int flag);

/* flags */
#define RAC_FB       0x01
#define RAC_CURSOR   0x02
#define RAC_COLORMAP 0x04
#define RAC_VIEWPORT 0x08

#endif /* __XF86RAC_H */
