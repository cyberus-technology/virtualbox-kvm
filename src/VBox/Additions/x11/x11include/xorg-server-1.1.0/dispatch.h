/* $XFree86: xc/programs/Xserver/dix/dispatch.h,v 3.2 2001/08/01 00:44:48 tsi Exp $ */
/************************************************************

Copyright 1996 by Thomas E. Dickey <dickey@clark.net>

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of the above listed
copyright holder(s) not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

/*
 * This prototypes the dispatch.c module (except for functions declared in
 * global headers), plus related dispatch procedures from devices.c, events.c,
 * extension.c, property.c. 
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef DISPATCH_H
#define DISPATCH_H 1

DISPATCH_PROC(InitClientPrivates);
DISPATCH_PROC(ProcAllocColor);
DISPATCH_PROC(ProcAllocColorCells);
DISPATCH_PROC(ProcAllocColorPlanes);
DISPATCH_PROC(ProcAllocNamedColor);
DISPATCH_PROC(ProcBell);
DISPATCH_PROC(ProcChangeAccessControl);
DISPATCH_PROC(ProcChangeCloseDownMode);
DISPATCH_PROC(ProcChangeGC);
DISPATCH_PROC(ProcChangeHosts);
DISPATCH_PROC(ProcChangeKeyboardControl);
DISPATCH_PROC(ProcChangeKeyboardMapping);
DISPATCH_PROC(ProcChangePointerControl);
DISPATCH_PROC(ProcChangeProperty);
DISPATCH_PROC(ProcChangeSaveSet);
DISPATCH_PROC(ProcChangeWindowAttributes);
DISPATCH_PROC(ProcCirculateWindow);
DISPATCH_PROC(ProcClearToBackground);
DISPATCH_PROC(ProcCloseFont);
DISPATCH_PROC(ProcConfigureWindow);
DISPATCH_PROC(ProcConvertSelection);
DISPATCH_PROC(ProcCopyArea);
DISPATCH_PROC(ProcCopyColormapAndFree);
DISPATCH_PROC(ProcCopyGC);
DISPATCH_PROC(ProcCopyPlane);
DISPATCH_PROC(ProcCreateColormap);
DISPATCH_PROC(ProcCreateCursor);
DISPATCH_PROC(ProcCreateGC);
DISPATCH_PROC(ProcCreateGlyphCursor);
DISPATCH_PROC(ProcCreatePixmap);
DISPATCH_PROC(ProcCreateWindow);
DISPATCH_PROC(ProcDeleteProperty);
DISPATCH_PROC(ProcDestroySubwindows);
DISPATCH_PROC(ProcDestroyWindow);
DISPATCH_PROC(ProcEstablishConnection);
DISPATCH_PROC(ProcFillPoly);
DISPATCH_PROC(ProcForceScreenSaver);
DISPATCH_PROC(ProcFreeColormap);
DISPATCH_PROC(ProcFreeColors);
DISPATCH_PROC(ProcFreeCursor);
DISPATCH_PROC(ProcFreeGC);
DISPATCH_PROC(ProcFreePixmap);
DISPATCH_PROC(ProcGetAtomName);
DISPATCH_PROC(ProcGetFontPath);
DISPATCH_PROC(ProcGetGeometry);
DISPATCH_PROC(ProcGetImage);
DISPATCH_PROC(ProcGetKeyboardControl);
DISPATCH_PROC(ProcGetKeyboardMapping);
DISPATCH_PROC(ProcGetModifierMapping);
DISPATCH_PROC(ProcGetMotionEvents);
DISPATCH_PROC(ProcGetPointerControl);
DISPATCH_PROC(ProcGetPointerMapping);
DISPATCH_PROC(ProcGetProperty);
DISPATCH_PROC(ProcGetScreenSaver);
DISPATCH_PROC(ProcGetSelectionOwner);
DISPATCH_PROC(ProcGetWindowAttributes);
DISPATCH_PROC(ProcGrabServer);
DISPATCH_PROC(ProcImageText16);
DISPATCH_PROC(ProcImageText8);
DISPATCH_PROC(ProcInitialConnection);
DISPATCH_PROC(ProcInstallColormap);
DISPATCH_PROC(ProcInternAtom);
DISPATCH_PROC(ProcKillClient);
DISPATCH_PROC(ProcListExtensions);
DISPATCH_PROC(ProcListFonts);
DISPATCH_PROC(ProcListFontsWithInfo);
DISPATCH_PROC(ProcListHosts);
DISPATCH_PROC(ProcListInstalledColormaps);
DISPATCH_PROC(ProcListProperties);
DISPATCH_PROC(ProcLookupColor);
DISPATCH_PROC(ProcMapSubwindows);
DISPATCH_PROC(ProcMapWindow);
DISPATCH_PROC(ProcNoOperation);
DISPATCH_PROC(ProcOpenFont);
DISPATCH_PROC(ProcPolyArc);
DISPATCH_PROC(ProcPolyFillArc);
DISPATCH_PROC(ProcPolyFillRectangle);
DISPATCH_PROC(ProcPolyLine);
DISPATCH_PROC(ProcPolyPoint);
DISPATCH_PROC(ProcPolyRectangle);
DISPATCH_PROC(ProcPolySegment);
DISPATCH_PROC(ProcPolyText);
DISPATCH_PROC(ProcPutImage);
DISPATCH_PROC(ProcQueryBestSize);
DISPATCH_PROC(ProcQueryColors);
DISPATCH_PROC(ProcQueryExtension);
DISPATCH_PROC(ProcQueryFont);
DISPATCH_PROC(ProcQueryKeymap);
DISPATCH_PROC(ProcQueryTextExtents);
DISPATCH_PROC(ProcQueryTree);
DISPATCH_PROC(ProcReparentWindow);
DISPATCH_PROC(ProcRotateProperties);
DISPATCH_PROC(ProcSetClipRectangles);
DISPATCH_PROC(ProcSetDashes);
DISPATCH_PROC(ProcSetFontPath);
DISPATCH_PROC(ProcSetModifierMapping);
DISPATCH_PROC(ProcSetPointerMapping);
DISPATCH_PROC(ProcSetScreenSaver);
DISPATCH_PROC(ProcSetSelectionOwner);
DISPATCH_PROC(ProcStoreColors);
DISPATCH_PROC(ProcStoreNamedColor);
DISPATCH_PROC(ProcTranslateCoords);
DISPATCH_PROC(ProcUngrabServer);
DISPATCH_PROC(ProcUninstallColormap);
DISPATCH_PROC(ProcUnmapSubwindows);
DISPATCH_PROC(ProcUnmapWindow);

#endif /* DISPATCH_H */
