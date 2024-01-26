/* $Xorg: DiPrint.h,v 1.3 2000/08/17 19:48:04 cpqbld Exp $ */
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
/*
 * The XpDiListEntry struct is the type of each element of the array
 * handed back to the extension code to handle a GetPrinterList request.
 * We don't use the printerDb directly because of the desire to handle
 * multiple locales.  Creating this new array for each GetPrinterList
 * request will allow us to build it with the description in the locale of
 * the requesting client.
 */
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _XpDiPrint_H_
#define _XpDiPrint_H_ 1

#include "scrnintstr.h"

typedef struct _diListEntry {
    char *name;
    char *description;
    char *localeName;
    unsigned long rootWinId;
} XpDiListEntry;

extern void XpDiFreePrinterList(XpDiListEntry **list);

extern XpDiListEntry **XpDiGetPrinterList(
    int nameLen,
    char *name,
    int localeLen,
    char *locale);

extern char * XpDiGetDriverName(int index, char *printerName);

extern WindowPtr XpDiValidatePrinter(char *printerName, int printerNameLen);

extern int PrinterOptions(int argc, char **argv, int i);

extern void PrinterUseMsg(void);

extern void PrinterInitGlobals(void);

extern void PrinterInitOutput(ScreenInfo *pScreenInfo, int argc, char **argv);

extern void _XpVoidNoop(void);

extern Bool _XpBoolNoop(void);

#endif /* _XpDiPrint_H_ */
