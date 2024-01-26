/* $Id: UIAbstractDockIconPreview.h $ */
/** @file
 * VBox Qt GUI - Abstract class for the dock icon preview.
 */

/*
 * Copyright (C) 2009-2023 Oracle and/or its affiliates.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef FEQT_INCLUDED_SRC_platform_darwin_UIAbstractDockIconPreview_h
#define FEQT_INCLUDED_SRC_platform_darwin_UIAbstractDockIconPreview_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* System includes */
#include <ApplicationServices/ApplicationServices.h>

/* VBox includes */
#include "VBoxUtils-darwin.h"

class UIFrameBuffer;
class UISession;

class QPixmap;

class UIAbstractDockIconPreview
{
public:
    UIAbstractDockIconPreview(UISession *pSession, const QPixmap& overlayImage);
    virtual ~UIAbstractDockIconPreview() {}

    virtual void updateDockOverlay() = 0;
    virtual void updateDockPreview(CGImageRef VMImage) = 0;
    virtual void updateDockPreview(UIFrameBuffer *pFrameBuffer);

    virtual void setOriginalSize(int /* aWidth */, int /* aHeight */) {}
};

class UIAbstractDockIconPreviewHelper
{
public:
    UIAbstractDockIconPreviewHelper(UISession *pSession, const QPixmap& overlayImage);
    virtual ~UIAbstractDockIconPreviewHelper();
    void initPreviewImages();
    void drawOverlayIcons(CGContextRef context);

    void* currentPreviewWindowId() const;

    /* Flipping is necessary cause the drawing context in Mac OS X is flipped by 180 degree */
    inline CGRect flipRect(CGRect rect) const { return ::darwinFlipCGRect(rect, m_dockIconRect); }
    inline CGRect centerRect(CGRect rect) const { return ::darwinCenterRectTo(rect, m_dockIconRect); }
    inline CGRect centerRectTo(CGRect rect, const CGRect& toRect) const { return ::darwinCenterRectTo(rect, toRect); }

    /* Private member vars */
    UISession *m_pSession;
    const CGRect m_dockIconRect;

    CGImageRef m_overlayImage;
    CGImageRef m_dockMonitor;
    CGImageRef m_dockMonitorGlossy;

    CGRect m_updateRect;
    CGRect m_monitorRect;
};

#endif /* !FEQT_INCLUDED_SRC_platform_darwin_UIAbstractDockIconPreview_h */

