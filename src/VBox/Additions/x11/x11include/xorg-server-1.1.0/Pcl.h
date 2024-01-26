/* $Xorg: Pcl.h,v 1.3 2000/08/17 19:48:07 cpqbld Exp $ */
/*******************************************************************
**
**    *********************************************************
**    *
**    *  File:		Pcl.h
**    *
**    *  Contents:  defines and includes for the Pcl driver
**    *             for a printing X server.
**    *
**    *  Created:	1/30/95
**    *
**    *********************************************************
**
********************************************************************/
/*
(c) Copyright 1996 Hewlett-Packard Company
(c) Copyright 1996 International Business Machines Corp.
(c) Copyright 1996 Sun Microsystems, Inc.
(c) Copyright 1996 Novell, Inc.
(c) Copyright 1996 Digital Equipment Corp.
(c) Copyright 1996 Fujitsu Limited
(c) Copyright 1996 Hitachi, Ltd.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the copyright holders shall
not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from said
copyright holders.
*/
/* $XFree86: xc/programs/Xserver/Xprint/pcl/Pcl.h,v 1.12 2001/12/21 21:02:05 dawes Exp $ */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _PCL_H_
#define _PCL_H_

#include <stdio.h>
#include "scrnintstr.h"

#include "PclDef.h"
#include "Pclmap.h"
#include "PclSFonts.h"

#include <X11/extensions/Print.h>
#include <X11/extensions/Printstr.h>

#include "regionstr.h"
#include <X11/fonts/fontstruct.h>
#include "dixfontstr.h"
#include "gcstruct.h"

/*
 * Some sleazes to force the XrmDB stuff into the server
 */
#ifndef HAVE_XPointer
typedef char *XPointer;
#endif
#define Status int
#define True 1
#define False 0
#include "misc.h"
#include <X11/Xfuncproto.h>
#include <X11/Xresource.h>
#include "attributes.h"

/******
 * externally visible variables from PclInit.c
 ******/
extern int PclScreenPrivateIndex, PclWindowPrivateIndex;
extern int PclContextPrivateIndex;
extern int PclPixmapPrivateIndex;
extern int PclGCPrivateIndex;

/******
 * externally visible variables from PclAttVal.c
 ******/
extern XpValidatePoolsRec PclValidatePoolsRec;

/*
 * This structure defines a mapping from an X colormap ID to a list of
 * print contexts which use the colormap.
 */
typedef struct _pclcontextlist {
    XpContextPtr context;
    struct _pclcontextlist *next;
} PclContextList, *PclContextListPtr;

typedef struct _pclcmaptocontexts {
    long colormapId;
    PclContextListPtr contexts;
    struct _pclcmaptocontexts *next;
} PclCmapToContexts;

typedef struct {
    PclCmapToContexts *colormaps;
    CloseScreenProcPtr CloseScreen;
} PclScreenPrivRec, *PclScreenPrivPtr;

/*
 * This structure defines a mapping from an X colormap ID to a PCL
 * palette ID.
 */
typedef struct _palettemap {
    long colormapId;
    int paletteId;
    int downloaded;
    struct _palettemap *next;
} PclPaletteMap, *PclPaletteMapPtr;

typedef struct {
    char *jobFileName;
    FILE *pJobFile;
    char *pageFileName;
    FILE *pPageFile;
    GC lastGC;
    unsigned char *dash;
    int validGC;
    ClientPtr getDocClient;
    int getDocBufSize;
    PclSoftFontInfoPtr pSoftFontInfo;
    PclPaletteMapPtr palettes;
    int currentPalette;
    int nextPaletteId;
    PclPaletteMap staticGrayPalette;
    PclPaletteMap trueColorPalette;
    PclPaletteMap specialTrueColorPalette;
    unsigned char *ctbl;
    int ctbldim;
    int isRaw;
#ifdef XP_PCL_LJ3
    unsigned int fcount;
    unsigned int fcount_max;
    char *figures;
#endif /* XP_PCL_LJ3 */
} PclContextPrivRec, *PclContextPrivPtr;

typedef struct {
    int validContext;
    XpContextPtr context;
} PclWindowPrivRec, *PclWindowPrivPtr;

typedef struct {
    unsigned long stippleFg, stippleBg;
} PclGCPrivRec, *PclGCPrivPtr;

typedef struct {
    XpContextPtr context;
    char *tempFileName;
    FILE *tempFile;
    GC lastGC;
    int validGC;
} PclPixmapPrivRec, *PclPixmapPrivPtr;

/******
 * Defined functions
 ******/
#define SEND_PCL(f,c) fwrite( c, sizeof( char ), strlen( c ), f )
#define SEND_PCL_COUNT(f,c,n) fwrite( c, sizeof( char ), n, f )

#ifndef XP_PCL_LJ3
#define SAVE_PCL(f,p,c) SEND_PCL(f,c)
#define SAVE_PCL_COUNT(f,p,c,n) SEND_PCL_COUNT(f,c,n)
#define MACRO_START(f,p) SEND_PCL(f, "\033&f1Y\033&f0X")
#define MACRO_END(f) SEND_PCL(f, "\033&f1X")
#else
#define SAVE_PCL(f,p,c) PclSpoolFigs(p, c, strlen(c))
#define SAVE_PCL_COUNT(f,p,c,n) PclSpoolFigs(p, c, n)
#define MACRO_START(f,p) p->fcount = 0
#define MACRO_END(f)	/* do nothing */
#endif /* XP_PCL_LJ3 */

#define MIN(a,b) (((a)<(b))?(a):(b))
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

/******
 * Functions in PclArc.c
 ******/
extern void PclPolyArc(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int nArcs,
    xArc *pArcs);
extern void PclPolyFillArc(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int nArcs,
    xArc *pArcs);

/******
 * Functions in PclArea.c
 ******/
extern void PclPutImage(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int depth,
    int x,
    int y,
    int w,
    int h,
    int leftPad,
    int format,
    char *pImage);
extern RegionPtr PclCopyArea(
    DrawablePtr pSrc,
    DrawablePtr pDst,
    GCPtr pGC,
    int srcx,
    int srcy,
    int width,
    int height,
    int dstx,
    int dsty);
RegionPtr PclCopyPlane(
    DrawablePtr pSrc,
    DrawablePtr pDst,
    GCPtr pGC,
    int srcx,
    int srcy,
    int width,
    int height,
    int dstx,
    int dsty,
    unsigned long plane);


/******
 * Functions in PclAttr.c
 ******/
extern char *PclGetAttributes(
    XpContextPtr pCon,
    XPAttributes pool );
extern char *PclGetOneAttribute(
    XpContextPtr pCon,
    XPAttributes pool,
    char *attr );
extern int PclAugmentAttributes(
    XpContextPtr pCon,
    XPAttributes pool,
    char *attrs );
extern int PclSetAttributes(
    XpContextPtr pCon,
    XPAttributes pool,
    char *attrs );

/******
 * Functions in PclColor.c
 ******/
extern Bool PclCreateDefColormap(ScreenPtr pScreen);
extern Bool PclCreateColormap(ColormapPtr pColor);
extern void PclDestroyColormap(ColormapPtr pColor);
extern void PclInstallColormap(ColormapPtr pColor);
extern void PclUninstallColormap(ColormapPtr pColor);
extern int PclListInstalledColormaps(ScreenPtr pScreen,
				      XID *pCmapList);
extern void PclStoreColors(ColormapPtr pColor,
			   int ndef,
			   xColorItem *pdefs);
extern void PclResolveColor(unsigned short *pRed,
			    unsigned short *pGreen,
			    unsigned short *pBlue,
			    VisualPtr pVisual);
extern int PclUpdateColormap(DrawablePtr pDrawable,
			     XpContextPtr pCon,
			     GCPtr gc,
			     FILE *outFile);
extern void PclLookUp(ColormapPtr cmap,
		      PclContextPrivPtr cPriv,
		      unsigned short *r,
		      unsigned short *g,
		      unsigned short *b);
extern PclPaletteMapPtr PclFindPaletteMap(PclContextPrivPtr cPriv,
				   ColormapPtr cmap,
				   GCPtr gc);
extern unsigned char *PclReadMap(char *, int *);


/******
 * Functions in PclCursor.c
 ******/
extern void PclConstrainCursor(
    ScreenPtr pScreen,
    BoxPtr pBox);
extern void PclCursorLimits(
    ScreenPtr pScreen,
    CursorPtr pCursor,
    BoxPtr pHotBox,
    BoxPtr pTopLeftbox);
extern Bool PclDisplayCursor(
    ScreenPtr pScreen,
    CursorPtr pCursor);
extern Bool PclRealizeCursor(
    ScreenPtr pScreen,
    CursorPtr pCursor);
extern Bool PclUnrealizeCursor(
    ScreenPtr pScreen,
    CursorPtr pCursor);
extern void PclRecolorCursor(
    ScreenPtr pScreen,
    CursorPtr pCursor,
    Bool displayed);
extern Bool PclSetCursorPosition(
    ScreenPtr pScreen,
    int x,
    int y,
    Bool generateEvent);

/******
 * Functions in PclSFonts.c
 ******/
extern void
PclDownloadSoftFont8(
    FILE *fp,
    PclSoftFontInfoPtr pSoftFontInfo,
    PclFontHead8Ptr pfh,
    PclCharDataPtr pcd,
    unsigned char *code);
extern void PclDownloadSoftFont16(
    FILE *fp,
    PclSoftFontInfoPtr pSoftFontInfo,
    PclFontHead16Ptr pfh,
    PclCharDataPtr pcd,
    unsigned char row,
    unsigned char col);
extern PclSoftFontInfoPtr PclCreateSoftFontInfo(void);
extern void PclDestroySoftFontInfo(
    PclSoftFontInfoPtr pSoftFontInfo );

/******
 * Functions in PclGC.c
 ******/
extern Bool PclCreateGC(GCPtr pGC);
extern void PclDestroyGC(GCPtr pGC);
extern int PclUpdateDrawableGC(
    GCPtr pGC,
    DrawablePtr pDrawable,
    FILE **outFile);
extern void PclValidateGC(
    GCPtr pGC,
    unsigned long changes,
    DrawablePtr pDrawable);
extern void PclSetDrawablePrivateStuff(
    DrawablePtr pDrawable,
    GC gc );
extern int PclGetDrawablePrivateStuff(
    DrawablePtr pDrawable,
    GC *gc,
    unsigned long *valid,
    FILE **file );
extern void PclSetDrawablePrivateGC(
     DrawablePtr pDrawable,
     GC gc);
extern void PclComputeCompositeClip(
    GCPtr pGC,
    DrawablePtr pDrawable);

/******
 * Functions in PclInit.c
 ******/
extern Bool PclCloseScreen(
    int index,
    ScreenPtr pScreen);
extern Bool InitializeColorPclDriver(
    int ndx,
    ScreenPtr pScreen,
    int argc,
    char **argv);
extern Bool InitializeMonoPclDriver(
    int ndx,
    ScreenPtr pScreen,
    int argc,
    char **argv);
extern Bool InitializeLj3PclDriver(
    int ndx,
    ScreenPtr pScreen,
    int argc,
    char **argv);
extern XpContextPtr PclGetContextFromWindow( WindowPtr win );

/******
 * Functions in PclLine.c
 ******/
extern void PclPolyLine(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int mode,
    int nPoints,
    xPoint *pPoints);
extern void PclPolySegment(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int nSegments,
    xSegment *pSegments);

/******
 * Functions in PclMisc.c
 ******/
extern void PclQueryBestSize(
    int class,
    short *pwidth,
    short *pheight,
    ScreenPtr pScreen);
extern char *GetPropString(WindowPtr pWin, char *propName);
extern int SystemCmd(char *cmdStr);
extern int PclGetMediumDimensions(
    XpContextPtr pCon,
    CARD16 *pWidth,
    CARD16 *pHeight);
extern int PclGetReproducibleArea(
    XpContextPtr pCon,
    xRectangle *pRect);
extern void PclSendData(
    FILE *outFile,
    PclContextPrivPtr pConPriv,
    BoxPtr pbox,
    int nbox,
    double ratio);

/******
 * Functions in PclPixel.c
 ******/
extern void PclPolyPoint(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int mode,
    int nPoints,
    xPoint *pPoints);
extern void PclPushPixels(
    GCPtr pGC,
    PixmapPtr pBitmap,
    DrawablePtr pDrawable,
    int width,
    int height,
    int x,
    int y);

/******
 * Functions in PclPixmap.c
 ******/
extern PixmapPtr PclCreatePixmap(
    ScreenPtr pScreen,
    int width,
    int height,
    int depth);
extern Bool PclDestroyPixmap(PixmapPtr pPixmap);

/******
 * Functions in PclPolygon.c
 ******/
extern void PclPolyRectangle(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int nRects,
    xRectangle *pRects);
extern void PclFillPolygon(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int shape,
    int mode,
    int nPoints,
    DDXPointPtr pPoints);
extern void PclPolyFillRect(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int nRects,
    xRectangle *pRects);

/******
 * Functions in PclSpans.c
 ******/
extern void PclFillSpans(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int nSpans,
    DDXPointPtr pPoints,
    int *pWidths,
    int fSorted);
extern void PclSetSpans(
    DrawablePtr pDrawable,
    GCPtr pGC,
    char *pSrc,
    DDXPointPtr pPoints,
    int *pWidths,
    int nSpans,
    int fSorted);

/******
 * Functions in PclText.c
 ******/
extern int PclPolyText8(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int x,
    int y,
    int count,
    char *string);
extern int PclPolyText16(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int x,
    int y,
    int count,
    unsigned short *string);
extern void PclImageText8(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int x,
    int y,
    int count,
    char *string);
extern void PclImageText16(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int x,
    int y,
    int count,
    unsigned short *string);
extern void PclImageGlyphBlt(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int x,
    int y,
    unsigned int nGlyphs,
    CharInfoPtr *pCharInfo,
    pointer pGlyphBase);
extern void PclPolyGlyphBlt(
    DrawablePtr pDrawable,
    GCPtr pGC,
    int x,
    int y,
    unsigned int nGlyphs,
    CharInfoPtr *pCharInfo,
    pointer pGlyphBase);

/******
 * Functions in PclWindow.c
 ******/
extern Bool PclCreateWindow(register WindowPtr pWin);
extern Bool PclDestroyWindow(WindowPtr pWin);
extern Bool PclMapWindow(WindowPtr pWindow);
extern Bool PclPositionWindow(
    register WindowPtr pWin,
    int x,
    int y);
extern Bool PclUnmapWindow(WindowPtr pWindow);
extern void PclCopyWindow(
    WindowPtr pWin,
    DDXPointRec ptOldOrg,
    RegionPtr prgnSrc);
extern Bool PclChangeWindowAttributes(
    register WindowPtr pWin,
    register unsigned long mask);
extern void PclPaintWindow(
    WindowPtr   pWin,
    RegionPtr   pRegion,
    int         what);

/******
 * Functions in PclFonts.c
 ******/
extern Bool PclRealizeFont(
    ScreenPtr   pscr,
    FontPtr     pFont);
extern Bool PclUnrealizeFont(
    ScreenPtr   pscr,
    FontPtr     pFont);

/******
 * Functions in PclPrint.c
 ******/
extern int PclStartJob(
    XpContextPtr pCon,
    Bool sendClientData,
    ClientPtr client);
extern int PclEndJob(
    XpContextPtr pCon,
    Bool cancel);
extern int PclStartPage(
    XpContextPtr pCon,
    WindowPtr pWin);
extern int PclEndPage(
    XpContextPtr pCon,
    WindowPtr pWin);
extern int PclStartDoc(XpContextPtr pCon,
		       XPDocumentType type);
extern int PclEndDoc(
    XpContextPtr pCon,
    Bool cancel);
extern int PclDocumentData(
    XpContextPtr pCon,
    DrawablePtr pDraw,
    char *pData,
    int len_data,
    char *pFmt,
    int len_fmt,
    char *pOpt,
    int len_opt,
    ClientPtr client);
extern int PclGetDocumentData(
    XpContextPtr pCon,
    ClientPtr client,
    int maxBufferSize);


#endif  /* _PCL_H_ */
