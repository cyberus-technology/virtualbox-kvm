/*
 * $Id: damageextint.h $
 *
 * Copyright © 2002 Keith Packard
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

#ifndef _DAMAGEEXTINT_H_
#define _DAMAGEEXTINT_H_

#define NEED_EVENTS
#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include <X11/extensions/damageproto.h>
#include "windowstr.h"
#include "selection.h"
#include "scrnintstr.h"
#include "damageext.h"
#include "damage.h" 
#include "xfixes.h"

extern unsigned char	DamageReqCode;
extern int		DamageEventBase;
extern int		DamageErrorBase;
extern int		DamageClientPrivateIndex;
extern RESTYPE		DamageExtType;
extern RESTYPE		DamageExtWinType;

typedef struct _DamageClient {
    CARD32	major_version;
    CARD32	minor_version;
    int		critical;
} DamageClientRec, *DamageClientPtr;

#define GetDamageClient(pClient)    ((DamageClientPtr) (pClient)->devPrivates[DamageClientPrivateIndex].ptr)

typedef struct _DamageExt {
    DamagePtr		pDamage;
    DrawablePtr		pDrawable;
    DamageReportLevel	level;
    ClientPtr		pClient;
    XID			id;
} DamageExtRec, *DamageExtPtr;

extern int	(*ProcDamageVector[/*XDamageNumberRequests*/])(ClientPtr);
extern int	(*SProcDamageVector[/*XDamageNumberRequests*/])(ClientPtr);

#define VERIFY_DAMAGEEXT(pDamageExt, rid, client, mode) { \
    pDamageExt = SecurityLookupIDByType (client, rid, DamageExtType, mode); \
    if (!pDamageExt) { \
	client->errorValue = rid; \
	return DamageErrorBase + BadDamage; \
    } \
}

void
SDamageNotifyEvent (xDamageNotifyEvent *from,
		    xDamageNotifyEvent *to);

void
DamageExtSetCritical (ClientPtr pClient, Bool critical);

#endif /* _DAMAGEEXTINT_H_ */
