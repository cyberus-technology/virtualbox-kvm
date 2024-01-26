/*
 * $Id$
 *
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

#ifndef _DAMAGE_H_
#define _DAMAGE_H_

typedef struct _damage	*DamagePtr;

typedef enum _damageReportLevel {
    DamageReportRawRegion,
    DamageReportDeltaRegion,
    DamageReportBoundingBox,
    DamageReportNonEmpty,
    DamageReportNone
} DamageReportLevel;

typedef void (*DamageReportFunc) (DamagePtr pDamage, RegionPtr pRegion, void *closure);
typedef void (*DamageDestroyFunc) (DamagePtr pDamage, void *closure);

Bool
DamageSetup (ScreenPtr pScreen);
    
DamagePtr
DamageCreate (DamageReportFunc  damageReport,
	      DamageDestroyFunc	damageDestroy,
	      DamageReportLevel damageLevel,
	      Bool		isInternal,
	      ScreenPtr		pScreen,
	      void *		closure);

void
DamageDrawInternal (ScreenPtr pScreen, Bool enable);

void
DamageRegister (DrawablePtr	pDrawable,
		DamagePtr	pDamage);

void
DamageUnregister (DrawablePtr	pDrawable,
		  DamagePtr	pDamage);

void
DamageDestroy (DamagePtr pDamage);

Bool
DamageSubtract (DamagePtr	    pDamage,
		const RegionPtr	    pRegion);

void
DamageEmpty (DamagePtr pDamage);

RegionPtr
DamageRegion (DamagePtr		    pDamage);

void
DamageDamageRegion (DrawablePtr	    pDrawable,
		    const RegionPtr pRegion);

void
DamageSetReportAfterOp (DamagePtr pDamage, Bool reportAfter);

#endif /* _DAMAGE_H_ */
