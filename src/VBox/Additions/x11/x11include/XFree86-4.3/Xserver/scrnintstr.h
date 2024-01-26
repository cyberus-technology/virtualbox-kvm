/* $Xorg: scrnintstr.h,v 1.4 2001/02/09 02:05:15 xorgcvs Exp $ */
/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XFree86: xc/programs/Xserver/include/scrnintstr.h,v 1.10 2001/12/14 19:59:56 dawes Exp $ */

#ifndef SCREENINTSTRUCT_H
#define SCREENINTSTRUCT_H

#include "screenint.h"
#include "miscstruct.h"
#include "bstore.h"
#include "colormap.h"
#include "cursor.h"
#include "validate.h"
#include "X11/Xproto.h"
#include "dix.h"

typedef struct _PixmapFormat {
    unsigned char	depth;
    unsigned char	bitsPerPixel;
    unsigned char	scanlinePad;
    } PixmapFormatRec;
    
typedef struct _Visual {
    VisualID		vid;
    short		class;
    short		bitsPerRGBValue;
    short		ColormapEntries;
    short		nplanes;/* = log2 (ColormapEntries). This does not
				 * imply that the screen has this many planes.
				 * it may have more or fewer */
    unsigned long	redMask, greenMask, blueMask;
    int			offsetRed, offsetGreen, offsetBlue;
  } VisualRec;

typedef struct _Depth {
    unsigned char	depth;
    short		numVids;
    VisualID		*vids;    /* block of visual ids for this depth */
  } DepthRec;


/*
 *  There is a typedef for each screen function pointer so that code that
 *  needs to declare a screen function pointer (e.g. in a screen private
 *  or as a local variable) can easily do so and retain full type checking.
 */

typedef    Bool (* CloseScreenProcPtr)(
#if NeedNestedPrototypes
	int /*index*/,
	ScreenPtr /*pScreen*/
#endif
);

typedef    void (* QueryBestSizeProcPtr)(
#if NeedNestedPrototypes
	int /*class*/,
	unsigned short * /*pwidth*/,
	unsigned short * /*pheight*/,
	ScreenPtr /*pScreen*/
#endif
);

typedef    Bool (* SaveScreenProcPtr)(
#if NeedNestedPrototypes
	 ScreenPtr /*pScreen*/,
	 int /*on*/
#endif
);

typedef    void (* GetImageProcPtr)(
#if NeedNestedPrototypes
	DrawablePtr /*pDrawable*/,
	int /*sx*/,
	int /*sy*/,
	int /*w*/,
	int /*h*/,
	unsigned int /*format*/,
	unsigned long /*planeMask*/,
	char * /*pdstLine*/
#endif
);

typedef    void (* GetSpansProcPtr)(
#if NeedNestedPrototypes
	DrawablePtr /*pDrawable*/,
	int /*wMax*/,
	DDXPointPtr /*ppt*/,
	int* /*pwidth*/,
	int /*nspans*/,
	char * /*pdstStart*/
#endif
);

typedef    void (* PointerNonInterestBoxProcPtr)(
#if NeedNestedPrototypes
	ScreenPtr /*pScreen*/,
	BoxPtr /*pBox*/
#endif
);

typedef    void (* SourceValidateProcPtr)(
#if NeedNestedPrototypes
	DrawablePtr /*pDrawable*/,
	int /*x*/,
	int /*y*/,
	int /*width*/,
	int /*height*/
#endif
);

typedef    Bool (* CreateWindowProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/
#endif
);

typedef    Bool (* DestroyWindowProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/
#endif
);

typedef    Bool (* PositionWindowProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/,
	int /*x*/,
	int /*y*/
#endif
);

typedef    Bool (* ChangeWindowAttributesProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/,
	unsigned long /*mask*/
#endif
);

typedef    Bool (* RealizeWindowProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/
#endif
);

typedef    Bool (* UnrealizeWindowProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/
#endif
);

typedef    void (* RestackWindowProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/,
	WindowPtr /*pOldNextSib*/
#endif
);

typedef    int  (* ValidateTreeProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pParent*/,
	WindowPtr /*pChild*/,
	VTKind /*kind*/
#endif
);

typedef    void (* PostValidateTreeProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pParent*/,
	WindowPtr /*pChild*/,
	VTKind /*kind*/
#endif
);

typedef    void (* WindowExposuresProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/,
	RegionPtr /*prgn*/,
	RegionPtr /*other_exposed*/
#endif
);

typedef    void (* PaintWindowProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/,
	RegionPtr /*pRegion*/,
	int /*what*/
#endif
);

typedef PaintWindowProcPtr PaintWindowBackgroundProcPtr;
typedef PaintWindowProcPtr PaintWindowBorderProcPtr;

typedef    void (* CopyWindowProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/,
	DDXPointRec /*ptOldOrg*/,
	RegionPtr /*prgnSrc*/
#endif
);

typedef    void (* ClearToBackgroundProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/,
	int /*x*/,
	int /*y*/,
	int /*w*/,
	int /*h*/,
	Bool /*generateExposures*/
#endif
);

typedef    void (* ClipNotifyProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/,
	int /*dx*/,
	int /*dy*/
#endif
);

typedef    PixmapPtr (* CreatePixmapProcPtr)(
#if NeedNestedPrototypes
	ScreenPtr /*pScreen*/,
	int /*width*/,
	int /*height*/,
	int /*depth*/
#endif
);

typedef    Bool (* DestroyPixmapProcPtr)(
#if NeedNestedPrototypes
	PixmapPtr /*pPixmap*/
#endif
);

typedef    void (* SaveDoomedAreasProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/,
	RegionPtr /*prgnSave*/,
	int /*xorg*/,
	int /*yorg*/
#endif
);

typedef    RegionPtr (* RestoreAreasProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/,
	RegionPtr /*prgnRestore*/
#endif
);

typedef    void (* ExposeCopyProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	GCPtr /*pGC*/,
	RegionPtr /*prgnExposed*/,
	int /*srcx*/,
	int /*srcy*/,
	int /*dstx*/,
	int /*dsty*/,
	unsigned long /*plane*/
#endif
);

typedef    RegionPtr (* TranslateBackingStoreProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/,
	int /*windx*/,
	int /*windy*/,
	RegionPtr /*oldClip*/,
	int /*oldx*/,
	int /*oldy*/
#endif
);

typedef    RegionPtr (* ClearBackingStoreProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/,
	int /*x*/,
	int /*y*/,
	int /*w*/,
	int /*h*/,
	Bool /*generateExposures*/
#endif
);

typedef    void (* DrawGuaranteeProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWindow*/,
	GCPtr /*pGC*/,
	int /*guarantee*/
#endif
);
    
typedef    Bool (* RealizeFontProcPtr)(
#if NeedNestedPrototypes
	ScreenPtr /*pScreen*/,
	FontPtr /*pFont*/
#endif
);

typedef    Bool (* UnrealizeFontProcPtr)(
#if NeedNestedPrototypes
	ScreenPtr /*pScreen*/,
	FontPtr /*pFont*/
#endif
);

typedef    void (* ConstrainCursorProcPtr)(
#if NeedNestedPrototypes
	ScreenPtr /*pScreen*/,
	BoxPtr /*pBox*/
#endif
);

typedef    void (* CursorLimitsProcPtr)(
#if NeedNestedPrototypes
	ScreenPtr /*pScreen*/,
	CursorPtr /*pCursor*/,
	BoxPtr /*pHotBox*/,
	BoxPtr /*pTopLeftBox*/
#endif
);

typedef    Bool (* DisplayCursorProcPtr)(
#if NeedNestedPrototypes
	ScreenPtr /*pScreen*/,
	CursorPtr /*pCursor*/
#endif
);

typedef    Bool (* RealizeCursorProcPtr)(
#if NeedNestedPrototypes
	ScreenPtr /*pScreen*/,
	CursorPtr /*pCursor*/
#endif
);

typedef    Bool (* UnrealizeCursorProcPtr)(
#if NeedNestedPrototypes
	ScreenPtr /*pScreen*/,
	CursorPtr /*pCursor*/
#endif
);

typedef    void (* RecolorCursorProcPtr)(
#if NeedNestedPrototypes
	ScreenPtr /*pScreen*/,
	CursorPtr /*pCursor*/,
	Bool /*displayed*/
#endif
);

typedef    Bool (* SetCursorPositionProcPtr)(
#if NeedNestedPrototypes
	ScreenPtr /*pScreen*/,
	int /*x*/,
	int /*y*/,
	Bool /*generateEvent*/
#endif
);

typedef    Bool (* CreateGCProcPtr)(
#if NeedNestedPrototypes
	GCPtr /*pGC*/
#endif
);

typedef    Bool (* CreateColormapProcPtr)(
#if NeedNestedPrototypes
	ColormapPtr /*pColormap*/
#endif
);

typedef    void (* DestroyColormapProcPtr)(
#if NeedNestedPrototypes
	ColormapPtr /*pColormap*/
#endif
);

typedef    void (* InstallColormapProcPtr)(
#if NeedNestedPrototypes
	ColormapPtr /*pColormap*/
#endif
);

typedef    void (* UninstallColormapProcPtr)(
#if NeedNestedPrototypes
	ColormapPtr /*pColormap*/
#endif
);

typedef    int (* ListInstalledColormapsProcPtr) (
#if NeedNestedPrototypes
	ScreenPtr /*pScreen*/,
	XID* /*pmaps */
#endif
);

typedef    void (* StoreColorsProcPtr)(
#if NeedNestedPrototypes
	ColormapPtr /*pColormap*/,
	int /*ndef*/,
	xColorItem * /*pdef*/
#endif
);

typedef    void (* ResolveColorProcPtr)(
#if NeedNestedPrototypes
	unsigned short* /*pred*/,
	unsigned short* /*pgreen*/,
	unsigned short* /*pblue*/,
	VisualPtr /*pVisual*/
#endif
);

#ifdef NEED_SCREEN_REGIONS

typedef    RegionPtr (* RegionCreateProcPtr)(
#if NeedNestedPrototypes
	BoxPtr /*rect*/,
	int /*size*/
#endif
);

typedef    void (* RegionInitProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*pReg*/,
	BoxPtr /*rect*/,
	int /*size*/
#endif
);

typedef    Bool (* RegionCopyProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*dst*/,
	RegionPtr /*src*/
#endif
);

typedef    void (* RegionDestroyProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*pReg*/
#endif
);

typedef    void (* RegionUninitProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*pReg*/
#endif
);

typedef    Bool (* IntersectProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*newReg*/,
	RegionPtr /*reg1*/,
	RegionPtr /*reg2*/
#endif
);

typedef    Bool (* UnionProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*newReg*/,
	RegionPtr /*reg1*/,
	RegionPtr /*reg2*/
#endif
);

typedef    Bool (* SubtractProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*regD*/,
	RegionPtr /*regM*/,
	RegionPtr /*regS*/
#endif
);

typedef    Bool (* InverseProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*newReg*/,
	RegionPtr /*reg1*/,
	BoxPtr /*invRect*/
#endif
);

typedef    void (* RegionResetProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*pReg*/,
	BoxPtr /*pBox*/
#endif
);

typedef    void (* TranslateRegionProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*pReg*/,
	int /*x*/,
	int /*y*/
#endif
);

typedef    int (* RectInProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*region*/,
	BoxPtr /*prect*/
#endif
);

typedef    Bool (* PointInRegionProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*pReg*/,
	int /*x*/,
	int /*y*/,
	BoxPtr /*box*/
#endif
);

typedef    Bool (* RegionNotEmptyProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*pReg*/
#endif
);

typedef    Bool (* RegionBrokenProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*pReg*/
#endif
);

typedef    Bool (* RegionBreakProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*pReg*/
#endif
);

typedef    void (* RegionEmptyProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*pReg*/
#endif
);

typedef    BoxPtr (* RegionExtentsProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*pReg*/
#endif
);

typedef    Bool (* RegionAppendProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*dstrgn*/,
	RegionPtr /*rgn*/
#endif
);

typedef    Bool (* RegionValidateProcPtr)(
#if NeedNestedPrototypes
	RegionPtr /*badreg*/,
	Bool* /*pOverlap*/
#endif
);

#endif /* NEED_SCREEN_REGIONS */

typedef    RegionPtr (* BitmapToRegionProcPtr)(
#if NeedNestedPrototypes
	PixmapPtr /*pPix*/
#endif
);

#ifdef NEED_SCREEN_REGIONS

typedef    RegionPtr (* RectsToRegionProcPtr)(
#if NeedNestedPrototypes
	int /*nrects*/,
	xRectangle* /*prect*/,
	int /*ctype*/
#endif
);

#endif /* NEED_SCREEN_REGIONS */

typedef    void (* SendGraphicsExposeProcPtr)(
#if NeedNestedPrototypes
	ClientPtr /*client*/,
	RegionPtr /*pRgn*/,
	XID /*drawable*/,
	int /*major*/,
	int /*minor*/
#endif
);

typedef    void (* ScreenBlockHandlerProcPtr)(
#if NeedNestedPrototypes
	int /*screenNum*/,
	pointer /*blockData*/,
	pointer /*pTimeout*/,
	pointer /*pReadmask*/
#endif
);

typedef    void (* ScreenWakeupHandlerProcPtr)(
#if NeedNestedPrototypes
	 int /*screenNum*/,
	 pointer /*wakeupData*/,
	 unsigned long /*result*/,
	 pointer /*pReadMask*/
#endif
);

typedef    Bool (* CreateScreenResourcesProcPtr)(
#if NeedNestedPrototypes
	ScreenPtr /*pScreen*/
#endif
);

typedef    Bool (* ModifyPixmapHeaderProcPtr)(
#if NeedNestedPrototypes
	PixmapPtr /*pPixmap*/,
	int /*width*/,
	int /*height*/,
	int /*depth*/,
	int /*bitsPerPixel*/,
	int /*devKind*/,
	pointer /*pPixData*/
#endif
);

typedef    PixmapPtr (* GetWindowPixmapProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWin*/
#endif
);

typedef    void (* SetWindowPixmapProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWin*/,
	PixmapPtr /*pPix*/
#endif
);

typedef    PixmapPtr (* GetScreenPixmapProcPtr)(
#if NeedNestedPrototypes
	ScreenPtr /*pScreen*/
#endif
);

typedef    void (* SetScreenPixmapProcPtr)(
#if NeedNestedPrototypes
	PixmapPtr /*pPix*/
#endif
);

typedef    void (* MarkWindowProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWin*/
#endif
);

typedef    Bool (* MarkOverlappedWindowsProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*parent*/,
	WindowPtr /*firstChild*/,
	WindowPtr * /*pLayerWin*/
#endif
);

typedef    Bool (* ChangeSaveUnderProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pLayerWin*/,
	WindowPtr /*firstChild*/
#endif
);

typedef    void (* PostChangeSaveUnderProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pLayerWin*/,
	WindowPtr /*firstChild*/
#endif
);

typedef    void (* MoveWindowProcPtr)(
#if NeedNestedPrototypes
	WindowPtr /*pWin*/,
	int /*x*/,
	int /*y*/,
	WindowPtr /*pSib*/,
	VTKind /*kind*/
#endif
);

typedef    void (* ResizeWindowProcPtr)(
#if NeedNestedPrototypes
    WindowPtr /*pWin*/,
    int /*x*/,
    int /*y*/, 
    unsigned int /*w*/,
    unsigned int /*h*/,
    WindowPtr /*pSib*/
#endif
);

typedef    WindowPtr (* GetLayerWindowProcPtr)(
#if NeedNestedPrototypes
    WindowPtr /*pWin*/
#endif
);

typedef    void (* HandleExposuresProcPtr)(
#if NeedNestedPrototypes
    WindowPtr /*pWin*/
#endif
);

typedef    void (* ReparentWindowProcPtr)(
#if NeedNestedPrototypes
    WindowPtr /*pWin*/,
    WindowPtr /*pPriorParent*/
#endif
);

#ifdef SHAPE
typedef    void (* SetShapeProcPtr)(
#if NeedFunctionPrototypes
	WindowPtr /*pWin*/
#endif
);
#endif /* SHAPE */

typedef    void (* ChangeBorderWidthProcPtr)(
#if NeedFunctionPrototypes
	WindowPtr /*pWin*/,
	unsigned int /*width*/
#endif
);

typedef    void (* MarkUnrealizedWindowProcPtr)(
#if NeedFunctionPrototypes
	WindowPtr /*pChild*/,
	WindowPtr /*pWin*/,
	Bool /*fromConfigure*/
#endif
);

typedef struct _Screen {
    int			myNum;	/* index of this instance in Screens[] */
    ATOM		id;
    short		width, height;
    short		mmWidth, mmHeight;
    short		numDepths;
    unsigned char      	rootDepth;
    DepthPtr       	allowedDepths;
    unsigned long      	rootVisual;
    unsigned long	defColormap;
    short		minInstalledCmaps, maxInstalledCmaps;
    char                backingStoreSupport, saveUnderSupport;
    unsigned long	whitePixel, blackPixel;
    unsigned long	rgf;	/* array of flags; she's -- HUNGARIAN */
    GCPtr		GCperDepth[MAXFORMATS+1];
			/* next field is a stipple to use as default in
			   a GC.  we don't build default tiles of all depths
			   because they are likely to be of a color
			   different from the default fg pixel, so
			   we don't win anything by building
			   a standard one.
			*/
    PixmapPtr		PixmapPerDepth[1];
    pointer		devPrivate;
    short       	numVisuals;
    VisualPtr		visuals;
    int			WindowPrivateLen;
    unsigned		*WindowPrivateSizes;
    unsigned		totalWindowSize;
    int			GCPrivateLen;
    unsigned		*GCPrivateSizes;
    unsigned		totalGCSize;

    /* Random screen procedures */

    CloseScreenProcPtr		CloseScreen;
    QueryBestSizeProcPtr	QueryBestSize;
    SaveScreenProcPtr		SaveScreen;
    GetImageProcPtr		GetImage;
    GetSpansProcPtr		GetSpans;
    PointerNonInterestBoxProcPtr PointerNonInterestBox;
    SourceValidateProcPtr	SourceValidate;

    /* Window Procedures */

    CreateWindowProcPtr		CreateWindow;
    DestroyWindowProcPtr	DestroyWindow;
    PositionWindowProcPtr	PositionWindow;
    ChangeWindowAttributesProcPtr ChangeWindowAttributes;
    RealizeWindowProcPtr	RealizeWindow;
    UnrealizeWindowProcPtr	UnrealizeWindow;
    ValidateTreeProcPtr		ValidateTree;
    PostValidateTreeProcPtr	PostValidateTree;
    WindowExposuresProcPtr	WindowExposures;
    PaintWindowBackgroundProcPtr PaintWindowBackground;
    PaintWindowBorderProcPtr	PaintWindowBorder;
    CopyWindowProcPtr		CopyWindow;
    ClearToBackgroundProcPtr	ClearToBackground;
    ClipNotifyProcPtr		ClipNotify;
    RestackWindowProcPtr	RestackWindow;

    /* Pixmap procedures */

    CreatePixmapProcPtr		CreatePixmap;
    DestroyPixmapProcPtr	DestroyPixmap;

    /* Backing store procedures */

    SaveDoomedAreasProcPtr	SaveDoomedAreas;
    RestoreAreasProcPtr		RestoreAreas;
    ExposeCopyProcPtr		ExposeCopy;
    TranslateBackingStoreProcPtr TranslateBackingStore;
    ClearBackingStoreProcPtr	ClearBackingStore;
    DrawGuaranteeProcPtr	DrawGuarantee;
    /*
     * A read/write copy of the lower level backing store vector is needed now
     * that the functions can be wrapped.
     */
    BSFuncRec			BackingStoreFuncs;
    
    /* Font procedures */

    RealizeFontProcPtr		RealizeFont;
    UnrealizeFontProcPtr	UnrealizeFont;

    /* Cursor Procedures */

    ConstrainCursorProcPtr	ConstrainCursor;
    CursorLimitsProcPtr		CursorLimits;
    DisplayCursorProcPtr	DisplayCursor;
    RealizeCursorProcPtr	RealizeCursor;
    UnrealizeCursorProcPtr	UnrealizeCursor;
    RecolorCursorProcPtr	RecolorCursor;
    SetCursorPositionProcPtr	SetCursorPosition;

    /* GC procedures */

    CreateGCProcPtr		CreateGC;

    /* Colormap procedures */

    CreateColormapProcPtr	CreateColormap;
    DestroyColormapProcPtr	DestroyColormap;
    InstallColormapProcPtr	InstallColormap;
    UninstallColormapProcPtr	UninstallColormap;
    ListInstalledColormapsProcPtr ListInstalledColormaps;
    StoreColorsProcPtr		StoreColors;
    ResolveColorProcPtr		ResolveColor;

    /* Region procedures */

#ifdef NEED_SCREEN_REGIONS
    RegionCreateProcPtr		RegionCreate;
    RegionInitProcPtr		RegionInit;
    RegionCopyProcPtr		RegionCopy;
    RegionDestroyProcPtr	RegionDestroy;
    RegionUninitProcPtr		RegionUninit;
    IntersectProcPtr		Intersect;
    UnionProcPtr		Union;
    SubtractProcPtr		Subtract;
    InverseProcPtr		Inverse;
    RegionResetProcPtr		RegionReset;
    TranslateRegionProcPtr	TranslateRegion;
    RectInProcPtr		RectIn;
    PointInRegionProcPtr	PointInRegion;
    RegionNotEmptyProcPtr	RegionNotEmpty;
    RegionBrokenProcPtr		RegionBroken;
    RegionBreakProcPtr		RegionBreak;
    RegionEmptyProcPtr		RegionEmpty;
    RegionExtentsProcPtr	RegionExtents;
    RegionAppendProcPtr		RegionAppend;
    RegionValidateProcPtr	RegionValidate;
#endif /* NEED_SCREEN_REGIONS */
    BitmapToRegionProcPtr	BitmapToRegion;
#ifdef NEED_SCREEN_REGIONS
    RectsToRegionProcPtr	RectsToRegion;
#endif /* NEED_SCREEN_REGIONS */
    SendGraphicsExposeProcPtr	SendGraphicsExpose;

    /* os layer procedures */

    ScreenBlockHandlerProcPtr	BlockHandler;
    ScreenWakeupHandlerProcPtr	WakeupHandler;

    pointer blockData;
    pointer wakeupData;

    /* anybody can get a piece of this array */
    DevUnion	*devPrivates;

    CreateScreenResourcesProcPtr CreateScreenResources;
    ModifyPixmapHeaderProcPtr	ModifyPixmapHeader;

    GetWindowPixmapProcPtr	GetWindowPixmap;
    SetWindowPixmapProcPtr	SetWindowPixmap;
    GetScreenPixmapProcPtr	GetScreenPixmap;
    SetScreenPixmapProcPtr	SetScreenPixmap;

    PixmapPtr pScratchPixmap;		/* scratch pixmap "pool" */

#ifdef PIXPRIV
    int			PixmapPrivateLen;
    unsigned int		*PixmapPrivateSizes;
    unsigned int		totalPixmapSize;
#endif

    MarkWindowProcPtr		MarkWindow;
    MarkOverlappedWindowsProcPtr MarkOverlappedWindows;
    ChangeSaveUnderProcPtr	ChangeSaveUnder;
    PostChangeSaveUnderProcPtr	PostChangeSaveUnder;
    MoveWindowProcPtr		MoveWindow;
    ResizeWindowProcPtr		ResizeWindow;
    GetLayerWindowProcPtr	GetLayerWindow;
    HandleExposuresProcPtr	HandleExposures;
    ReparentWindowProcPtr	ReparentWindow;

#ifdef SHAPE
    SetShapeProcPtr		SetShape;
#endif /* SHAPE */

    ChangeBorderWidthProcPtr	ChangeBorderWidth;
    MarkUnrealizedWindowProcPtr	MarkUnrealizedWindow;

} ScreenRec;

typedef struct _ScreenInfo {
    int		imageByteOrder;
    int		bitmapScanlineUnit;
    int		bitmapScanlinePad;
    int		bitmapBitOrder;
    int		numPixmapFormats;
    PixmapFormatRec
		formats[MAXFORMATS];
    int		arraySize;
    int		numScreens;
    ScreenPtr	screens[MAXSCREENS];
    int		numVideoScreens;
} ScreenInfo;

extern ScreenInfo screenInfo;

extern void InitOutput(
#if NeedFunctionPrototypes
    ScreenInfo 	* /*pScreenInfo*/,
    int     	/*argc*/,
    char    	** /*argv*/
#endif
);

#endif /* SCREENINTSTRUCT_H */
