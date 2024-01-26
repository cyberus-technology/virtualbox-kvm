/* $Id: UIMachineViewSeamless.h $ */
/** @file
 * VBox Qt GUI - UIMachineViewSeamless class declaration.
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_runtime_seamless_UIMachineViewSeamless_h
#define FEQT_INCLUDED_SRC_runtime_seamless_UIMachineViewSeamless_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIMachineView.h"

/** UIMachineView subclass used as seamless machine view implementation. */
class UIMachineViewSeamless : public UIMachineView
{
    Q_OBJECT;

public:

    /* Seamless machine-view constructor: */
    UIMachineViewSeamless(UIMachineWindow *pMachineWindow, ulong uScreenId);
    /* Seamless machine-view destructor: */
    virtual ~UIMachineViewSeamless() { cleanupSeamless(); }

private slots:

    /* Handler: Console callback stuff: */
    void sltAdditionsStateChanged();

    /* Handler: Frame-buffer SetVisibleRegion stuff: */
    virtual void sltHandleSetVisibleRegion(QRegion region);

private:

    /* Event handlers: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /* Prepare helpers: */
    void prepareCommon();
    void prepareFilters();
    void prepareConsoleConnections();
    void prepareSeamless();

    /* Cleanup helpers: */
    void cleanupSeamless();
    //void cleanupConsoleConnections() {}
    //void cleanupFilters() {}
    //void cleanupCommon() {}

    /** Adjusts guest-screen size to correspond current <i>working area</i> size. */
    void adjustGuestScreenSize();

    /* Helpers: Geometry stuff: */
    QRect workingArea() const;
    QSize calculateMaxGuestSize() const;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_seamless_UIMachineViewSeamless_h */
