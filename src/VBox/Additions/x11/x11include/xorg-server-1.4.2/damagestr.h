/*
 * Copyright © 2003 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _DAMAGESTR_H_
#define _DAMAGESTR_H_

#include "damage.h"
#include "gcstruct.h"
#ifdef RENDER
# include "picturestr.h"
#endif

typedef struct _damage {
    DamagePtr		pNext;
    DamagePtr		pNextWin;
    RegionRec		damage;
    
    DamageReportLevel	damageLevel;
    Bool		isInternal;
    void		*closure;
    Bool		isWindow;
    DrawablePtr		pDrawable;
    
    DamageReportFunc	damageReport;
    DamageDestroyFunc	damageDestroy;

    Bool		reportAfter;
    RegionRec		pendingDamage;
} DamageRec;

typedef struct _damageScrPriv {
    int				internalLevel;

    /*
     * For DDXen which don't provide GetScreenPixmap, this provides
     * a place to hook damage for windows on the screen
     */
    DamagePtr			pScreenDamage;

    PaintWindowBackgroundProcPtr PaintWindowBackground;
    PaintWindowBorderProcPtr	PaintWindowBorder;
    CopyWindowProcPtr		CopyWindow;
    CloseScreenProcPtr		CloseScreen;
    CreateGCProcPtr		CreateGC;
    DestroyPixmapProcPtr	DestroyPixmap;
    SetWindowPixmapProcPtr	SetWindowPixmap;
    DestroyWindowProcPtr	DestroyWindow;
#ifdef RENDER
    CompositeProcPtr		Composite;
    GlyphsProcPtr		Glyphs;
#endif
    BSFuncRec			BackingStoreFuncs;
} DamageScrPrivRec, *DamageScrPrivPtr;

typedef struct _damageGCPriv {
    GCOps   *ops;
    GCFuncs *funcs;
} DamageGCPrivRec, *DamageGCPrivPtr;

/* XXX should move these into damage.c, damageScrPrivateIndex is static */
#define damageGetScrPriv(pScr) \
    ((DamageScrPrivPtr) (pScr)->devPrivates[damageScrPrivateIndex].ptr)

#define damageScrPriv(pScr) \
    DamageScrPrivPtr    pScrPriv = damageGetScrPriv(pScr)

#define damageGetPixPriv(pPix) \
    ((DamagePtr) (pPix)->devPrivates[damagePixPrivateIndex].ptr)

#define damgeSetPixPriv(pPix,v) \
    ((pPix)->devPrivates[damagePixPrivateIndex].ptr = (pointer ) (v))

#define damagePixPriv(pPix) \
    DamagePtr	    pDamage = damageGetPixPriv(pPix)

#define damageGetGCPriv(pGC) \
    ((DamageGCPrivPtr) (pGC)->devPrivates[damageGCPrivateIndex].ptr)

#define damageGCPriv(pGC) \
    DamageGCPrivPtr  pGCPriv = damageGetGCPriv(pGC)

#define damageGetWinPriv(pWin) \
    ((DamagePtr) (pWin)->devPrivates[damageWinPrivateIndex].ptr)

#define damageSetWinPriv(pWin,d) \
    ((pWin)->devPrivates[damageWinPrivateIndex].ptr = (d))

#endif /* _DAMAGESTR_H_ */
