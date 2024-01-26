/* $XFree86: xc/programs/Xserver/mi/micmap.h,v 1.7 2000/09/20 00:09:15 keithp Exp $ */

#include "colormapst.h"

#ifndef _MICMAP_H_
#define _MICMAP_H_

extern ColormapPtr miInstalledMaps[MAXSCREENS];

typedef Bool (* miInitVisualsProcPtr)(VisualPtr *, DepthPtr *, int *, int *,
					int *, VisualID *, unsigned long, int,
					int);

extern miInitVisualsProcPtr miInitVisualsProc;
					
int miListInstalledColormaps(ScreenPtr pScreen, Colormap *pmaps);
void miInstallColormap(ColormapPtr pmap);
void miUninstallColormap(ColormapPtr pmap);

void miResolveColor(unsigned short *, unsigned short *, unsigned short *,
			VisualPtr);
Bool miInitializeColormap(ColormapPtr);
int miExpandDirectColors(ColormapPtr, int, xColorItem *, xColorItem *);
Bool miCreateDefColormap(ScreenPtr);
void miClearVisualTypes(void);
Bool miSetVisualTypes(int, int, int, int);
Bool miSetPixmapDepths(void);
Bool miSetVisualTypesAndMasks(int depth, int visuals, int bitsPerRGB, 
			      int preferredCVC,
			      Pixel redMask, Pixel greenMask, Pixel blueMask);
int miGetDefaultVisualMask(int);
Bool miInitVisuals(VisualPtr *, DepthPtr *, int *, int *, int *, VisualID *,
			unsigned long, int, int);
void miResetInitVisuals(void);

void miHookInitVisuals(void (**old)(miInitVisualsProcPtr *),
		       void (*new)(miInitVisualsProcPtr *));


#define MAX_PSEUDO_DEPTH	10
#define MIN_TRUE_DEPTH		6

#define StaticGrayMask	(1 << StaticGray)
#define GrayScaleMask	(1 << GrayScale)
#define StaticColorMask	(1 << StaticColor)
#define PseudoColorMask	(1 << PseudoColor)
#define TrueColorMask	(1 << TrueColor)
#define DirectColorMask	(1 << DirectColor)
                
#define ALL_VISUALS	(StaticGrayMask|\
			 GrayScaleMask|\
			 StaticColorMask|\
			 PseudoColorMask|\
			 TrueColorMask|\
			 DirectColorMask)

#define LARGE_VISUALS	(TrueColorMask|\
			 DirectColorMask)

#define SMALL_VISUALS	(StaticGrayMask|\
			 GrayScaleMask|\
			 StaticColorMask|\
			 PseudoColorMask)

#endif /* _MICMAP_H_ */
