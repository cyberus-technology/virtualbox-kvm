
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _CFB8_32_H
#define _CFB8_32_H

#include "gcstruct.h"

typedef struct {
   GCOps		*Ops8bpp;
   GCOps 		*Ops32bpp;
   unsigned long	changes;	
   Bool			OpsAre8bpp;  
} cfb8_32GCRec, *cfb8_32GCPtr;

typedef struct {
   unsigned char	key;
   void                (*EnableDisableFBAccess)(int scrnIndex, Bool enable);
   pointer		visualData;
} cfb8_32ScreenRec, *cfb8_32ScreenPtr;


extern DevPrivateKey cfb8_32GetGCPrivateKey(void);
extern DevPrivateKey cfb8_32GetScreenPrivateKey(void);

RegionPtr
cfb8_32CopyArea(
    DrawablePtr pSrcDraw,
    DrawablePtr pDstDraw,
    GC *pGC,
    int srcx, int srcy,
    int width, int height,
    int dstx, int dsty 
);

void 
cfbDoBitblt8To32(
    DrawablePtr pSrc, 
    DrawablePtr pDst, 
    int rop,
    RegionPtr prgnDst, 
    DDXPointPtr pptSrc,
    unsigned long planemask
);

void 
cfbDoBitblt32To8(
    DrawablePtr pSrc, 
    DrawablePtr pDst, 
    int rop,
    RegionPtr prgnDst, 
    DDXPointPtr pptSrc,
    unsigned long planemask
);


void
cfb8_32ValidateGC8(
    GCPtr  		pGC,
    unsigned long 	changes,
    DrawablePtr		pDrawable
);

void
cfb8_32ValidateGC32(
    GCPtr  		pGC,
    unsigned long 	changes,
    DrawablePtr		pDrawable
);

void
cfb32ValidateGC_Underlay(
    GCPtr  		pGC,
    unsigned long 	changes,
    DrawablePtr		pDrawable
);

Bool cfb8_32CreateGC(GCPtr pGC);

void
cfb8_32GetSpans(
   DrawablePtr pDraw,
   int wMax,
   DDXPointPtr ppt,
   int *pwidth,
   int nspans,
   char *pchardstStart
);

void
cfb8_32PutImage (
    DrawablePtr pDraw,
    GCPtr pGC,
    int depth, 
    int x, int y, int w, int h,
    int leftPad,
    int format,
    char *pImage
);

void
cfb8_32GetImage (
    DrawablePtr pDraw,
    int sx, int sy, int w, int h,
    unsigned int format,
    unsigned long planeMask,
    char *pdstLine
);

Bool
cfb8_32ScreenInit (
    ScreenPtr pScreen,
    pointer pbits,
    int xsize, int ysize,
    int dpix, int dpiy,	
    int width
);

void
cfb8_32FillBoxSolid8 (
   DrawablePtr pDraw,
   int nbox,
   BoxPtr pBox,
   unsigned long color
);

RegionPtr 
cfb8_32CopyPlane(
    DrawablePtr pSrc,
    DrawablePtr pDst,
    GCPtr pGC,
    int srcx, int srcy,
    int width, int height,
    int dstx, int dsty,
    unsigned long bitPlane
);

void 
cfbDoBitblt8To8GXcopy(
    DrawablePtr pSrc, 
    DrawablePtr pDst, 
    int rop,
    RegionPtr prgnDst, 
    DDXPointPtr pptSrc,
    unsigned long pm
);

void 
cfbDoBitblt24To24GXcopy(
    DrawablePtr pSrc, 
    DrawablePtr pDst, 
    int rop,
    RegionPtr prgnDst, 
    DDXPointPtr pptSrc,
    unsigned long pm
);

Bool cfb8_32CreateWindow(WindowPtr pWin);
Bool cfb8_32DestroyWindow(WindowPtr pWin);

Bool
cfb8_32PositionWindow(
    WindowPtr pWin,
    int x, int y
);

void
cfb8_32CopyWindow(
    WindowPtr pWin,
    DDXPointRec ptOldOrg,
    RegionPtr prgnSrc
);

Bool
cfb8_32ChangeWindowAttributes(
    WindowPtr pWin,
    unsigned long mask
);


#define CFB8_32_GET_GC_PRIVATE(pGC) ((cfb8_32GCPtr) \
    dixLookupPrivate(&(pGC)->devPrivates, cfb8_32GetGCPrivateKey()))

#define CFB8_32_GET_SCREEN_PRIVATE(pScreen) ((cfb8_32ScreenPtr) \
    dixLookupPrivate(&(pScreen)->devPrivates, cfb8_32GetScreenPrivateKey()))

Bool xf86Overlay8Plus32Init (ScreenPtr pScreen);

#endif /* _CFB8_32_H */
