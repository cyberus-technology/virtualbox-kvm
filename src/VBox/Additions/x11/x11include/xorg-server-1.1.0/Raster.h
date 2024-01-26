/* $Xorg: Raster.h,v 1.3 2000/08/17 19:48:12 cpqbld Exp $ */
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
/*******************************************************************
**
**    *********************************************************
**    *
**    *  File:		printer/Raster.h
**    *
**    *  Contents:  defines and includes for the raster layer
**    *             for a printing X server.
**    *
**    *  Copyright:	Copyright 1993 Hewlett-Packard Company
**    *
**    *********************************************************
** 
********************************************************************/
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _RASTER_H_
#define _RASTER_H_

/*
 * Some sleazes to force the XrmDB stuff into the server
 */
#ifndef HAVE_XPointer
#define HAVE_XPointer 1
typedef char *XPointer;
#endif
#define Status int
#define True 1
#define False 0
#include "misc.h"
#include <X11/Xfuncproto.h>
#include <X11/Xresource.h>
#include "attributes.h"

#include <X11/extensions/Printstr.h>

#define MAX_TOKEN_LEN 512

#define RASTER_PRINT_PAGE_COMMAND	"_XP_RASTER_PAGE_PROC_COMMAND"

#define RASTER_IN_FILE_STRING		"%(InFile)%"
#define RASTER_OUT_FILE_STRING		"%(OutFile)%"

#define RASTER_ALLOWED_COMMANDS_FILE	"printCommands"

/*
 * Defines for the "options" in DtPrintDocumentData.
 */
#define PRE_RASTER	"PRE-RASTER"
#define POST_RASTER	"POST-RASTER"
#define NO_RASTER	"NO-RASTER"


typedef struct {
    char *pBits;
    CreateWindowProcPtr CreateWindow;
    ChangeWindowAttributesProcPtr ChangeWindowAttributes;
    DestroyWindowProcPtr DestroyWindow;
    CloseScreenProcPtr CloseScreen;
} RasterScreenPrivRec, *RasterScreenPrivPtr;

typedef struct {
    XrmDatabase config;
    char *jobFileName;
    FILE *pJobFile;
    char *pageFileName;
    FILE *pPageFile;
    char *preRasterFileName; /* Pre-raster document data */
    FILE *pPreRasterFile;
    char *noRasterFileName; /* Raster replacement document data */
    FILE *pNoRasterFile;
    char *postRasterFileName; /* Post-raster document data */
    FILE *pPostRasterFile;
    ClientPtr getDocClient;
    int getDocBufSize;
} RasterContextPrivRec, *RasterContextPrivPtr;


extern XpValidatePoolsRec RasterValidatePoolsRec;

extern Bool InitializeRasterDriver(int ndx, ScreenPtr pScreen, int argc,
                                  char **argv);

#endif  /* _RASTER_H_ */
