/* $Xorg: Pclmap.h,v 1.3 2000/08/17 19:48:08 cpqbld Exp $ */
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
/* $XFree86: xc/programs/Xserver/Xprint/pcl/Pclmap.h,v 1.5 2001/07/25 15:05:00 dawes Exp $ */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _PCLMAP_H_
#define _PCLMAP_H_

#ifdef XP_PCL_COLOR
#ifdef CATNAME
#undef CATNAME
#endif
#if !defined(UNIXCPP) || defined(ANSICPP)
#define PCLNAME(subname) PclCr##subname
#define CATNAME(prefix,subname) prefix##Color##subname
#else
#define PCLNAME(subname) PclCr/**/subname
#define CATNAME(prefix,subname) prefix/**/Color/**/subname
#endif
#endif /* XP_PCL_COLOR */

#ifdef XP_PCL_MONO
#ifdef CATNAME
#undef CATNAME
#endif
#if !defined(UNIXCPP) || defined(ANSICPP)
#define PCLNAME(subname) PclMn##subname
#define CATNAME(prefix,subname) prefix##Mono##subname
#else
#define PCLNAME(subname) PclMn/**/subname
#define CATNAME(prefix,subname) prefix/**/Mono/**/subname
#endif
#endif /* XP_PCL_MONO */

#ifdef XP_PCL_LJ3
#ifdef CATNAME
#undef CATNAME
#endif
#if !defined(UNIXCPP) || defined(ANSICPP)
#define PCLNAME(subname) PclLj3##subname
#define CATNAME(prefix,subname) prefix##Lj3##subname
#else
#define PCLNAME(subname) PclLj3/**/subname
#define CATNAME(prefix,subname) prefix/**/Lj3/**/subname
#endif
#endif /* XP_PCL_LJ3 */

#ifdef PCLNAME

/* PclInit.c */
#define InitializePclDriver		CATNAME(Initialize, PclDriver)
#define PclCloseScreen			PCLNAME(CloseScreen)
#define PclGetContextFromWindow		PCLNAME(GetContextFromWindow)
#define PclScreenPrivateIndex	PCLNAME(ScreenPrivateIndex)
#define PclWindowPrivateIndex	PCLNAME(WindowPrivateIndex)
#define PclContextPrivateIndex	PCLNAME(ContextPrivateIndex)
#define PclPixmapPrivateIndex	PCLNAME(PixmapPrivateIndex)
#define PclGCPrivateIndex	PCLNAME(GCPrivateIndex)

/* PclPrint.c */
#define PclStartJob			PCLNAME(StartJob)
#define PclEndJob			PCLNAME(EndJob)
#define PclStartPage			PCLNAME(StartPage)
#define PclEndPage			PCLNAME(EndPage)
#define PclStartDoc			PCLNAME(StartDoc)
#define PclEndDoc			PCLNAME(EndDoc)
#define PclDocumentData			PCLNAME(DocumentData)
#define PclGetDocumentData		PCLNAME(GetDocumentData)

/* PclWindow.c */
#define PclCreateWindow			PCLNAME(CreateWindow)
#define PclMapWindow			PCLNAME(MapWindow)
#define PclPositionWindow		PCLNAME(PositionWindow)
#define PclUnmapWindow			PCLNAME(UnmapWindow)
#define PclCopyWindow			PCLNAME(CopyWindow)
#define PclChangeWindowAttributes	PCLNAME(ChangeWindowAttributes)
#define PclPaintWindow			PCLNAME(PaintWindow)
#define PclDestroyWindow		PCLNAME(DestroyWindow)

/* PclGC.c */
#define PclCreateGC			PCLNAME(CreateGC)
#define PclDestroyGC			PCLNAME(DestroyGC)
#define PclGetDrawablePrivateStuff	PCLNAME(GetDrawablePrivateStuff)
#define PclSetDrawablePrivateGC		PCLNAME(SetDrawablePrivateGC)
#define PclSendPattern			PCLNAME(SendPattern)
#define PclUpdateDrawableGC		PCLNAME(UpdateDrawableGC)
#define PclComputeCompositeClip		PCLNAME(ComputeCompositeClip)
#define PclValidateGC			PCLNAME(ValidateGC)

/* PclAttr.c */
#define PclGetAttributes		PCLNAME(GetAttributes)
#define PclGetOneAttribute		PCLNAME(GetOneAttribute)
#define PclAugmentAttributes		PCLNAME(AugmentAttributes)
#define PclSetAttributes		PCLNAME(SetAttributes)

/* PclColor.c */
#define PclLookUp			PCLNAME(LookUp)
#define PclCreateDefColormap		PCLNAME(CreateDefColormap)
#define PclCreateColormap		PCLNAME(CreateColormap)
#define PclDestroyColormap		PCLNAME(DestroyColormap)
#define PclInstallColormap		PCLNAME(InstallColormap)
#define PclUninstallColormap		PCLNAME(UninstallColormap)
#define PclListInstalledColormaps	PCLNAME(ListInstalledColormaps)
#define PclStoreColors			PCLNAME(StoreColors)
#define PclResolveColor			PCLNAME(ResolveColor)
#define PclFindPaletteMap		PCLNAME(FindPaletteMap)
#define PclUpdateColormap		PCLNAME(UpdateColormap)
#define PclReadMap			PCLNAME(ReadMap)

/* PclPixmap.c */
#define PclCreatePixmap			PCLNAME(CreatePixmap)
#define PclDestroyPixmap		PCLNAME(DestroyPixmap)

/* PclArc.c */
#define PclDoArc			PCLNAME(DoArc)
#define PclPolyArc			PCLNAME(PolyArc)
#define PclPolyFillArc			PCLNAME(PolyFillArc)

/* PclArea.c */
#define PclPutImage			PCLNAME(PutImage)
#define PclCopyArea			PCLNAME(CopyArea)
#define PclCopyPlane			PCLNAME(CopyPlane)

/* PclLine */
#define PclPolyLine			PCLNAME(PolyLine)
#define PclPolySegment			PCLNAME(PolySegment)

/* PclPixel.c */
#define PclPolyPoint			PCLNAME(PolyPoint)
#define PclPushPixels			PCLNAME(PushPixels)

/* PclPolygon.c */
#define PclPolyRectangle		PCLNAME(PolyRectangle)
#define PclFillPolygon			PCLNAME(FillPolygon)
#define PclPolyFillRect			PCLNAME(PolyFillRect)

/* PclSpans.c */
#define PclFillSpans			PCLNAME(FillSpans)
#define PclSetSpans			PCLNAME(SetSpans)

/* PclText.c */
#define PclPolyText8			PCLNAME(PolyText8)
#define PclPolyText16			PCLNAME(PolyText16)
#define PclImageText8			PCLNAME(ImageText8)
#define PclImageText16			PCLNAME(ImageText16)
#define PclImageGlyphBlt		PCLNAME(ImageGlyphBlt)
#define PclPolyGlyphBlt			PCLNAME(PolyGlyphBlt)
#define PclPolyGlyphBlt			PCLNAME(PolyGlyphBlt)

/* PclFonts.c */
#define PclRealizeFont			PCLNAME(RealizeFont)
#define PclUnrealizeFont		PCLNAME(UnrealizeFont)

/* PclSFonts.c */
#define PclDownloadSoftFont8		PCLNAME(DownloadSoftFont8)
#define PclDownloadSoftFont16		PCLNAME(DownloadSoftFont16)
#define PclCreateSoftFontInfo		PCLNAME(CreateSoftFontInfo)
#define PclDestroySoftFontInfo		PCLNAME(DestroySoftFontInfo)

/* PclMisc.c */
#define PclQueryBestSize		PCLNAME(QueryBestSize)
#define GetPropString			PCLNAME(GetPropString)
#define SystemCmd			PCLNAME(SystemCmd)
#define PclGetMediumDimensions		PCLNAME(GetMediumDimensions)
#define PclGetReproducibleArea		PCLNAME(GetReproducibleArea)
#define PclSpoolFigs			PCLNAME(SpoolFigs)
#define PclSendData			PCLNAME(SendData)

/* PclCursor.c */
#define PclConstrainCursor		PCLNAME(ConstrainCursor)
#define PclCursorLimits			PCLNAME(CursorLimits)
#define PclDisplayCursor		PCLNAME(DisplayCursor)
#define PclRealizeCursor		PCLNAME(RealizeCursor)
#define PclUnrealizeCursor		PCLNAME(UnrealizeCursor)
#define PclRecolorCursor		PCLNAME(RecolorCursor)
#define PclSetCursorPosition		PCLNAME(SetCursorPosition)

#endif

#endif /* _PCLMAP_H_ */
