/* $Xorg: PclSFonts.h,v 1.3 2000/08/17 19:48:08 cpqbld Exp $ */
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _PCLFONTS_H
#define _PCLFONTS_H

/* -*-H-*-
******************************************************************************
******************************************************************************
*
* File:         PclFonts.h
* Description:  Send Soft Font Download data to the specified file pointer.
*
*
******************************************************************************
******************************************************************************
*/
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


typedef struct {
	unsigned char fid;		/* sfont font ID */
	unsigned char cindex;		/* character indext */
} PclFontMapRec, PclFontMapPtr;

typedef struct {
	int h_offset;
	int v_offset;
	unsigned int width;
	unsigned int height;
	int font_pitch;
	unsigned char *raster_top;
} PclCharDataRec, *PclCharDataPtr;

typedef struct {
	unsigned char spacing;
	unsigned int pitch;
	unsigned int cellheight;
	unsigned int cellwidth;
	int ascent;
	int descent;
} PclFontDescRec, *PclFontDescPtr;

typedef struct _PclFontHead8Rec {
	char *fontname;
	PclFontDescRec fd;
	unsigned short fid;
	unsigned char *index;
	struct _PclFontHead8Rec *next;
} PclFontHead8Rec, *PclFontHead8Ptr;

typedef struct _PclFontHead16Rec {
	char *fontname;
	PclFontDescRec fd;
	unsigned short cur_fid;
	unsigned char cur_cindex;
	PclFontMapRec **index;
	unsigned short firstCol;
	unsigned short lastCol;
	unsigned short firstRow;
	unsigned short lastRow;
	struct _PclFontHead16Rec *next;
} PclFontHead16Rec, *PclFontHead16Ptr;

typedef struct _PclInternalFontRec {
	char *fontname;
	float pitch;
	float height;
	char *pcl_font_name;
	char *spacing;
	struct _PclInternalFontRec *next;
} PclInternalFontRec, *PclInternalFontPtr;

typedef struct {
	PclFontHead8Ptr phead8;
	PclFontHead16Ptr phead16;
	PclInternalFontPtr pinfont;
	unsigned char cur_max_fid;
} PclSoftFontInfoRec, *PclSoftFontInfoPtr;

#define MONOSPACE 0
#define PROPSPACE 1

#endif /* _PCLFONTS_H */
