/* $XFree86: xc/programs/Xserver/iplan2p4/iplmap.h,v 3.1 1998/04/05 16:42:26 robin Exp $ */
/*
 * $XConsortium: iplmap.h,v 1.9 94/04/17 20:28:54 dpw Exp $
 *
Copyright (c) 1991  X Consortium

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
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

/*
 * Map names around so that multiple depths can be supported simultaneously
 */

/* a losing vendor cpp dumps core if we define NAME in terms of CATNAME */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#if INTER_PLANES == 2
#define NAME(subname) ipl2p2##subname
#elif INTER_PLANES == 4
#define NAME(subname) ipl2p4##subname
#elif INTER_PLANES == 8
#define NAME(subname) ipl2p8##subname
#endif


#if !defined(UNIXCPP) || defined(ANSICPP)
#define CATNAME(prefix,subname) prefix##subname
#else
#define CATNAME(prefix,subname) prefix/**/subname
#endif

#define iplScreenPrivateIndex NAME(ScreenPrivateIndex)
#define QuartetBitsTable NAME(QuartetBitsTable)
#define QuartetPixelMaskTable NAME(QuartetPixelMaskTable)
#define iplAllocatePrivates NAME(AllocatePrivates)
#define iplBSFuncRec NAME(BSFuncRec)
#define iplBitBlt NAME(BitBlt)
#define iplBresD NAME(BresD)
#define iplBresS NAME(BresS)
#define iplChangeWindowAttributes NAME(ChangeWindowAttributes)
#define iplCloseScreen NAME(CloseScreen)
#define iplCopyArea NAME(CopyArea)
#define iplCopyImagePlane NAME(CopyImagePlane)
#define iplCopyPixmap NAME(CopyPixmap)
#define iplCopyPlane NAME(CopyPlane)
#define iplCopyRotatePixmap NAME(CopyRotatePixmap)
#define iplCopyWindow NAME(CopyWindow)
#define iplCreateGC NAME(CreateGC)
#define iplCreatePixmap NAME(CreatePixmap)
#define iplCreateWindow NAME(CreateWindow)
#define iplCreateScreenResources NAME(CreateScreenResoures)
#define iplDestroyPixmap NAME(DestroyPixmap)
#define iplDestroyWindow NAME(DestroyWindow)
#define iplDoBitblt NAME(DoBitblt)
#define iplDoBitbltCopy NAME(DoBitbltCopy)
#define iplDoBitbltGeneral NAME(DoBitbltGeneral)
#define iplDoBitbltOr NAME(DoBitbltOr)
#define iplDoBitbltXor NAME(DoBitbltXor)
#define iplFillBoxSolid NAME(FillBoxSolid)
#define iplFillBoxTile32 NAME(FillBoxTile32)
#define iplFillBoxTile32sCopy NAME(FillBoxTile32sCopy)
#define iplFillBoxTile32sGeneral NAME(FillBoxTile32sGeneral)
#define iplFillBoxTileOdd NAME(FillBoxTileOdd)
#define iplFillBoxTileOddCopy NAME(FillBoxTileOddCopy)
#define iplFillBoxTileOddGeneral NAME(FillBoxTileOddGeneral)
#define iplFillPoly1RectCopy NAME(FillPoly1RectCopy)
#define iplFillPoly1RectGeneral NAME(FillPoly1RectGeneral)
#define iplFillRectSolidCopy NAME(FillRectSolidCopy)
#define iplFillRectSolidGeneral NAME(FillRectSolidGeneral)
#define iplFillRectSolidXor NAME(FillRectSolidXor)
#define iplFillRectTile32Copy NAME(FillRectTile32Copy)
#define iplFillRectTile32General NAME(FillRectTile32General)
#define iplFillRectTileOdd NAME(FillRectTileOdd)
#define iplFillSpanTile32sCopy NAME(FillSpanTile32sCopy)
#define iplFillSpanTile32sGeneral NAME(FillSpanTile32sGeneral)
#define iplFillSpanTileOddCopy NAME(FillSpanTileOddCopy)
#define iplFillSpanTileOddGeneral NAME(FillSpanTileOddGeneral)
#define iplFinishScreenInit NAME(FinishScreenInit)
#define iplGCFuncs NAME(GCFuncs)
#define iplGetImage NAME(GetImage)
#define iplGetScreenPixmap NAME(GetScreenPixmap)
#define iplGetSpans NAME(GetSpans)
#define iplHorzS NAME(HorzS)
#define iplImageGlyphBlt8 NAME(ImageGlyphBlt8)
#define iplLineSD NAME(LineSD)
#define iplLineSS NAME(LineSS)
#define iplMapWindow NAME(MapWindow)
#define iplMatchCommon NAME(MatchCommon)
#define iplNonTEOps NAME(NonTEOps)
#define iplNonTEOps1Rect NAME(NonTEOps1Rect)
#define iplPadPixmap NAME(PadPixmap)
#define iplPaintWindow NAME(PaintWindow)
#define iplPolyGlyphBlt8 NAME(PolyGlyphBlt8)
#define iplPolyGlyphRop8 NAME(PolyGlyphRop8)
#define iplPolyFillArcSolidCopy NAME(PolyFillArcSolidCopy)
#define iplPolyFillArcSolidGeneral NAME(PolyFillArcSolidGeneral)
#define iplPolyFillRect NAME(PolyFillRect)
#define iplPolyPoint NAME(PolyPoint)
#define iplPositionWindow NAME(PositionWindow)
#define iplPutImage NAME(PutImage)
#define iplReduceRasterOp NAME(ReduceRasterOp)
#define iplRestoreAreas NAME(RestoreAreas)
#define iplSaveAreas NAME(SaveAreas)
#define iplScreenInit NAME(ScreenInit)
#define iplSegmentSD NAME(SegmentSD)
#define iplSegmentSS NAME(SegmentSS)
#define iplSetScanline NAME(SetScanline)
#define iplSetScreenPixmap NAME(SetScreenPixmap)
#define iplSetSpans NAME(SetSpans)
#define iplSetupScreen NAME(SetupScreen)
#define iplSolidSpansCopy NAME(SolidSpansCopy)
#define iplSolidSpansGeneral NAME(SolidSpansGeneral)
#define iplSolidSpansXor NAME(SolidSpansXor)
#define iplStippleStack NAME(StippleStack)
#define iplStippleStackTE NAME(StippleStackTE)
#define iplTEGlyphBlt NAME(TEGlyphBlt)
#define iplTEOps NAME(TEOps)
#define iplTEOps1Rect NAME(TEOps1Rect)
#define iplTile32FSCopy NAME(Tile32FSCopy)
#define iplTile32FSGeneral NAME(Tile32FSGeneral)
#define iplUnmapWindow NAME(UnmapWindow)
#define iplUnnaturalStippleFS NAME(UnnaturalStippleFS)
#define iplUnnaturalTileFS NAME(UnnaturalTileFS)
#define iplValidateGC NAME(ValidateGC)
#define iplVertS NAME(VertS)
#define iplXRotatePixmap NAME(XRotatePixmap)
#define iplYRotatePixmap NAME(YRotatePixmap)
#define iplendpartial NAME(endpartial)
#define iplendtab NAME(endtab)
#define iplmask NAME(mask)
#define iplrmask NAME(rmask)
#define iplstartpartial NAME(startpartial)
#define iplstarttab NAME(starttab)
#define ipl8LineSS1Rect NAME(LineSS1Rect)
#define ipl8SegmentSS1Rect NAME(SegmentSS1Rect)
#define ipl8ClippedLineCopy NAME(ClippedLineCopy)
#define ipl8ClippedLineXor NAME(ClippedLineXor)
#define ipl8ClippedLineGeneral  NAME(ClippedLineGeneral )
#define ipl8SegmentSS1RectCopy NAME(SegmentSS1RectCopy)
#define ipl8SegmentSS1RectXor NAME(SegmentSS1RectXor)
#define ipl8SegmentSS1RectGeneral  NAME(SegmentSS1RectGeneral )
#define ipl8SegmentSS1RectShiftCopy NAME(SegmentSS1RectShiftCopy)
#define ipl8LineSS1RectCopy NAME(LineSS1RectCopy)
#define ipl8LineSS1RectXor NAME(LineSS1RectXor)
#define ipl8LineSS1RectGeneral  NAME(LineSS1RectGeneral )
#define ipl8LineSS1RectPreviousCopy NAME(LineSS1RectPreviousCopy)
#define iplZeroPolyArcSS8Copy NAME(ZeroPolyArcSSCopy)
#define iplZeroPolyArcSS8Xor NAME(ZeroPolyArcSSXor)
#define iplZeroPolyArcSS8General NAME(ZeroPolyArcSSGeneral)
