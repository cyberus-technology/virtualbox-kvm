/* $Id: UIMachineViewScale.h $ */
/** @file
 * VBox Qt GUI - UIMachineViewScale class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_scale_UIMachineViewScale_h
#define FEQT_INCLUDED_SRC_runtime_scale_UIMachineViewScale_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIMachineView.h"

/** UIMachineView subclass used as scaled machine view implementation. */
class UIMachineViewScale : public UIMachineView
{
    Q_OBJECT;

public:

    /* Scale machine-view constructor: */
    UIMachineViewScale(UIMachineWindow *pMachineWindow, ulong uScreenId);
    /* Scale machine-view destructor: */
    virtual ~UIMachineViewScale() {}

private slots:

    /* Slot to perform guest resize: */
    void sltPerformGuestScale();

private:

    /* Event handlers: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /** Applies machine-view scale-factor. */
    void applyMachineViewScaleFactor();

    /** Resends guest size-hint. */
    void resendSizeHint();

    /* Private helpers: */
    QSize sizeHint() const;
    QRect workingArea() const;
    QSize calculateMaxGuestSize() const;
    void updateSliders();
};

#endif /* !FEQT_INCLUDED_SRC_runtime_scale_UIMachineViewScale_h */
