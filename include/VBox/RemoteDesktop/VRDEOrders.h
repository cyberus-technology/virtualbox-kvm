/** @file
 * VBox Remote Desktop Extension (VRDE) - Graphics Orders Structures.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef VBOX_INCLUDED_RemoteDesktop_VRDEOrders_h
#define VBOX_INCLUDED_RemoteDesktop_VRDEOrders_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

/*
 * VRDE gets an information about a graphical update as a pointer
 * to a memory block and the size of the memory block.
 * The memory block layout is:
 *    VRDEORDERHDR - Describes the affected rectangle.
 *    Then VRDE orders follow:
 *        VRDEORDERCODE;
 *        a VRDEORDER* structure.
 *
 * If size of the memory block is equal to the VRDEORDERHDR, then a bitmap
 * update is assumed.
 */

/* VRDE order codes. Must be >= 0, because the VRDE internally
 * uses negative values to mark some operations.
 */
#define VRDE_ORDER_DIRTY_RECT     (0)
#define VRDE_ORDER_SOLIDRECT      (1)
#define VRDE_ORDER_SOLIDBLT       (2)
#define VRDE_ORDER_DSTBLT         (3)
#define VRDE_ORDER_SCREENBLT      (4)
#define VRDE_ORDER_PATBLTBRUSH    (5)
#define VRDE_ORDER_MEMBLT         (6)
#define VRDE_ORDER_CACHED_BITMAP  (7)
#define VRDE_ORDER_DELETED_BITMAP (8)
#define VRDE_ORDER_LINE           (9)
#define VRDE_ORDER_BOUNDS         (10)
#define VRDE_ORDER_REPEAT         (11)
#define VRDE_ORDER_POLYLINE       (12)
#define VRDE_ORDER_ELLIPSE        (13)
#define VRDE_ORDER_SAVESCREEN     (14)
#define VRDE_ORDER_TEXT           (15)

/* 128 bit bitmap hash. */
typedef uint8_t VRDEBITMAPHASH[16];

#pragma pack(1)
typedef struct _VRDEORDERHDR
{
   /** Coordinates of the affected rectangle. */
   int16_t x;
   int16_t y;
   uint16_t w;
   uint16_t h;
} VRDEORDERHDR;

typedef struct _VRDEORDERCODE
{
   uint32_t u32Code;
} VRDEORDERCODE;

typedef struct _VRDEORDERPOINT
{
    int16_t  x;
    int16_t  y;
} VRDEORDERPOINT;

typedef struct _VRDEORDERPOLYPOINTS
{
    uint8_t  c;
    VRDEORDERPOINT a[16];
} VRDEORDERPOLYPOINTS;

typedef struct _VRDEORDERAREA
{
    int16_t  x;
    int16_t  y;
    uint16_t w;
    uint16_t h;
} VRDEORDERAREA;

typedef struct _VRDEORDERRECT
{
    int16_t left;
    int16_t top;
    int16_t right;
    int16_t bottom;
} VRDEORDERRECT;


typedef struct _VRDEORDERBOUNDS
{
    VRDEORDERPOINT pt1;
    VRDEORDERPOINT pt2;
} VRDEORDERBOUNDS;

typedef struct _VRDEORDERREPEAT
{
    VRDEORDERBOUNDS bounds;
} VRDEORDERREPEAT;


/* Header for bitmap bits. */
typedef struct _VRDEDATABITS
{
    uint32_t cb; /* Size of bitmap data without the header. */
    int16_t  x;
    int16_t  y;
    uint16_t cWidth;
    uint16_t cHeight;
    uint8_t  cbPixel;
    /* Bitmap data follow. */
} VRDEDATABITS;

typedef struct _VRDEORDERSOLIDRECT
{
    int16_t  x;
    int16_t  y;
    uint16_t w;
    uint16_t h;
    uint32_t rgb;
} VRDEORDERSOLIDRECT;

typedef struct _VRDEORDERSOLIDBLT
{
    int16_t  x;
    int16_t  y;
    uint16_t w;
    uint16_t h;
    uint32_t rgb;
    uint8_t  rop;
} VRDEORDERSOLIDBLT;

typedef struct _VRDEORDERDSTBLT
{
    int16_t  x;
    int16_t  y;
    uint16_t w;
    uint16_t h;
    uint8_t  rop;
} VRDEORDERDSTBLT;

typedef struct _VRDEORDERSCREENBLT
{
    int16_t  x;
    int16_t  y;
    uint16_t w;
    uint16_t h;
    int16_t  xSrc;
    int16_t  ySrc;
    uint8_t  rop;
} VRDEORDERSCREENBLT;

typedef struct _VRDEORDERPATBLTBRUSH
{
    int16_t  x;
    int16_t  y;
    uint16_t w;
    uint16_t h;
    int8_t   xSrc;
    int8_t   ySrc;
    uint32_t rgbFG;
    uint32_t rgbBG;
    uint8_t  rop;
    uint8_t  pattern[8];
} VRDEORDERPATBLTBRUSH;

typedef struct _VRDEORDERMEMBLT
{
    int16_t  x;
    int16_t  y;
    uint16_t w;
    uint16_t h;
    int16_t  xSrc;
    int16_t  ySrc;
    uint8_t  rop;
    VRDEBITMAPHASH hash;
} VRDEORDERMEMBLT;

typedef struct _VRDEORDERCACHEDBITMAP
{
    VRDEBITMAPHASH hash;
    /* VRDEDATABITS and the bitmap data follow. */
} VRDEORDERCACHEDBITMAP;

typedef struct _VRDEORDERDELETEDBITMAP
{
    VRDEBITMAPHASH hash;
} VRDEORDERDELETEDBITMAP;

typedef struct _VRDEORDERLINE
{
    int16_t  x1;
    int16_t  y1;
    int16_t  x2;
    int16_t  y2;
    int16_t  xBounds1;
    int16_t  yBounds1;
    int16_t  xBounds2;
    int16_t  yBounds2;
    uint8_t  mix;
    uint32_t rgb;
} VRDEORDERLINE;

typedef struct _VRDEORDERPOLYLINE
{
    VRDEORDERPOINT ptStart;
    uint8_t  mix;
    uint32_t rgb;
    VRDEORDERPOLYPOINTS points;
} VRDEORDERPOLYLINE;

typedef struct _VRDEORDERELLIPSE
{
    VRDEORDERPOINT pt1;
    VRDEORDERPOINT pt2;
    uint8_t  mix;
    uint8_t  fillMode;
    uint32_t rgb;
} VRDEORDERELLIPSE;

typedef struct _VRDEORDERSAVESCREEN
{
    VRDEORDERPOINT pt1;
    VRDEORDERPOINT pt2;
    uint8_t ident;
    uint8_t restore;
} VRDEORDERSAVESCREEN;

typedef struct _VRDEORDERGLYPH
{
    uint32_t o32NextGlyph;
    uint64_t u64Handle;

    /* The glyph origin position on the screen. */
    int16_t  x;
    int16_t  y;

    /* The glyph bitmap dimensions. Note w == h == 0 for the space character. */
    uint16_t w;
    uint16_t h;

    /* The character origin in the bitmap. */
    int16_t  xOrigin;
    int16_t  yOrigin;

    /* 1BPP bitmap. Rows are byte aligned. Size is (((w + 7)/8) * h + 3) & ~3. */
    uint8_t au8Bitmap[1];
} VRDEORDERGLYPH;

typedef struct _VRDEORDERTEXT
{
    uint32_t cbOrder;

    int16_t  xBkGround;
    int16_t  yBkGround;
    uint16_t wBkGround;
    uint16_t hBkGround;

    int16_t  xOpaque;
    int16_t  yOpaque;
    uint16_t wOpaque;
    uint16_t hOpaque;

    uint16_t u16MaxGlyph;

    uint8_t  u8Glyphs;
    uint8_t  u8Flags;
    uint16_t u8CharInc;
    uint32_t u32FgRGB;
    uint32_t u32BgRGB;

    /* u8Glyphs glyphs follow. Size of each glyph structure may vary. */
} VRDEORDERTEXT;
#pragma pack()

#endif /* !VBOX_INCLUDED_RemoteDesktop_VRDEOrders_h */
