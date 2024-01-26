
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef SPOOLER_H
#define SPOOLER_H 1

/* $Xorg: spooler.h,v 1.1 2003/09/14 1:19:56 gisburn Exp $ */
/*
Copyright (c) 2003-2004 Roland Mainz <roland.mainz@nrubsig.org>

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
 * Define platform-specific default spooler type
 */
#if defined(sun)
#define XPDEFAULTSPOOLERNAMELIST "solaris"
#elif defined(AIXV4)
#define XPDEFAULTSPOOLERNAMELIST "aix4"
#elif defined(hpux)
#define XPDEFAULTSPOOLERNAMELIST "hpux"
#elif defined(__osf__)
#define XPDEFAULTSPOOLERNAMELIST "osf"
#elif defined(__uxp__)
#define XPDEFAULTSPOOLERNAMELIST "uxp"
#elif defined(CSRG_BASED) || defined(linux)
/* ToDo: This should be "cups:bsd" in the future, but for now
 * the search order first-bsd-then-cups is better for backwards
 * compatibility.
 */
#define XPDEFAULTSPOOLERNAMELIST "bsd:cups"
#else
#define XPDEFAULTSPOOLERNAMELIST "other"
#endif

typedef struct
{
  const char  *name;
  const char  *list_queues_command;
  const char  *spool_command;
} XpSpoolerType, *XpSpoolerTypePtr;

/* prototypes */
extern XpSpoolerTypePtr  XpSpoolerNameToXpSpoolerType(char *name);
extern void              XpSetSpoolerTypeNameList(char *namelist);
extern char             *XpGetSpoolerTypeNameList(void);

/* global vars */
extern XpSpoolerTypePtr  spooler_type;
extern XpSpoolerType     xpstm[];

#endif /* !SPOOLER_H */

