/* $XFree86: xc/programs/Xserver/include/swaprep.h,v 3.0 1996/04/15 11:34:34 dawes Exp $ */
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

#ifndef SWAPREP_H
#define SWAPREP_H 1

void
Swap32Write(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    CARD32 * /* pbuf */
#endif
);

void
CopySwap32Write(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    CARD32 * /* pbuf */
#endif
);

void
CopySwap16Write(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    short * /* pbuf */
#endif
);

void
SGenericReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGenericReply * /* pRep */
#endif
);

void
SGetWindowAttributesReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetWindowAttributesReply * /* pRep */
#endif
);

void
SGetGeometryReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetGeometryReply * /* pRep */
#endif
);

void
SQueryTreeReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xQueryTreeReply * /* pRep */
#endif
);

void
SInternAtomReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xInternAtomReply * /* pRep */
#endif
);

void
SGetAtomNameReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetAtomNameReply * /* pRep */
#endif
);

void
SGetPropertyReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetPropertyReply * /* pRep */
#endif
);

void
SListPropertiesReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xListPropertiesReply * /* pRep */
#endif
);

void
SGetSelectionOwnerReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetSelectionOwnerReply * /* pRep */
#endif
);

void
SQueryPointerReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xQueryPointerReply * /* pRep */
#endif
);

void
SwapTimecoord(
#if NeedFunctionPrototypes
    xTimecoord * /* pCoord */
#endif
);

void
SwapTimeCoordWrite(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xTimecoord * /* pRep */
#endif
);

void
SGetMotionEventsReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetMotionEventsReply * /* pRep */
#endif
);

void
STranslateCoordsReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xTranslateCoordsReply * /* pRep */
#endif
);

void
SGetInputFocusReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetInputFocusReply * /* pRep */
#endif
);

void
SQueryKeymapReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xQueryKeymapReply * /* pRep */
#endif
);

#ifdef LBX
void
SwapCharInfo(
#if NeedFunctionPrototypes
    xCharInfo * /* pInfo */
#endif
);
#endif

#ifdef LBX
void
SwapFont(
#if NeedFunctionPrototypes
    xQueryFontReply * /* pr */,
    Bool /* hasGlyphs */
#endif
);
#endif

void
SQueryFontReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xQueryFontReply * /* pRep */
#endif
);

void
SQueryTextExtentsReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xQueryTextExtentsReply * /* pRep */
#endif
);

void
SListFontsReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xListFontsReply * /* pRep */
#endif
);

void
SListFontsWithInfoReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xListFontsWithInfoReply * /* pRep */
#endif
);

void
SGetFontPathReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetFontPathReply * /* pRep */
#endif
);

void
SGetImageReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetImageReply * /* pRep */
#endif
);

void
SListInstalledColormapsReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xListInstalledColormapsReply * /* pRep */
#endif
);

void
SAllocColorReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xAllocColorReply * /* pRep */
#endif
);

void
SAllocNamedColorReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xAllocNamedColorReply * /* pRep */
#endif
);

void
SAllocColorCellsReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xAllocColorCellsReply * /* pRep */
#endif
);

void
SAllocColorPlanesReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xAllocColorPlanesReply * /* pRep */
#endif
);

void
SwapRGB(
#if NeedFunctionPrototypes
    xrgb * /* prgb */
#endif
);

void
SQColorsExtend(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xrgb * /* prgb */
#endif
);

void
SQueryColorsReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xQueryColorsReply * /* pRep */
#endif
);

void
SLookupColorReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xLookupColorReply * /* pRep */
#endif
);

void
SQueryBestSizeReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xQueryBestSizeReply * /* pRep */
#endif
);

void
SListExtensionsReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xListExtensionsReply * /* pRep */
#endif
);

void
SGetKeyboardMappingReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetKeyboardMappingReply * /* pRep */
#endif
);

void
SGetPointerMappingReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetPointerMappingReply * /* pRep */
#endif
);

void
SGetModifierMappingReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetModifierMappingReply * /* pRep */
#endif
);

void
SGetKeyboardControlReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetKeyboardControlReply * /* pRep */
#endif
);

void
SGetPointerControlReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetPointerControlReply * /* pRep */
#endif
);

void
SGetScreenSaverReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xGetScreenSaverReply * /* pRep */
#endif
);

void
SLHostsExtend(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    char * /* buf */
#endif
);

void
SListHostsReply(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    int /* size */,
    xListHostsReply * /* pRep */
#endif
);

void
SErrorEvent(
#if NeedFunctionPrototypes
    xError * /* from */,
    xError * /* to */
#endif
);

void
SwapConnSetupInfo(
#if NeedFunctionPrototypes
    char * /* pInfo */,
    char * /* pInfoTBase */
#endif
);

void
WriteSConnectionInfo(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    unsigned long /* size */,
    char * /* pInfo */
#endif
);

void
SwapConnSetup(
#if NeedFunctionPrototypes
    xConnSetup * /* pConnSetup */,
    xConnSetup * /* pConnSetupT */
#endif
);

void
SwapWinRoot(
#if NeedFunctionPrototypes
    xWindowRoot * /* pRoot */,
    xWindowRoot * /* pRootT */
#endif
);

void
SwapVisual(
#if NeedFunctionPrototypes
    xVisualType * /* pVis */,
    xVisualType * /* pVisT */
#endif
);

void
SwapConnSetupPrefix(
#if NeedFunctionPrototypes
    xConnSetupPrefix * /* pcspFrom */,
    xConnSetupPrefix * /* pcspTo */
#endif
);

void
WriteSConnSetupPrefix(
#if NeedFunctionPrototypes
    ClientPtr /* pClient */,
    xConnSetupPrefix * /* pcsp */
#endif
);

#undef SWAPREP_PROC
#if NeedFunctionPrototypes
#define SWAPREP_PROC(func) void func(xEvent * /* from */, xEvent * /* to */)
#else
#define SWAPREP_PROC(func) void func(/* xEvent * from,    xEvent *    to */)
#endif

SWAPREP_PROC(SCirculateEvent);
SWAPREP_PROC(SClientMessageEvent);
SWAPREP_PROC(SColormapEvent);
SWAPREP_PROC(SConfigureNotifyEvent);
SWAPREP_PROC(SConfigureRequestEvent);
SWAPREP_PROC(SCreateNotifyEvent);
SWAPREP_PROC(SDestroyNotifyEvent);
SWAPREP_PROC(SEnterLeaveEvent);
SWAPREP_PROC(SExposeEvent);
SWAPREP_PROC(SFocusEvent);
SWAPREP_PROC(SGraphicsExposureEvent);
SWAPREP_PROC(SGravityEvent);
SWAPREP_PROC(SKeyButtonPtrEvent);
SWAPREP_PROC(SKeymapNotifyEvent);
SWAPREP_PROC(SMapNotifyEvent);
SWAPREP_PROC(SMapRequestEvent);
SWAPREP_PROC(SMappingEvent);
SWAPREP_PROC(SNoExposureEvent);
SWAPREP_PROC(SPropertyEvent);
SWAPREP_PROC(SReparentEvent);
SWAPREP_PROC(SResizeRequestEvent);
SWAPREP_PROC(SSelectionClearEvent);
SWAPREP_PROC(SSelectionNotifyEvent);
SWAPREP_PROC(SSelectionRequestEvent);
SWAPREP_PROC(SUnmapNotifyEvent);
SWAPREP_PROC(SVisibilityEvent);

#undef SWAPREP_PROC

#endif /* SWAPREP_H */
