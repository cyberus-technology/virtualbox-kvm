/* $XFree86: xc/programs/Xserver/include/bstorestr.h,v 1.2 2001/01/06 20:58:12 tsi Exp $*/
/*
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  The University of
 * California makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 */

/*
 * Moved here from mi to allow wrapping of lower level backing store functions.
 * -- 1997.10.27  Marc Aurele La France (tsi@xfree86.org)
 */

#ifndef _BSTORESTR_H_
#define _BSTORESTR_H_

#include "gc.h"
#include "pixmap.h"
#include "region.h"
#include "window.h"

typedef    void (* BackingStoreSaveAreasProcPtr)(
#if NeedNestedPrototypes
	PixmapPtr /*pBackingPixmap*/,
	RegionPtr /*pObscured*/,
	int /*x*/,
	int /*y*/,
	WindowPtr /*pWin*/
#endif
);

typedef    void (* BackingStoreRestoreAreasProcPtr)(
#if NeedNestedPrototypes
	PixmapPtr /*pBackingPixmap*/,
	RegionPtr /*pExposed*/,
	int /*x*/,
	int /*y*/,
	WindowPtr /*pWin*/
#endif
);

typedef    void (* BackingStoreSetClipmaskRgnProcPtr)(
#if NeedNestedPrototypes
	GCPtr /*pBackingGC*/,
	RegionPtr /*pbackingCompositeClip*/
#endif
);

typedef    PixmapPtr (* BackingStoreGetImagePixmapProcPtr)(	/* unused */
#if NeedNestedPrototypes
	void
#endif
);

typedef    PixmapPtr (* BackingStoreGetSpansPixmapProcPtr)(	/* unused */
#if NeedNestedPrototypes
	void
#endif
);

typedef struct _BSFuncs {

	BackingStoreSaveAreasProcPtr SaveAreas;
	BackingStoreRestoreAreasProcPtr RestoreAreas;
	BackingStoreSetClipmaskRgnProcPtr SetClipmaskRgn;
	BackingStoreGetImagePixmapProcPtr GetImagePixmap;
	BackingStoreGetSpansPixmapProcPtr GetSpansPixmap;

} BSFuncRec, *BSFuncPtr;

#endif /* _BSTORESTR_H_ */
