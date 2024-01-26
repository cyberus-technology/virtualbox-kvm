/* $Xorg: psout.h,v 1.6 2001/02/09 02:04:37 xorgcvs Exp $ */
/*

Copyright 1996, 1998  The Open Group

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

*/
/*
 * (c) Copyright 1996 Hewlett-Packard Company
 * (c) Copyright 1996 International Business Machines Corp.
 * (c) Copyright 1996 Sun Microsystems, Inc.
 * (c) Copyright 1996 Novell, Inc.
 * (c) Copyright 1996 Digital Equipment Corp.
 * (c) Copyright 1996 Fujitsu Limited
 * (c) Copyright 1996 Hitachi, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the names of the copyright holders
 * shall not be used in advertising or otherwise to promote the sale, use
 * or other dealings in this Software without prior written authorization
 * from said copyright holders.
 */

/*******************************************************************
**
**    *********************************************************
**    *
**    *  File:          psout.h
**    *
**    *  Contents:      Include file for psout.c
**    *
**    *  Created By:    Roger Helmendach (Liberty Systems)
**    *
**    *  Copyright:     Copyright 1996 The Open Group, Inc.
**    *
**    *********************************************************
**
********************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _psout_
#define _psout_

#include <stdio.h>

typedef enum PsCapEnum_  { PsCButt=0,   PsCRound, PsCSquare    } PsCapEnum;
typedef enum PsJoinEnum_ { PsJMiter=0,  PsJRound, PsJBevel     } PsJoinEnum;
typedef enum PsArcEnum_  { PsChord,     PsPieSlice             } PsArcEnum;
typedef enum PsRuleEnum_ { PsEvenOdd,   PsNZWinding            } PsRuleEnum;
typedef enum PsFillEnum_ { PsSolid=0, PsTile, PsStip, PsOpStip } PsFillEnum;

typedef struct PsPointRec_
{
  int  x;
  int  y;
} PsPointRec;

typedef PsPointRec *PsPointPtr;

typedef struct PsRectRec_
{
  int  x;
  int  y;
  int  w;
  int  h;
} PsRectRec;

typedef PsRectRec *PsRectPtr;

typedef struct PsArcRec_
{
  int       x;
  int       y;
  int       w;
  int       h;
  int       a1;
  int       a2;
  PsArcEnum style;
} PsArcRec;

typedef PsArcRec *PsArcPtr;

#define PSOUT_RECT    0
#define PSOUT_ARC     1
#define PSOUT_POINTS  2

typedef struct PsElmRec_
{
  int  type;
  int  nPoints;
  union
  {
    PsRectRec  rect;
    PsArcRec   arc;
    PsPointPtr points;
  } c;
} PsElmRec;

typedef PsElmRec *PsElmPtr;

typedef struct PsClipRec_
{
  int        nRects;
  PsRectPtr  rects;
  int        nElms;
  PsElmPtr   elms;
  int        nOutterClips;
  PsRectPtr  outterClips;
} PsClipRec;

typedef PsClipRec *PsClipPtr;

typedef enum PsFTDownloadFontType_ 
{ 
  PsFontBitmap=0,
  PsFontType1,
  PsFontType3
} PsFTDownloadFontType;

/* Define |PsOutColor| color type which can hold one RGB value
 * (note: this needs to be |signed| long/long long to represent
 * special values such as |PSOUTCOLOR_NOCOLOR|)
 */
#ifdef PSOUT_USE_DEEPCOLOR
/* 64bit |PsOutColor| which can hold 16bit R-,G-,B-values */
#ifdef WIN32
typedef signed __int64    PsOutColor;
#else
# if defined(__alpha__) || defined(__alpha) || \
     defined(ia64) || defined(__ia64__) || \
     defined(__sparc64__) || defined(_LP64) || \
     defined(__s390x__) || \
     defined(amd64) || defined (__amd64__) || \
     defined (__powerpc64__) || \
     (defined(sgi) && (_MIPS_SZLONG == 64))
typedef signed long       PsOutColor;
# else
typedef signed long long  PsOutColor;
# endif /* native 64bit platform */
#endif /* WIN32 */

#define PSOUTCOLOR_TO_REDBITS(clr)    ((clr) >> 32)
#define PSOUTCOLOR_TO_GREENBITS(clr)  (((clr) >> 16) & 0xFFFF)
#define PSOUTCOLOR_TO_BLUEBITS(clr)   ((clr) & 0xFFFF)
#define PSOUTCOLOR_BITS_TO_PSFLOAT(b) ((float)(b) / 65535.)
#define PSOUTCOLOR_WHITE              (0xFFFFFFFFFFFFLL)
#define PSOUTCOLOR_NOCOLOR            (-1LL)
#define PSOUTCOLOR_TO_RGB24BIT(clr)   (((PSOUTCOLOR_TO_REDBITS(clr)   >> 8) << 16) | \
                                       ((PSOUTCOLOR_TO_GREENBITS(clr) >> 8) << 8)  | \
                                       ((PSOUTCOLOR_TO_BLUEBITS(clr)  >> 8) << 0))
#else
/* 32bit |PsOutColor| which can hold 8bit R-,G-,B-values */
typedef signed long PsOutColor;
#define PSOUTCOLOR_TO_REDBITS(clr)    ((clr) >> 16)
#define PSOUTCOLOR_TO_GREENBITS(clr)  (((clr) >> 8) & 0xFF)
#define PSOUTCOLOR_TO_BLUEBITS(clr)   ((clr) & 0xFF)
#define PSOUTCOLOR_BITS_TO_PSFLOAT(b) ((float)(b) / 255.)
#define PSOUTCOLOR_WHITE              (0xFFFFFF)
#define PSOUTCOLOR_NOCOLOR            (-1)
#define PSOUTCOLOR_TO_RGB24BIT(clr)   ((PSOUTCOLOR_TO_REDBITS(clr)   << 16) | \
                                       (PSOUTCOLOR_TO_GREENBITS(clr) << 8)  | \
                                       (PSOUTCOLOR_TO_BLUEBITS(clr)  << 0))
#endif /* PSOUT_USE_DEEPCOLOR */

#ifdef USE_PSOUT_PRIVATE
typedef void *voidPtr;

typedef struct PsPatRec_
{
  PsFillEnum type;
  voidPtr    tag;
} PsPatRec;

typedef PsPatRec *PsPatPtr;

typedef struct PsOutRec_
{
  FILE       *Fp;
  char        Buf[16384];
  PsOutColor  CurColor;
  int         LineWidth;
  PsCapEnum   LineCap;
  PsJoinEnum  LineJoin;
  int         NDashes;
  int        *Dashes;
  int         DashOffset;
  PsOutColor  LineBClr;
  PsRuleEnum  FillRule;
  char       *FontName;
  int         FontSize;
  float       FontMtx[4];
  int         ImageFormat;
  int         RevImage;
  int         NPatterns;
  int         MxPatterns;
  PsPatPtr    Patterns;
  int         ClipType;
  PsClipRec   Clip;
  int         InFrame;
  int         XOff;
  int         YOff;

  PsFillEnum  InTile;
  int         ImgSkip;
  PsOutColor  ImgBClr;
  PsOutColor  ImgFClr;
  int         ImgX;
  int         ImgY;
  int         ImgW;
  int         ImgH;
  int         SclW;
  int         SclH;

  Bool        isRaw;
  
  int         pagenum;

  int         start_image;
} PsOutRec;

typedef struct PsOutRec_ *PsOutPtr;

extern void S_Flush(PsOutPtr self);
extern void S_OutNum(PsOutPtr self, float num);
extern void S_OutTok(PsOutPtr self, char *tok, int cr);
#else
typedef struct PsOutRec_ *PsOutPtr;
#endif /* USE_PSOUT_PRIVATE */

extern PsOutPtr PsOut_BeginFile(FILE *fp, char *title, int orient, int count, int plex,
                                int res, int wd, int ht, Bool raw);
extern void PsOut_EndFile(PsOutPtr self, int closeFile);
extern void PsOut_BeginPage(PsOutPtr self, int orient, int count, int plex,
                            int res, int wd, int ht);
extern void PsOut_EndPage(PsOutPtr self);
extern void PsOut_DirtyAttributes(PsOutPtr self);
extern void PsOut_Comment(PsOutPtr self, char *comment);
extern void PsOut_Offset(PsOutPtr self, int x, int y);

extern void PsOut_Clip(PsOutPtr self, int clpTyp, PsClipPtr clpinf);

extern void PsOut_Color(PsOutPtr self, PsOutColor clr);
extern void PsOut_FillRule(PsOutPtr self, PsRuleEnum rule);
extern void PsOut_LineAttrs(PsOutPtr self, int wd, PsCapEnum cap,
                            PsJoinEnum join, int nDsh, int *dsh, int dshOff,
                            PsOutColor bclr);
extern void PsOut_TextAttrs(PsOutPtr self, char *fnam, int siz, int iso);
extern void PsOut_TextAttrsMtx(PsOutPtr self, char *fnam, float *mtx, int iso);

extern void PsOut_Polygon(PsOutPtr self, int nPts, PsPointPtr pts);
extern void PsOut_FillRect(PsOutPtr self, int x, int y, int w, int h);
extern void PsOut_FillArc(PsOutPtr self, int x, int y, int w, int h,
                          float ang1, float ang2, PsArcEnum style);

extern void PsOut_Lines(PsOutPtr self, int nPts, PsPointPtr pts);
extern void PsOut_Points(PsOutPtr self, int nPts, PsPointPtr pts);
extern void PsOut_DrawRect(PsOutPtr self, int x, int y, int w, int h);
extern void PsOut_DrawArc(PsOutPtr self, int x, int y, int w, int h,
                          float ang1, float ang2);

extern void PsOut_Text(PsOutPtr self, int x, int y, char *text, int textl,
                       PsOutColor bclr);
extern void PsOut_Text16(PsOutPtr self, int x, int y, unsigned short *text, int textl, PsOutColor bclr);

extern void PsOut_BeginImage(PsOutPtr self, PsOutColor bclr, PsOutColor fclr, int x, int y,
                             int w, int h, int sw, int sh, int format);
extern void PsOut_BeginImageIM(PsOutPtr self, PsOutColor bclr, PsOutColor fclr, int x, int y,
                               int w, int h, int sw, int sh, int format);
extern void PsOut_EndImage(PsOutPtr self);
extern void PsOut_OutImageBytes(PsOutPtr self, int nBytes, char *bytes);

extern void PsOut_BeginFrame(PsOutPtr self, int xoff, int yoff, int x, int y,
                             int w, int h);
extern void PsOut_EndFrame(PsOutPtr self);

extern int  PsOut_BeginPattern(PsOutPtr self, void *tag, int w, int h,
                               PsFillEnum type, PsOutColor bclr, PsOutColor fclr);
extern void PsOut_EndPattern(PsOutPtr self);
extern void PsOut_SetPattern(PsOutPtr self, void *tag, PsFillEnum type);

extern void PsOut_RawData(PsOutPtr self, char *data, int len);

extern int  PsOut_DownloadType1(PsOutPtr self, const char *auditmsg, const char *name, const char *fname);

extern int  PsOut_DownloadFreeType1(PsOutPtr self, const char *psfontname, FontPtr pFont, long block_offset);
extern int  PsOut_DownloadFreeType3(PsOutPtr self, const char *psfontname, FontPtr pFont, long block_offset);

extern int  PsOut_DownloadFreeType(PsOutPtr self, PsFTDownloadFontType downloadfonttype, const char *psfontname, FontPtr pFont, long block_offset);
extern void PsOut_Get_FreeType_Glyph_Name( char *destbuf, FontPtr pFont, unsigned long x11fontindex);
extern void PsOut_FreeType_Text(FontPtr pFont, PsOutPtr self, int x, int y, char *text, int textl);
extern void PsOut_FreeType_Text16(FontPtr pFont, PsOutPtr self, int x, int y, unsigned short *text, int textl);

extern void PsOut_FreeType_TextAttrs16(PsOutPtr self, char *fnam, int siz, int iso);
extern void PsOut_FreeType_TextAttrsMtx16(PsOutPtr self, char *fnam, float *mtx, int iso);
#endif
