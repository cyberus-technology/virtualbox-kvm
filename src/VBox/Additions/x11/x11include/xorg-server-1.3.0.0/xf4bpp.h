
#ifndef __XF4BPP_H__
#define __XF4BPP_H__


#include "windowstr.h"
#include "gcstruct.h"
#include "colormapst.h"
#include <X11/fonts/fontstruct.h>
#ifndef PixelType
#define PixelType CARD32
#endif

/* ppcArea.c */
void xf4bppFillArea(
    WindowPtr,
    int,
    BoxPtr,
    GCPtr
);

/* ppcBStore.c */
void xf4bppSaveAreas(
    PixmapPtr,
    RegionPtr,
    int,
    int,
    WindowPtr
);
void xf4bppRestoreAreas(
    PixmapPtr,
    RegionPtr,
    int,
    int,
    WindowPtr
);

/* ppcClip.c */
void xf4bppDestroyClip(
    GCPtr
);
void xf4bppChangeClip(
    GCPtr,
    int,
    pointer,
    int
);
void xf4bppCopyClip(
    GCPtr,
    GCPtr
);

/* ppcCpArea.c */
RegionPtr xf4bppCopyArea(
    DrawablePtr,
    DrawablePtr,
    GCPtr,
    int,
    int,
    int,
    int,
    int,
    int
);

/* ppcDepth.c */
Bool xf4bppDepthOK(
    DrawablePtr,
    int
);

/* ppcFillRct.c */
void xf4bppPolyFillRect(
    DrawablePtr,
    GCPtr,
    int,
    xRectangle *
);

/* ppcWindowFS.c */
void xf4bppSolidWindowFS(
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
);
void xf4bppStippleWindowFS(
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
);
void xf4bppOpStippleWindowFS(
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
);
void xf4bppTileWindowFS(
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
);

/* xf4bppPixmapFS.c */
void xf4bppSolidPixmapFS(
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
);
void xf4bppStipplePixmapFS(
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
);
void xf4bppOpStipplePixmapFS(
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
);
void xf4bppTilePixmapFS(
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
);

/* ppcGC.c */
Bool xf4bppCreateGC(
    GCPtr
);
void xf4bppDestroyGC(
    GC *
);
void xf4bppValidateGC(
    GCPtr,
    unsigned long,
    DrawablePtr
);

/* ppcGetSp.c */
void xf4bppGetSpans(
    DrawablePtr,
    int,
    DDXPointPtr,
    int *,
    int,
    char *
);

/* ppcImg.c */
void xf4bppGetImage(
    DrawablePtr,
    int,
    int,
    int,
    int,
    unsigned int,
    unsigned long,
    char *
);

/* ppcLine.c */
void xf4bppScrnZeroLine(
    DrawablePtr,
    GCPtr,
    int,
    int,
    DDXPointPtr
);
void xf4bppScrnZeroDash(
    DrawablePtr,
    GCPtr,
    int,
    int,
    DDXPointPtr
);
void xf4bppScrnZeroSegs(
    DrawablePtr,
    GCPtr,
    int,
    xSegment *
);

/* ppcPixmap.c */
PixmapPtr xf4bppCreatePixmap(
    ScreenPtr,
    int,
    int,
    int
);
PixmapPtr xf4bppCopyPixmap(
    PixmapPtr
);

/* ppcPntWin.c */
void xf4bppPaintWindow(
    WindowPtr,
    RegionPtr,
    int
);

/* ppcPolyPnt.c */
void xf4bppPolyPoint(
    DrawablePtr,
    GCPtr,
    int,
    int,
    xPoint *
);

/* ppcPolyRec.c */
void xf4bppPolyRectangle(
    DrawablePtr,
    GCPtr,
    int,
    xRectangle *
);

/* ppcQuery.c */
void xf4bppQueryBestSize(
    int,
    unsigned short *,
    unsigned short *,
    ScreenPtr
);

/* ppcRslvC.c */
void xf4bppResolveColor(
    unsigned short *,
    unsigned short *,
    unsigned short *,
    VisualPtr
);
Bool xf4bppInitializeColormap(
    ColormapPtr
);

/* ppcSetSp.c */
void xf4bppSetSpans(
    DrawablePtr,
    GCPtr,
    char *,
    DDXPointPtr,
    int *,
    int,
    int
);

/* ppcWindow.c */
void xf4bppCopyWindow(
    WindowPtr,
    DDXPointRec,
    RegionPtr
);
Bool xf4bppPositionWindow(
    WindowPtr,
    int,
    int
);
Bool xf4bppUnrealizeWindow(
    WindowPtr,
    int,
    int
);
Bool xf4bppDestroyWindow(
    WindowPtr
);
Bool xf4bppCreateWindowForXYhardware(
    WindowPtr
);

/* emulOpStip.c */
void xf4bppOpaqueStipple(
    WindowPtr,
    PixmapPtr,
    unsigned long int,
    unsigned long int,
    int,
    unsigned long int,
    int,
    int,
    int,
    int,
    int,
    int
);

/* emulRepAre.c */
void xf4bppReplicateArea(
    WindowPtr,
    int,
    int,
    int,
    int,
    int,
    int,
    int
);

/* emulTile.c */
void xf4bppTileRect(
    WindowPtr,
    PixmapPtr,
    const int,
    const unsigned long int,
    int,
    int,
    int,
    int,
    int,
    int
);

/* vgaGC.c */
Mask xf4bppChangeWindowGC(
    GCPtr,
    Mask
);

/* vgaBitBlt.c */
void xf4bppBitBlt(
    WindowPtr,
    int,
    int,
    int,
    int,
    int,
    int,
    int,
    int
);

/* vgaImages.c */
void xf4bppDrawColorImage(
    WindowPtr,
    int,
    int,
    int,
    int,
    unsigned char *,
    int,
    const int,
    const unsigned long int
);
void xf4bppReadColorImage(
    WindowPtr,
    int,
    int,
    int,
    int,
    unsigned char *,
    int
);

/* vgaLine.c */
void xf4bppHorzLine(
    WindowPtr,
    unsigned long int,
    int,
    unsigned long int,
    int,
    int,
    int
);
void xf4bppVertLine(
    WindowPtr,
    unsigned long int,
    int,
    unsigned long int,
    int,
    int,
    int
);
void xf4bppBresLine(
    WindowPtr,
    unsigned long int,
    int,
    unsigned long int,
    int,
    int,
    int,
    int,
    int,
    int,
    int,
    int,
    unsigned long int
);

/* vgaStipple.c */
void xf4bppFillStipple(
    WindowPtr,
    const PixmapPtr,
    unsigned long int,
    const int,
    unsigned long int,
    int,
    int,
    int,
    int,
    const int,
    const int
);

/* vgaSolid.c */
void xf4bppFillSolid(
    WindowPtr,
    unsigned long int,
    const int,
    unsigned long int,
    int,
    const int,
    int,
    const int
);

/* offscreen.c */
void xf4bppOffBitBlt(
    WindowPtr,
    const int,
    const int,
    int,
    int,
    int,
    int,
    int,
    int
);
void xf4bppOffDrawColorImage(
    WindowPtr,
    int,
    int,
    int,
    int,
    unsigned char *,
    int,
    const int,
    const unsigned long int
);
void xf4bppOffReadColorImage(
    WindowPtr,
    int,
    int,
    int,
    int,
    unsigned char *,
    int
);
void xf4bppOffFillSolid(
    WindowPtr,
    unsigned long int,
    const int,
    unsigned long int,
    int,
    const int,
    int,
    const int
);
void xf4bppOffDrawMonoImage(
    WindowPtr,
    unsigned char *,
    int,
    int,
    int,
    int,
    unsigned long int,
    int,
    unsigned long int
);
void xf4bppOffFillStipple(
    WindowPtr,
    const PixmapPtr,
    unsigned long int,
    const int,
    unsigned long int,
    int,
    int,
    int,
    int,
    const int,
    const int
);

/* mfbimggblt.c */
void xf4bppImageGlyphBlt(
    DrawablePtr,
    GCPtr,
    int,
    int,
    unsigned int,
    CharInfoPtr *,
    pointer
);

/* wm3.c */
int wm3_set_regs(
    GC *
);

/* ppcIO.c */
void xf4bppNeverCalled(
    void
);
Bool xf4bppScreenInit(
    ScreenPtr,
    pointer,
    int,
    int,
    int,
    int,
    int
);

/* mfbfillarc.c */
void xf4bppPolyFillArc(
    DrawablePtr,
    GCPtr,
    int,
    xArc *
);

/* mfbzerarc.c */
void xf4bppZeroPolyArc(
    DrawablePtr,
    GCPtr,
    int,
    xArc *
);

/* mfbline.c */
void xf4bppSegmentSS (
    DrawablePtr,
    GCPtr,
    int,
    xSegment *
);
void xf4bppLineSS (
    DrawablePtr,
    GCPtr,
    int,
    int,
    DDXPointPtr
);
void xf4bppSegmentSD (
    DrawablePtr,
    GCPtr,
    int,
    xSegment *
);
void xf4bppLineSD (
    DrawablePtr,
    GCPtr,
    int,
    int,
    DDXPointPtr
);

/* mfbbres.c */
void xf4bppBresS(
	PixelType *,
	int,
	int,
	int,
	int,
	int,
	int,
	int,
	int,
	int,
	int
);

/* mfbbresd.c */
void xf4bppBresD(
	DrawablePtr,
	int, int,
	int *,
	unsigned char *,
	int,
	int *,
	int,
	PixelType *,
	int, int, int, int, int, int,
	int, int,
	int, int
);

/* mfbhrzvert.c */
void xf4bppHorzS(
	PixelType *,
	int,
	int,
	int,
	int
);
void xf4bppVertS(
	PixelType *,
	int,
	int,
	int,
	int
);

#ifdef PC98_EGC

/* egc_asm.s */
unsigned char getbits_x(
	int,
	unsigned int,
	pointer,
	unsigned int
);
void wcopyr(
	pointer,
	pointer,
	int,
	pointer
);
void wcopyl(
	pointer,
	pointer,
	int,
	pointer
);
unsigned long int read8Z(
	pointer
);

#endif /* PC98_EGC */

#endif /* __XF4BPP_H__ */
