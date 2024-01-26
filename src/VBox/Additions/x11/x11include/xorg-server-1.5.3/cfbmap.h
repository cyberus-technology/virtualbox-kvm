/*
 *
Copyright 1991, 1998  The Open Group

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
 *
 * Author:  Keith Packard, MIT X Consortium
 */


/*
 * Map names around so that multiple depths can be supported simultaneously
 */

/* a losing vendor cpp dumps core if we define CFBNAME in terms of CATNAME */

#if PSZ != 8

#if PSZ == 32
#if !defined(UNIXCPP) || defined(ANSICPP)
#define CFBNAME(subname) cfb32##subname
#else
#define CFBNAME(subname) cfb32/**/subname
#endif
#endif

#if PSZ == 24
#if !defined(UNIXCPP) || defined(ANSICPP)
#define CFBNAME(subname) cfb24##subname
#else
#define CFBNAME(subname) cfb24/**/subname
#endif
#endif

#if PSZ == 16
#if !defined(UNIXCPP) || defined(ANSICPP)
#define CFBNAME(subname) cfb16##subname
#else
#define CFBNAME(subname) cfb16/**/subname
#endif
#endif

#if PSZ == 4
#if !defined(UNIXCPP) || defined(ANSICPP)
#define CFBNAME(subname) cfb4##subname
#else
#define CFBNAME(subname) cfb4/**/subname
#endif
#endif

#ifndef CFBNAME
cfb can not hack PSZ yet
#endif

#undef CATNAME

#if !defined(UNIXCPP) || defined(ANSICPP)
#define CATNAME(prefix,subname) prefix##subname
#else
#define CATNAME(prefix,subname) prefix/**/subname
#endif

#define QuartetBitsTable CFBNAME(QuartetBitsTable)
#define QuartetPixelMaskTable CFBNAME(QuartetPixelMaskTable)
#define cfb8ClippedLineCopy CFBNAME(ClippedLineCopy)
#define cfb8ClippedLineGeneral  CFBNAME(ClippedLineGeneral )
#define cfb8ClippedLineXor CFBNAME(ClippedLineXor)
#define cfb8LineSS1Rect CFBNAME(LineSS1Rect)
#define cfb8LineSS1RectCopy CFBNAME(LineSS1RectCopy)
#define cfb8LineSS1RectGeneral  CFBNAME(LineSS1RectGeneral )
#define cfb8LineSS1RectPreviousCopy CFBNAME(LineSS1RectPreviousCopy)
#define cfb8LineSS1RectXor CFBNAME(LineSS1RectXor)
#define cfb8SegmentSS1Rect CFBNAME(SegmentSS1Rect)
#define cfb8SegmentSS1RectCopy CFBNAME(SegmentSS1RectCopy)
#define cfb8SegmentSS1RectGeneral  CFBNAME(SegmentSS1RectGeneral )
#define cfb8SegmentSS1RectShiftCopy CFBNAME(SegmentSS1RectShiftCopy)
#define cfb8SegmentSS1RectXor CFBNAME(SegmentSS1RectXor)
#define cfbAllocatePrivates CFBNAME(AllocatePrivates)
#define cfbBSFuncRec CFBNAME(BSFuncRec)
#define cfbBitBlt CFBNAME(BitBlt)
#define cfbBresD CFBNAME(BresD)
#define cfbBresS CFBNAME(BresS)
#define cfbChangeWindowAttributes CFBNAME(ChangeWindowAttributes)
#define cfbClearVisualTypes CFBNAME(cfbClearVisualTypes)
#define cfbCloseScreen CFBNAME(CloseScreen)
#define cfbCreateDefColormap CFBNAME (cfbCreateDefColormap)
#define cfbCopyArea CFBNAME(CopyArea)
#define cfbCopyImagePlane CFBNAME(CopyImagePlane)
#define cfbCopyPixmap CFBNAME(CopyPixmap)
#define cfbCopyPlane CFBNAME(CopyPlane)
#define cfbCopyPlaneReduce CFBNAME(CopyPlaneReduce)
#define cfbCopyRotatePixmap CFBNAME(CopyRotatePixmap)
#define cfbCopyWindow CFBNAME(CopyWindow)
#define cfbCreateGC CFBNAME(CreateGC)
#define cfbCreatePixmap CFBNAME(CreatePixmap)
#define cfbCreateScreenResources CFBNAME(CreateScreenResources)
#define cfbCreateWindow CFBNAME(CreateWindow)
#define cfbDestroyPixmap CFBNAME(DestroyPixmap)
#define cfbDestroyWindow CFBNAME(DestroyWindow)
#define cfbDoBitblt CFBNAME(DoBitblt)
#define cfbDoBitbltCopy CFBNAME(DoBitbltCopy)
#define cfbDoBitbltGeneral CFBNAME(DoBitbltGeneral)
#define cfbDoBitbltOr CFBNAME(DoBitbltOr)
#define cfbDoBitbltXor CFBNAME(DoBitbltXor)
#define cfbExpandDirectColors CFBNAME(cfbExpandDirectColors)
#define cfbFillBoxTile32sCopy CFBNAME(FillBoxTile32sCopy)
#define cfbFillBoxTile32sGeneral CFBNAME(FillBoxTile32sGeneral)
#define cfbFillBoxTileOdd CFBNAME(FillBoxTileOdd)
#define cfbFillBoxTileOddCopy CFBNAME(FillBoxTileOddCopy)
#define cfbFillBoxTileOddGeneral CFBNAME(FillBoxTileOddGeneral)
#define cfbFillPoly1RectCopy CFBNAME(FillPoly1RectCopy)
#define cfbFillPoly1RectGeneral CFBNAME(FillPoly1RectGeneral)
#define cfbFillRectSolidCopy CFBNAME(FillRectSolidCopy)
#define cfbFillRectSolidGeneral CFBNAME(FillRectSolidGeneral)
#define cfbFillRectSolidXor CFBNAME(FillRectSolidXor)
#define cfbFillRectTile32Copy CFBNAME(FillRectTile32Copy)
#define cfbFillRectTile32General CFBNAME(FillRectTile32General)
#define cfbFillRectTileOdd CFBNAME(FillRectTileOdd)
#define cfbFillSpanTile32sCopy CFBNAME(FillSpanTile32sCopy)
#define cfbFillSpanTile32sGeneral CFBNAME(FillSpanTile32sGeneral)
#define cfbFillSpanTileOddCopy CFBNAME(FillSpanTileOddCopy)
#define cfbFillSpanTileOddGeneral CFBNAME(FillSpanTileOddGeneral)
#define cfbFinishScreenInit CFBNAME(FinishScreenInit)
#define cfbGCFuncs CFBNAME(GCFuncs)
#define cfbGCPrivateKey CFBNAME(GCPrivateKey)
#define cfbGetImage CFBNAME(GetImage)
#define cfbGetScreenPixmap CFBNAME(GetScreenPixmap)
#define cfbGetSpans CFBNAME(GetSpans)
#define cfbHorzS CFBNAME(HorzS)
#define cfbImageGlyphBlt8 CFBNAME(ImageGlyphBlt8)
#define cfbInitializeColormap CFBNAME(InitializeColormap)
#define cfbInitVisuals CFBNAME(cfbInitVisuals)
#define cfbInstallColormap CFBNAME(InstallColormap)
#define cfbLineSD CFBNAME(LineSD)
#define cfbLineSS CFBNAME(LineSS)
#define cfbListInstalledColormaps CFBNAME(ListInstalledColormaps)
#define cfbMapWindow CFBNAME(MapWindow)
#define cfbMatchCommon CFBNAME(MatchCommon)
#define cfbNonTEOps CFBNAME(NonTEOps)
#define cfbNonTEOps1Rect CFBNAME(NonTEOps1Rect)
#define cfbPadPixmap CFBNAME(PadPixmap)
#define cfbPolyFillArcSolidCopy CFBNAME(PolyFillArcSolidCopy)
#define cfbPolyFillArcSolidGeneral CFBNAME(PolyFillArcSolidGeneral)
#define cfbPolyFillRect CFBNAME(PolyFillRect)
#define cfbPolyGlyphBlt8 CFBNAME(PolyGlyphBlt8)
#define cfbPolyGlyphRop8 CFBNAME(PolyGlyphRop8)
#define cfbPolyPoint CFBNAME(PolyPoint)
#define cfbPositionWindow CFBNAME(PositionWindow)
#define cfbPutImage CFBNAME(PutImage)
#define cfbReduceRasterOp CFBNAME(ReduceRasterOp)
#define cfbResolveColor CFBNAME(ResolveColor)
#define cfbRestoreAreas CFBNAME(RestoreAreas)
#define cfbSaveAreas CFBNAME(SaveAreas)
#define cfbScreenInit CFBNAME(ScreenInit)
#define cfbScreenPrivateKey CFBNAME(ScreenPrivateKey)
#define cfbSegmentSD CFBNAME(SegmentSD)
#define cfbSegmentSS CFBNAME(SegmentSS)
#define cfbSetScanline CFBNAME(SetScanline)
#define cfbSetScreenPixmap CFBNAME(SetScreenPixmap)
#define cfbSetSpans CFBNAME(SetSpans)
#define cfbSetVisualTypes CFBNAME(cfbSetVisualTypes)
#define cfbSetupScreen CFBNAME(SetupScreen)
#define cfbSolidSpansCopy CFBNAME(SolidSpansCopy)
#define cfbSolidSpansGeneral CFBNAME(SolidSpansGeneral)
#define cfbSolidSpansXor CFBNAME(SolidSpansXor)
#define cfbStippleStack CFBNAME(StippleStack)
#define cfbStippleStackTE CFBNAME(StippleStackTE)
#define cfbTEGlyphBlt CFBNAME(TEGlyphBlt)
#define cfbTEOps CFBNAME(TEOps)
#define cfbTEOps1Rect CFBNAME(TEOps1Rect)
#define cfbTile32FSCopy CFBNAME(Tile32FSCopy)
#define cfbTile32FSGeneral CFBNAME(Tile32FSGeneral)
#define cfbUninstallColormap CFBNAME(UninstallColormap)
#define cfbUnmapWindow CFBNAME(UnmapWindow)
#define cfbUnnaturalStippleFS CFBNAME(UnnaturalStippleFS)
#define cfbUnnaturalTileFS CFBNAME(UnnaturalTileFS)
#define cfbValidateGC CFBNAME(ValidateGC)
#define cfbVertS CFBNAME(VertS)
#define cfbWindowPrivateKey CFBNAME(WindowPrivateKey)
#define cfbXRotatePixmap CFBNAME(XRotatePixmap)
#define cfbYRotatePixmap CFBNAME(YRotatePixmap)
#define cfbZeroPolyArcSS8Copy CFBNAME(ZeroPolyArcSSCopy)
#define cfbZeroPolyArcSS8General CFBNAME(ZeroPolyArcSSGeneral)
#define cfbZeroPolyArcSS8Xor CFBNAME(ZeroPolyArcSSXor)
#define cfbendpartial CFBNAME(endpartial)
#define cfbendtab CFBNAME(endtab)
#define cfbmask CFBNAME(mask)
#define cfbrmask CFBNAME(rmask)
#define cfbstartpartial CFBNAME(startpartial)
#define cfbstarttab CFBNAME(starttab)

#endif /* PSZ != 8 */
