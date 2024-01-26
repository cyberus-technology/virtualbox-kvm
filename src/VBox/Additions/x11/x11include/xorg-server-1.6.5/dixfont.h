/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef DIXFONT_H
#define DIXFONT_H 1

#include "dix.h"
#include <X11/fonts/font.h>
#include "closure.h"
#include <X11/fonts/fontstruct.h>

#define NullDIXFontProp ((DIXFontPropPtr)0)

typedef struct _DIXFontProp *DIXFontPropPtr;

extern Bool SetDefaultFont(char * /*defaultfontname*/);

extern void QueueFontWakeup(FontPathElementPtr /*fpe*/);

extern void RemoveFontWakeup(FontPathElementPtr /*fpe*/);

extern void FontWakeup(pointer /*data*/,
		       int /*count*/,
		       pointer /*LastSelectMask*/);

extern int OpenFont(ClientPtr /*client*/,
		    XID /*fid*/,
		    Mask /*flags*/,
		    unsigned /*lenfname*/,
		    char * /*pfontname*/);

extern int CloseFont(pointer /*pfont*/,
		     XID /*fid*/);

typedef struct _xQueryFontReply *xQueryFontReplyPtr;

extern void QueryFont(FontPtr /*pFont*/,
		      xQueryFontReplyPtr /*pReply*/,
		      int /*nProtoCCIStructs*/);

extern int ListFonts(ClientPtr /*client*/,
		     unsigned char * /*pattern*/,
		     unsigned int /*length*/,
		     unsigned int /*max_names*/);

int
doListFontsWithInfo(ClientPtr /*client*/,
		    LFWIclosurePtr /*c*/);

extern int doPolyText(ClientPtr /*client*/,
		      PTclosurePtr /*c*/
);

extern int PolyText(ClientPtr /*client*/,
		    DrawablePtr /*pDraw*/,
		    GCPtr /*pGC*/,
		    unsigned char * /*pElt*/,
		    unsigned char * /*endReq*/,
		    int /*xorg*/,
		    int /*yorg*/,
		    int /*reqType*/,
		    XID /*did*/);

extern int doImageText(ClientPtr /*client*/,
		       ITclosurePtr /*c*/);

extern int ImageText(ClientPtr /*client*/,
		     DrawablePtr /*pDraw*/,
		     GCPtr /*pGC*/,
		     int /*nChars*/,
		     unsigned char * /*data*/,
		     int /*xorg*/,
		     int /*yorg*/,
		     int /*reqType*/,
		     XID /*did*/);

extern int SetFontPath(ClientPtr /*client*/,
		       int /*npaths*/,
		       unsigned char * /*paths*/,
		       int * /*error*/);

extern int SetDefaultFontPath(char * /*path*/);

extern int GetFontPath(ClientPtr client,
		       int *count,
		       int *length,
		       unsigned char **result);

extern void DeleteClientFontStuff(ClientPtr /*client*/);

/* Quartz support on Mac OS X pulls in the QuickDraw
   framework whose InitFonts function conflicts here. */
#ifdef __APPLE__
#define InitFonts Darwin_X_InitFonts
#endif
extern void InitFonts(void);

extern void FreeFonts(void);

extern FontPtr find_old_font(XID /*id*/);

extern void GetGlyphs(FontPtr     /*font*/,
		      unsigned long /*count*/,
		      unsigned char * /*chars*/,
		      FontEncoding /*fontEncoding*/,
		      unsigned long * /*glyphcount*/,
		      CharInfoPtr * /*glyphs*/);

extern void QueryGlyphExtents(FontPtr     /*pFont*/,
			      CharInfoPtr * /*charinfo*/,
			      unsigned long /*count*/,
			      ExtentInfoPtr /*info*/);

extern Bool QueryTextExtents(FontPtr     /*pFont*/,
			     unsigned long /*count*/,
			     unsigned char * /*chars*/,
			     ExtentInfoPtr /*info*/);

extern Bool ParseGlyphCachingMode(char * /*str*/);

extern void InitGlyphCaching(void);

extern void SetGlyphCachingMode(int /*newmode*/);

/*
 * libXfont/src/builtins/builtin.h
 */
extern void BuiltinRegisterFpeFunctions(void);

/*
 * libXfont stubs.
 */
extern int client_auth_generation(ClientPtr client);

extern void DeleteFontClientID(Font id);

extern FontResolutionPtr GetClientResolutions(int *num);

extern int GetDefaultPointSize(void);

extern Font GetNewFontClientID(void);

extern int init_fs_handlers(FontPathElementPtr fpe,
				      BlockHandlerProcPtr block_handler);

extern int RegisterFPEFunctions(NameCheckFunc name_func,
					  InitFpeFunc init_func,
					  FreeFpeFunc free_func,
					  ResetFpeFunc reset_func,
					  OpenFontFunc open_func,
					  CloseFontFunc close_func,
					  ListFontsFunc list_func,
					  StartLfwiFunc start_lfwi_func,
					  NextLfwiFunc next_lfwi_func,
					  WakeupFpeFunc wakeup_func,
					  ClientDiedFunc client_died,
					  LoadGlyphsFunc load_glyphs,
					  StartLaFunc start_list_alias_func,
					  NextLaFunc next_list_alias_func,
					  SetPathFunc set_path_func);

extern void remove_fs_handlers(FontPathElementPtr fpe,
					 BlockHandlerProcPtr blockHandler,
					 Bool all);

extern int StoreFontClientFont(FontPtr pfont, Font id);

#endif				/* DIXFONT_H */
