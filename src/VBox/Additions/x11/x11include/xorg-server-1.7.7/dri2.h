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

#include <X11/extensions/dri2tokens.h>

/* Version 2 structure (with format at the end) */
typedef struct {
    unsigned int attachment;
    unsigned int name;
    unsigned int pitch;
    unsigned int cpp;
    unsigned int flags;
    unsigned int format;
    void *driverPrivate;
} DRI2BufferRec, *DRI2BufferPtr;

typedef DRI2BufferRec DRI2Buffer2Rec, *DRI2Buffer2Ptr;

typedef DRI2BufferPtr	(*DRI2CreateBuffersProcPtr)(DrawablePtr pDraw,
						    unsigned int *attachments,
						    int count);
typedef void		(*DRI2DestroyBuffersProcPtr)(DrawablePtr pDraw,
						     DRI2BufferPtr buffers,
						     int count);
typedef void		(*DRI2CopyRegionProcPtr)(DrawablePtr pDraw,
						 RegionPtr pRegion,
						 DRI2BufferPtr pDestBuffer,
						 DRI2BufferPtr pSrcBuffer);

typedef void		(*DRI2WaitProcPtr)(WindowPtr pWin,
					   unsigned int sequence);

typedef DRI2BufferPtr	(*DRI2CreateBufferProcPtr)(DrawablePtr pDraw,
						   unsigned int attachment,
						   unsigned int format);
typedef void		(*DRI2DestroyBufferProcPtr)(DrawablePtr pDraw,
						    DRI2BufferPtr buffer);

/**
 * Version of the DRI2InfoRec structure defined in this header
 */
#define DRI2INFOREC_VERSION 3

typedef struct {
    unsigned int version;	/**< Version of this struct */
    int fd;
    const char *driverName;
    const char *deviceName;

    DRI2CreateBufferProcPtr	CreateBuffer;
    DRI2DestroyBufferProcPtr	DestroyBuffer;
    DRI2CopyRegionProcPtr	CopyRegion;
    DRI2WaitProcPtr		Wait;

}  DRI2InfoRec, *DRI2InfoPtr;

extern _X_EXPORT Bool DRI2ScreenInit(ScreenPtr	pScreen,
		    DRI2InfoPtr info);

extern _X_EXPORT void DRI2CloseScreen(ScreenPtr pScreen);

extern _X_EXPORT Bool DRI2Connect(ScreenPtr pScreen,
		 unsigned int driverType,
		 int *fd,
		 const char **driverName,
		 const char **deviceName);

extern _X_EXPORT Bool DRI2Authenticate(ScreenPtr pScreen, drm_magic_t magic);

extern _X_EXPORT int DRI2CreateDrawable(DrawablePtr pDraw);

extern _X_EXPORT void DRI2DestroyDrawable(DrawablePtr pDraw);

extern _X_EXPORT DRI2BufferPtr *DRI2GetBuffers(DrawablePtr pDraw,
			     int *width,
			     int *height,
			     unsigned int *attachments,
			     int count,
			     int *out_count);

extern _X_EXPORT int DRI2CopyRegion(DrawablePtr pDraw,
		   RegionPtr pRegion,
		   unsigned int dest,
		   unsigned int src);

/**
 * Determine the major and minor version of the DRI2 extension.
 *
 * Provides a mechanism to other modules (e.g., 2D drivers) to determine the
 * version of the DRI2 extension.  While it is possible to peek directly at
 * the \c XF86ModuleData from a layered module, such a module will fail to
 * load (due to an unresolved symbol) if the DRI2 extension is not loaded.
 *
 * \param major  Location to store the major verion of the DRI2 extension
 * \param minor  Location to store the minor verion of the DRI2 extension
 *
 * \note
 * This interface was added some time after the initial release of the DRI2
 * module.  Layered modules that wish to use this interface must first test
 * its existance by calling \c xf86LoaderCheckSymbol.
 */
extern _X_EXPORT void DRI2Version(int *major, int *minor);

extern _X_EXPORT DRI2BufferPtr *DRI2GetBuffersWithFormat(DrawablePtr pDraw,
	int *width, int *height, unsigned int *attachments, int count,
	int *out_count);

#endif
