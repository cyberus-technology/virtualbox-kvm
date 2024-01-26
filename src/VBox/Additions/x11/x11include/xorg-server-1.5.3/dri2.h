/*
 * Copyright © 2007 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Soft-
 * ware"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, provided that the above copyright
 * notice(s) and this permission notice appear in all copies of the Soft-
 * ware and that both the above copyright notice(s) and this permission
 * notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
 * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
 * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
 * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
 * MANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization of
 * the copyright holder.
 *
 * Authors:
 *   Kristian Høgsberg (krh@redhat.com)
 */

#ifndef _DRI2_H_
#define _DRI2_H_

typedef unsigned int	(*DRI2GetPixmapHandleProcPtr)(PixmapPtr p,
						      unsigned int *flags);
typedef void		(*DRI2BeginClipNotifyProcPtr)(ScreenPtr pScreen);
typedef void		(*DRI2EndClipNotifyProcPtr)(ScreenPtr pScreen);

typedef struct {
    unsigned int version;	/* Version of this struct */
    int fd;
    size_t driverSareaSize;
    const char *driverName;
    DRI2GetPixmapHandleProcPtr getPixmapHandle;
    DRI2BeginClipNotifyProcPtr beginClipNotify;
    DRI2EndClipNotifyProcPtr endClipNotify;
}  DRI2InfoRec, *DRI2InfoPtr;

void *DRI2ScreenInit(ScreenPtr	pScreen,
		     DRI2InfoPtr info);

void DRI2CloseScreen(ScreenPtr pScreen);

Bool DRI2Connect(ScreenPtr pScreen,
		 int *fd,
		 const char **driverName,
		 unsigned int *sareaHandle);

Bool DRI2AuthConnection(ScreenPtr pScreen, drm_magic_t magic);

unsigned int DRI2GetPixmapHandle(PixmapPtr pPixmap,
				 unsigned int *flags);

void DRI2Lock(ScreenPtr pScreen);
void DRI2Unlock(ScreenPtr pScreen);

Bool DRI2CreateDrawable(DrawablePtr pDraw,
			unsigned int *handle,
			unsigned int *head);

void DRI2DestroyDrawable(DrawablePtr pDraw);

void DRI2ReemitDrawableInfo(DrawablePtr pDraw,
			    unsigned int *head);

Bool DRI2PostDamage(DrawablePtr pDrawable,
		    struct drm_clip_rect *rects, int numRects);

#endif
