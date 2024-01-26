/*
 * Copyright (C) 1994-1998 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */

/*
 * Unmap names
 */

#undef CFBNAME
#undef CATNAME

#undef QuartetBitsTable
#undef QuartetPixelMaskTable
#undef cfb8ClippedLineCopy
#undef cfb8ClippedLineGeneral 
#undef cfb8ClippedLineXor
#undef cfb8LineSS1Rect
#undef cfb8LineSS1RectCopy
#undef cfb8LineSS1RectGeneral 
#undef cfb8LineSS1RectPreviousCopy
#undef cfb8LineSS1RectXor
#undef cfb8SegmentSS1Rect
#undef cfb8SegmentSS1RectCopy
#undef cfb8SegmentSS1RectGeneral 
#undef cfb8SegmentSS1RectShiftCopy
#undef cfb8SegmentSS1RectXor
#undef cfbAllocatePrivates
#undef cfbBSFuncRec
#undef cfbBitBlt
#undef cfbBresD
#undef cfbBresS
#undef cfbChangeWindowAttributes
#undef cfbClearVisualTypes
#undef cfbCloseScreen
#undef cfbCreateDefColormap
#undef cfbCopyArea
#undef cfbCopyImagePlane
#undef cfbCopyPixmap
#undef cfbCopyPlane
#undef cfbCopyPlaneReduce
#undef cfbCopyRotatePixmap
#undef cfbCopyWindow
#undef cfbCreateGC
#undef cfbCreatePixmap
#undef cfbCreateScreenResources
#undef cfbCreateWindow
#undef cfbDestroyPixmap
#undef cfbDestroyWindow
#undef cfbDoBitblt
#undef cfbDoBitbltCopy
#undef cfbDoBitbltGeneral
#undef cfbDoBitbltOr
#undef cfbDoBitbltXor
#undef cfbExpandDirectColors
#undef cfbFillBoxTile32sCopy
#undef cfbFillBoxTile32sGeneral
#undef cfbFillBoxTileOdd
#undef cfbFillBoxTileOddCopy
#undef cfbFillBoxTileOddGeneral
#undef cfbFillPoly1RectCopy
#undef cfbFillPoly1RectGeneral
#undef cfbFillRectSolidCopy
#undef cfbFillRectSolidGeneral
#undef cfbFillRectSolidXor
#undef cfbFillRectTile32Copy
#undef cfbFillRectTile32General
#undef cfbFillRectTileOdd
#undef cfbFillSpanTile32sCopy
#undef cfbFillSpanTile32sGeneral
#undef cfbFillSpanTileOddCopy
#undef cfbFillSpanTileOddGeneral
#undef cfbFinishScreenInit
#undef cfbGCFuncs
#undef cfbGCPrivateKey
#undef cfbGetImage
#undef cfbGetScreenPixmap
#undef cfbGetSpans
#undef cfbHorzS
#undef cfbImageGlyphBlt8
#undef cfbInitializeColormap
#undef cfbInitVisuals
#undef cfbInstallColormap
#undef cfbLineSD
#undef cfbLineSS
#undef cfbListInstalledColormaps
#undef cfbMapWindow
#undef cfbMatchCommon
#undef cfbNonTEOps
#undef cfbNonTEOps1Rect
#undef cfbPadPixmap
#undef cfbPolyFillArcSolidCopy
#undef cfbPolyFillArcSolidGeneral
#undef cfbPolyFillRect
#undef cfbPolyGlyphBlt8
#undef cfbPolyGlyphRop8
#undef cfbPolyPoint
#undef cfbPositionWindow
#undef cfbPutImage
#undef cfbReduceRasterOp
#undef cfbResolveColor
#undef cfbRestoreAreas
#undef cfbSaveAreas
#undef cfbScreenInit
#undef cfbScreenPrivateKey
#undef cfbSegmentSD
#undef cfbSegmentSS
#undef cfbSetScanline
#undef cfbSetScreenPixmap
#undef cfbSetSpans
#undef cfbSetVisualTypes
#undef cfbSetupScreen
#undef cfbSolidSpansCopy
#undef cfbSolidSpansGeneral
#undef cfbSolidSpansXor
#undef cfbStippleStack
#undef cfbStippleStackTE
#undef cfbTEGlyphBlt
#undef cfbTEOps
#undef cfbTEOps1Rect
#undef cfbTile32FSCopy
#undef cfbTile32FSGeneral
#undef cfbUninstallColormap
#undef cfbUnmapWindow
#undef cfbUnnaturalStippleFS
#undef cfbUnnaturalTileFS
#undef cfbValidateGC
#undef cfbVertS
#undef cfbWindowPrivateKey
#undef cfbXRotatePixmap
#undef cfbYRotatePixmap
#undef cfbZeroPolyArcSS8Copy
#undef cfbZeroPolyArcSS8General
#undef cfbZeroPolyArcSS8Xor
#undef cfbendpartial
#undef cfbendtab
#undef cfbmask
#undef cfbrmask
#undef cfbstartpartial
#undef cfbstarttab
