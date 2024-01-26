/* $XFree86: xc/programs/Xserver/hw/xfree86/xf4bpp/xf4bpp.h,v 1.8 2003/02/18 21:29:59 tsi Exp $ */


#ifndef __XF4BPP_H__
#define __XF4BPP_H__


#include "windowstr.h"
#include "gcstruct.h"
#include "colormapst.h"
#include "fontstruct.h"
#ifndef PixelType
#define PixelType CARD32
#endif

/* ppcArea.c */
void xf4bppFillArea(
#if NeedFunctionPrototypes
    WindowPtr,
    int,
    BoxPtr,
    GCPtr
#endif
);

/* ppcBStore.c */
void xf4bppSaveAreas(
#if NeedFunctionPrototypes
    PixmapPtr,
    RegionPtr,
    int,
    int,
    WindowPtr
#endif
);
void xf4bppRestoreAreas(
#if NeedFunctionPrototypes
    PixmapPtr,
    RegionPtr,
    int,
    int,
    WindowPtr
#endif
);

/* ppcClip.c */
void xf4bppDestroyClip(
#if NeedFunctionPrototypes
    GCPtr
#endif
);
void xf4bppChangeClip(
#if NeedFunctionPrototypes
    GCPtr,
    int,
    pointer,
    int
#endif
);
void xf4bppCopyClip(
#if NeedFunctionPrototypes
    GCPtr,
    GCPtr
#endif
);

/* ppcCpArea.c */
RegionPtr xf4bppCopyArea(
#if NeedFunctionPrototypes
    DrawablePtr,
    DrawablePtr,
    GCPtr,
    int,
    int,
    int,
    int,
    int,
    int
#endif
);

/* ppcDepth.c */
Bool xf4bppDepthOK(
#if NeedFunctionPrototypes
    DrawablePtr,
    int
#endif
);

/* ppcFillRct.c */
void xf4bppPolyFillRect(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    xRectangle *
#endif
);

/* ppcWindowFS.c */
void xf4bppSolidWindowFS(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
#endif
);
void xf4bppStippleWindowFS(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
#endif
);
void xf4bppOpStippleWindowFS(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
#endif
);
void xf4bppTileWindowFS(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
#endif
);

/* xf4bppPixmapFS.c */
void xf4bppSolidPixmapFS(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
#endif
);
void xf4bppStipplePixmapFS(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
#endif
);
void xf4bppOpStipplePixmapFS(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
#endif
);
void xf4bppTilePixmapFS(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    DDXPointPtr,
    int *,
    int
#endif
);

/* ppcGC.c */
Bool xf4bppCreateGC(
#if NeedFunctionPrototypes
    GCPtr
#endif
);
void xf4bppDestroyGC(
#if NeedFunctionPrototypes
    GC *
#endif
);
void xf4bppValidateGC(
#if NeedFunctionPrototypes
    GCPtr,
    unsigned long,
    DrawablePtr
#endif
);

/* ppcGetSp.c */
void xf4bppGetSpans(
#if NeedFunctionPrototypes
    DrawablePtr,
    int,
    DDXPointPtr,
    int *,
    int,
    char *
#endif
);

/* ppcImg.c */
void xf4bppGetImage(
#if NeedFunctionPrototypes
    DrawablePtr,
    int,
    int,
    int,
    int,
    unsigned int,
    unsigned long,
    char *
#endif
);

/* ppcLine.c */
void xf4bppScrnZeroLine(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    int,
    DDXPointPtr
#endif
);
void xf4bppScrnZeroDash(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    int,
    DDXPointPtr
#endif
);
void xf4bppScrnZeroSegs(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    xSegment *
#endif
);

/* ppcPixmap.c */
PixmapPtr xf4bppCreatePixmap(
#if NeedFunctionPrototypes
    ScreenPtr,
    int,
    int,
    int
#endif
);
PixmapPtr xf4bppCopyPixmap(
#if NeedFunctionPrototypes
    PixmapPtr
#endif
);

/* ppcPntWin.c */
void xf4bppPaintWindow(
#if NeedFunctionPrototypes
    WindowPtr,
    RegionPtr,
    int
#endif
);

/* ppcPolyPnt.c */
void xf4bppPolyPoint(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    int,
    xPoint *
#endif
);

/* ppcPolyRec.c */
void xf4bppPolyRectangle(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    xRectangle *
#endif
);

/* ppcQuery.c */
void xf4bppQueryBestSize(
#if NeedFunctionPrototypes
    int,
    unsigned short *,
    unsigned short *,
    ScreenPtr
#endif
);

/* ppcRslvC.c */
void xf4bppResolveColor(
#if NeedFunctionPrototypes
    unsigned short *,
    unsigned short *,
    unsigned short *,
    VisualPtr
#endif
);
Bool xf4bppInitializeColormap(
#if NeedFunctionPrototypes
    ColormapPtr
#endif
);

/* ppcSetSp.c */
void xf4bppSetSpans(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    char *,
    DDXPointPtr,
    int *,
    int,
    int
#endif
);

/* ppcWindow.c */
void xf4bppCopyWindow(
#if NeedFunctionPrototypes
    WindowPtr,
    DDXPointRec,
    RegionPtr
#endif
);
Bool xf4bppPositionWindow(
#if NeedFunctionPrototypes
    WindowPtr,
    int,
    int
#endif
);
Bool xf4bppUnrealizeWindow(
#if NeedFunctionPrototypes
    WindowPtr,
    int,
    int
#endif
);
Bool xf4bppDestroyWindow(
#if NeedFunctionPrototypes
    WindowPtr
#endif
);
Bool xf4bppCreateWindowForXYhardware(
#if NeedFunctionPrototypes
    WindowPtr
#endif
);

/* emulOpStip.c */
void xf4bppOpaqueStipple(
#if NeedFunctionPrototypes
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
#endif
);

/* emulRepAre.c */
void xf4bppReplicateArea(
#if NeedFunctionPrototypes
    WindowPtr,
    int,
    int,
    int,
    int,
    int,
    int,
    int
#endif
);

/* emulTile.c */
void xf4bppTileRect(
#if NeedFunctionPrototypes
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
#endif
);

/* vgaGC.c */
Mask xf4bppChangeWindowGC(
#if NeedFunctionPrototypes
    GCPtr,
    Mask
#endif
);

/* vgaBitBlt.c */
void xf4bppBitBlt(
#if NeedFunctionPrototypes
    WindowPtr,
    int,
    int,
    int,
    int,
    int,
    int,
    int,
    int
#endif
);

/* vgaImages.c */
void xf4bppDrawColorImage(
#if NeedFunctionPrototypes
    WindowPtr,
    int,
    int,
    int,
    int,
    unsigned char *,
    int,
    const int,
    const unsigned long int
#endif
);
void xf4bppReadColorImage(
#if NeedFunctionPrototypes
    WindowPtr,
    int,
    int,
    int,
    int,
    unsigned char *,
    int
#endif
);

/* vgaLine.c */
void xf4bppHorzLine(
#if NeedFunctionPrototypes
    WindowPtr,
    unsigned long int,
    int,
    unsigned long int,
    int,
    int,
    int
#endif
);
void xf4bppVertLine(
#if NeedFunctionPrototypes
    WindowPtr,
    unsigned long int,
    int,
    unsigned long int,
    int,
    int,
    int
#endif
);
void xf4bppBresLine(
#if NeedFunctionPrototypes
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
#endif
);

/* vgaStipple.c */
void xf4bppFillStipple(
#if NeedFunctionPrototypes
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
#endif
);

/* vgaSolid.c */
void xf4bppFillSolid(
#if NeedFunctionPrototypes
    WindowPtr,
    unsigned long int,
    const int,
    unsigned long int,
    int,
    const int,
    int,
    const int
#endif
);

/* offscreen.c */
void xf4bppOffBitBlt(
#if NeedFunctionPrototypes
    WindowPtr,
    const int,
    const int,
    int,
    int,
    int,
    int,
    int,
    int
#endif
);
void xf4bppOffDrawColorImage(
#if NeedFunctionPrototypes
    WindowPtr,
    int,
    int,
    int,
    int,
    unsigned char *,
    int,
    const int,
    const unsigned long int
#endif
);
void xf4bppOffReadColorImage(
#if NeedFunctionPrototypes
    WindowPtr,
    int,
    int,
    int,
    int,
    unsigned char *,
    int
#endif
);
void xf4bppOffFillSolid(
#if NeedFunctionPrototypes
    WindowPtr,
    unsigned long int,
    const int,
    unsigned long int,
    int,
    const int,
    int,
    const int
#endif
);
void xf4bppOffDrawMonoImage(
#if NeedFunctionPrototypes
    WindowPtr,
    unsigned char *,
    int,
    int,
    int,
    int,
    unsigned long int,
    int,
    unsigned long int
#endif
);
void xf4bppOffFillStipple(
#if NeedFunctionPrototypes
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
#endif
);

/* mfbimggblt.c */
void xf4bppImageGlyphBlt(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    int,
    unsigned int,
    CharInfoPtr *,
    pointer
#endif
);

/* wm3.c */
int wm3_set_regs(
#if NeedFunctionPrototypes
    GC *
#endif
);

/* ppcIO.c */
int xf4bppNeverCalled(
#if NeedFunctionPrototypes
    void
#endif
);
Bool xf4bppScreenInit(
#if NeedFunctionPrototypes
    ScreenPtr,
    pointer,
    int,
    int,
    int,
    int,
    int
#endif
);

/* mfbfillarc.c */
void xf4bppPolyFillArc(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    xArc *
#endif
);

/* mfbzerarc.c */
void xf4bppZeroPolyArc(
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    xArc *
#endif
);

/* mfbline.c */
void xf4bppSegmentSS (
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    xSegment *
#endif
);
void xf4bppLineSS (
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    int,
    DDXPointPtr
#endif
);
void xf4bppSegmentSD (
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    xSegment *
#endif
);
void xf4bppLineSD (
#if NeedFunctionPrototypes
    DrawablePtr,
    GCPtr,
    int,
    int,
    DDXPointPtr
#endif
);

/* mfbbres.c */
void xf4bppBresS(
#if NeedFunctionPrototypes
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
#endif
);

/* mfbbresd.c */
void xf4bppBresD(
#if NeedFunctionPrototypes
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
#endif
);

/* mfbhrzvert.c */
void xf4bppHorzS(
#if NeedFunctionPrototypes
	PixelType *,
	int,
	int,
	int,
	int
#endif
);
void xf4bppVertS(
#if NeedFunctionPrototypes
	PixelType *,
	int,
	int,
	int,
	int
#endif
);

#ifdef PC98_EGC

/* egc_asm.s */
unsigned char getbits_x(
#if NeedFunctionPrototypes
	int,
	unsigned int,
	pointer,
	unsigned int
#endif
);
void wcopyr(
#if NeedFunctionPrototypes
	pointer,
	pointer,
	int,
	pointer
#endif
);
void wcopyl(
#if NeedFunctionPrototypes
	pointer,
	pointer,
	int,
	pointer
#endif
);
unsigned long int read8Z(
#if NeedFunctionPrototypes
	pointer
#endif
);

#endif /* PC98_EGC */

#endif /* __XF4BPP_H__ */
