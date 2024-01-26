/* $XFree86: xc/programs/Xserver/hw/xfree86/xf24_32bpp/cfb24_32.h,v 1.5 2000/04/01 00:17:19 mvojkovi Exp $ */

#ifndef _CFB24_32_H
#define _CFB24_32_H

#include "gcstruct.h"
#include "window.h"

typedef struct {
   GCOps		*Ops24bpp;
   GCOps 		*Ops32bpp;
   unsigned long	changes;	
   Bool			OpsAre24bpp;  
} cfb24_32GCRec, *cfb24_32GCPtr;


extern int cfb24_32GCIndex;
extern int cfb24_32PixmapIndex;

typedef struct {
   PixmapPtr		pix;
   Bool			freePrivate;
   Bool			isRefPix;
} cfb24_32PixmapRec, *cfb24_32PixmapPtr;

RegionPtr
cfb24_32CopyArea(
    DrawablePtr pSrcDraw,
    DrawablePtr pDstDraw,
    GC *pGC,
    int srcx, int srcy,
    int width, int height,
    int dstx, int dsty 
);

void 
cfbDoBitblt24To32(
    DrawablePtr pSrc, 
    DrawablePtr pDst, 
    int rop,
    RegionPtr prgnDst, 
    DDXPointPtr pptSrc,
    unsigned long planemask,
    unsigned long bitPlane
);

void 
cfbDoBitblt32To24(
    DrawablePtr pSrc, 
    DrawablePtr pDst, 
    int rop,
    RegionPtr prgnDst, 
    DDXPointPtr pptSrc,
    unsigned long planemask,
    unsigned long bitPlane
);

void 
cfb24_32DoBitblt24To24GXcopy(
    DrawablePtr pSrc, 
    DrawablePtr pDst, 
    int rop,
    RegionPtr prgnDst, 
    DDXPointPtr pptSrc,
    unsigned long pm,
    unsigned long bitPlane
);

void
cfb24_32ValidateGC24(
    GCPtr  		pGC,
    unsigned long 	changes,
    DrawablePtr		pDrawable
);

void
cfb24_32ValidateGC32(
    GCPtr  		pGC,
    unsigned long 	changes,
    DrawablePtr		pDrawable
);

Bool cfb24_32CreateGC(GCPtr pGC);

void
cfb24_32GetSpans(
   DrawablePtr pDraw,
   int wMax,
   DDXPointPtr ppt,
   int *pwidth,
   int nspans,
   char *pchardstStart
);

void
cfb24_32PutImage (
    DrawablePtr pDraw,
    GCPtr pGC,
    int depth, 
    int x, int y, int w, int h,
    int leftPad,
    int format,
    char *pImage
);

void
cfb24_32GetImage (
    DrawablePtr pDraw,
    int sx, int sy, int w, int h,
    unsigned int format,
    unsigned long planeMask,
    char *pdstLine
);

Bool
cfb24_32ScreenInit (
    ScreenPtr pScreen,
    pointer pbits,
    int xsize, int ysize,
    int dpix, int dpiy,	
    int width
);


Bool cfb24_32CreateWindow(WindowPtr pWin);
Bool cfb24_32DestroyWindow(WindowPtr pWin);

Bool
cfb24_32PositionWindow(
    WindowPtr pWin,
    int x, int y
);

void
cfb24_32CopyWindow(
    WindowPtr pWin,
    DDXPointRec ptOldOrg,
    RegionPtr prgnSrc
);

Bool
cfb24_32ChangeWindowAttributes(
    WindowPtr pWin,
    unsigned long mask
);

PixmapPtr
cfb24_32CreatePixmap (
    ScreenPtr	pScreen,
    int		width,
    int		height,
    int		depth
);

Bool cfb24_32DestroyPixmap(PixmapPtr pPixmap);

PixmapPtr cfb24_32RefreshPixmap(PixmapPtr pix);

#define CFB24_32_GET_GC_PRIVATE(pGC)\
   (cfb24_32GCPtr)((pGC)->devPrivates[cfb24_32GCIndex].ptr)

#define CFB24_32_GET_PIXMAP_PRIVATE(pPix) \
    (cfb24_32PixmapPtr)((pPix)->devPrivates[cfb24_32PixmapIndex].ptr)

#endif /* _CFB24_32_H */
