/*
 * $XFree86: xc/programs/Xserver/randr/randrstr.h,v 1.7 2002/10/14 18:01:42 keithp Exp $
 *
 * Copyright © 2000 Compaq Computer Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Compaq not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Compaq makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * COMPAQ DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL COMPAQ BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _RANDRSTR_H_
#define _RANDRSTR_H_

#include "randr.h"

typedef struct _rrScreenRate {
    int		    rate;
    Bool	    referenced;
    Bool	    oldReferenced;
} RRScreenRate, *RRScreenRatePtr;

typedef struct _rrScreenSize {
    int		    id;
    short	    width, height;
    short	    mmWidth, mmHeight;
    RRScreenRatePtr pRates;
    int		    nRates;
    int		    nRatesInUse;
    Bool	    referenced;
    Bool	    oldReferenced;
} RRScreenSize, *RRScreenSizePtr;

typedef Bool (*RRSetConfigProcPtr) (ScreenPtr		pScreen,
				    Rotation		rotation,
				    int			rate,
				    RRScreenSizePtr	pSize);

typedef Bool (*RRGetInfoProcPtr) (ScreenPtr pScreen, Rotation *rotations);
typedef Bool (*RRCloseScreenProcPtr) ( int i, ScreenPtr pscreen);
	
typedef struct _rrScrPriv {
    RRSetConfigProcPtr	    rrSetConfig;
    RRGetInfoProcPtr	    rrGetInfo;
    
    TimeStamp		    lastSetTime;	/* last changed by client */
    TimeStamp		    lastConfigTime;	/* possible configs changed */
    RRCloseScreenProcPtr    CloseScreen;

    /*
     * Configuration information
     */
    Rotation		    rotations;
    
    int			    nSizes;
    int			    nSizesInUse;
    RRScreenSizePtr	    pSizes;

    /*
     * Current state
     */
    Rotation		    rotation;
    int			    size;
    int			    rate;
} rrScrPrivRec, *rrScrPrivPtr;

extern int rrPrivIndex;

#define rrGetScrPriv(pScr)  ((rrScrPrivPtr) (pScr)->devPrivates[rrPrivIndex].ptr)
#define rrScrPriv(pScr)	rrScrPrivPtr    pScrPriv = rrGetScrPriv(pScr)
#define SetRRScreen(s,p) ((s)->devPrivates[rrPrivIndex].ptr = (pointer) (p))

/* Initialize the extension */
void
RRExtensionInit (void);

/*
 * Then, register the specific size with the screen
 */

RRScreenSizePtr
RRRegisterSize (ScreenPtr		pScreen,
		short			width, 
		short			height,
		short			mmWidth,
		short			mmHeight);

Bool RRRegisterRate (ScreenPtr		pScreen,
		     RRScreenSizePtr	pSize,
		     int		rate);

/*
 * Finally, set the current configuration of the screen
 */

void
RRSetCurrentConfig (ScreenPtr		pScreen,
		    Rotation		rotation,
		    int			rate,
		    RRScreenSizePtr	pSize);

Bool RRScreenInit(ScreenPtr pScreen);
    
Bool
miRandRInit (ScreenPtr pScreen);

Bool
miRRGetInfo (ScreenPtr pScreen, Rotation *rotations);

Bool
miRRSetConfig (ScreenPtr	pScreen,
	       Rotation		rotation,
	       int		rate,
	       RRScreenSizePtr	size);

Bool
miRRGetScreenInfo (ScreenPtr pScreen);

#endif /* _RANDRSTR_H_ */
