/** @file
 * VBox Remote Desktop Extension (VRDE) - Image updates interface.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef VBOX_INCLUDED_RemoteDesktop_VRDEImage_h
#define VBOX_INCLUDED_RemoteDesktop_VRDEImage_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/RemoteDesktop/VRDE.h>

/*
 * Generic interface for external image updates with a clipping region to be sent
 * to the client.
 *
 * Async callbacks are used for reporting errors, providing feedback, etc.
 */

#define VRDE_IMAGE_INTERFACE_NAME "IMAGE"

#ifdef __cplusplus
class VRDEImage;
typedef class VRDEImage *HVRDEIMAGE;
#else
struct VRDEImage;
typedef struct VRDEImage *HVRDEIMAGE;
#endif /* __cplusplus */

/*
 * Format description structures for VRDEImageHandleCreate.
 */
typedef struct VRDEIMAGEFORMATBITMAP
{
    uint32_t u32BytesPerPixel; /** @todo impl */
} VRDEIMAGEFORMATBITMAP;

typedef struct VRDEIMAGEBITMAP
{
    uint32_t    cWidth;      /* The width of the bitmap in pixels. */
    uint32_t    cHeight;     /* The height of the bitmap in pixels. */
    const void *pvData;      /* Address of pixel buffer. */
    uint32_t    cbData;      /* Size of pixel buffer. */
    const void *pvScanLine0; /* Address of first scanline. */
    int32_t     iScanDelta;  /* Difference between two scanlines. */
} VRDEIMAGEBITMAP;

/*
 * Image update handle creation flags.
 */
#define VRDE_IMAGE_F_CREATE_DEFAULT       0x00000000
#define VRDE_IMAGE_F_CREATE_CONTENT_3D    0x00000001 /* Input image data is a rendered 3d scene. */
#define VRDE_IMAGE_F_CREATE_CONTENT_VIDEO 0x00000002 /* Input image data is a sequence of video frames. */
#define VRDE_IMAGE_F_CREATE_WINDOW        0x00000004 /* pRect parameter is the image update area. */

/*
 * Completion flags for image update handle creation.
 */
#define VRDE_IMAGE_F_COMPLETE_DEFAULT 0x00000000 /* The handle has been created. */
#define VRDE_IMAGE_F_COMPLETE_ASYNC   0x00000001 /* The server will call VRDEImageCbNotify when the handle is ready. */

/*
 * Supported input image formats.
 *
 * The identifiers are arbitrary and new formats can be introduced later.
 *
 */
#define VRDE_IMAGE_FMT_ID_BITMAP_BGRA8 "BITMAP_BGRA8.07e46a64-e93e-41d4-a845-204094f5ccf1"

/** The VRDE server external image updates interface entry points. Interface version 1. */
typedef struct VRDEIMAGEINTERFACE
{
    /** The header. */
    VRDEINTERFACEHDR header;

    /** Create image updates handle.
     *
     * The server can setup a context which will speed up further updates.
     *
     * A failure is returned if the server either does not support requested updates
     * or it failed to create a handle.
     *
     * A success means that the server was able to create an internal context for
     * the updates.
     *
     * @param hServer     The server instance handle.
     * @param phImage     The returned image updates handle.
     * @param pvUser      The caller context of the call.
     * @param u32ScreenId Updates are for this screen in a multimonitor config.
     * @param fu32Flags   VRDE_IMAGE_F_CREATE_* flags, which describe input data.
     * @param pRect       If VRDE_IMAGE_F_CREATE_WINDOW is set, this is the area of expected updates.
     *                    Otherwise the entire screen will be used for updates.
     * @param pvFormat    Format specific data.
     * @param cbFormat    Size of format specific data.
     * @param *pfu32CompletionFlags VRDE_IMAGE_F_COMPLETE_* flags. Async handle creation, etc.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDEImageHandleCreate, (HVRDESERVER hServer,
                                                      HVRDEIMAGE *phImage,
                                                      void *pvUser,
                                                      uint32_t u32ScreenId,
                                                      uint32_t fu32Flags,
                                                      const RTRECT *pRect,
                                                      const char *pszFormatId,
                                                      const void *pvFormat,
                                                      uint32_t cbFormat,
                                                      uint32_t *pfu32CompletionFlags));

    /** Create image updates handle.
     *
     * @param hImage The image updates handle, which the caller will not use anymore.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(void, VRDEImageHandleClose, (HVRDEIMAGE hImage));

    /** Set a clipping region for a particular screen.
     *
     * @param hImage    The image updates handle.
     * @param cRects    How many rectangles. 0 clears region for this screen.
     * @param paRects   Rectangles in the screen coordinates.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDEImageRegionSet, (HVRDEIMAGE hImage,
                                                   uint32_t cRects,
                                                   const RTRECT *paRects));

    /** Set the new position of the update area. Only works if the image handle
     * has been created with VRDE_IMAGE_F_CREATE_WINDOW.
     *
     * @param hImage    The image updates handle.
     * @param pRect     New area rectangle in the screen coordinates.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDEImageGeometrySet, (HVRDEIMAGE hImage,
                                                     const RTRECT *pRect));

    /** Set a configuration parameter.
     *
     * @param hImage      The image updates handle.
     * @param pszName     The parameter name.
     * @param pszValue    The parameter value.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDEImagePropertySet, (HVRDEIMAGE hImage,
                                                     const char *pszName,
                                                     const char *pszValue));

    /** Query a configuration parameter.
     *
     * @param hImage      The image updates handle.
     * @param pszName     The parameter name.
     * @param pszValue    The parameter value.
     * @param cbValueIn   The size of pszValue buffer.
     * @param pcbValueOut The length of data returned in pszValue buffer.
     *
     * Properties names:
     * "ID" - an unique string for this hImage.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDEImagePropertyQuery, (HVRDEIMAGE hImage,
                                                       const char *pszName,
                                                       char *pszValue,
                                                       uint32_t cbValueIn,
                                                       uint32_t *pcbValueOut));

    /** Data for an image update.
     *
     * @param hImage      The image updates handle.
     * @param i32TargetX  Target x.
     * @param i32TargetY  Target y.
     * @param i32TargetW  Target width.
     * @param i32TargetH  Target height.
     * @param pvImageData Format specific image data (for example VRDEIMAGEBITMAP).
     * @param cbImageData Size of format specific image data.
     */
    DECLR3CALLBACKMEMBER(void, VRDEImageUpdate, (HVRDEIMAGE hImage,
                                                 int32_t i32TargetX,
                                                 int32_t i32TargetY,
                                                 uint32_t u32TargetW,
                                                 uint32_t u32TargetH,
                                                 const void *pvImageData,
                                                 uint32_t cbImageData));
} VRDEIMAGEINTERFACE;

/*
 * Notifications.
 * u32Id parameter of VRDEIMAGECALLBACKS::VRDEImageCbNotify.
 */
#define VRDE_IMAGE_NOTIFY_HANDLE_CREATE 1 /* Async result of VRDEImageHandleCreate.
                                           * pvData: uint32_t = 0 if stream was not created,
                                           * a non zero value otherwise.
                                           */

typedef struct VRDEIMAGECALLBACKS
{
    /** The header. */
    VRDEINTERFACEHDR header;

    /** Generic notification callback.
     *
     * @param hServer   The server instance handle.
     * @param pvContext The callbacks context specified in VRDEGetInterface.
     * @param pvUser    The pvUser parameter of VRDEImageHandleCreate.
     * @param hImage    The handle, same as returned by VRDEImageHandleCreate.
     * @param u32Id     The notification identifier: VRDE_IMAGE_NOTIFY_*.
     * @param pvData    The callback specific data.
     * @param cbData    The size of buffer pointed by pvData.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDEImageCbNotify,(void *pvContext,
                                                 void *pvUser,
                                                 HVRDEIMAGE hVideo,
                                                 uint32_t u32Id,
                                                 void *pvData,
                                                 uint32_t cbData));
} VRDEIMAGECALLBACKS;

#endif /* !VBOX_INCLUDED_RemoteDesktop_VRDEImage_h */
