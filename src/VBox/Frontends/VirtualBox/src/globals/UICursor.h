/* $Id: UICursor.h $ */
/** @file
 * VBox Qt GUI - UICursor namespace declaration.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef FEQT_INCLUDED_SRC_globals_UICursor_h
#define FEQT_INCLUDED_SRC_globals_UICursor_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QGraphicsWidget;
class QWidget;

/** QObject subclass containing common GUI functionality. */
namespace UICursor
{
    /** Does some checks on certain platforms before calling QWidget::setCursor(...). */
    SHARED_LIBRARY_STUFF void setCursor(QWidget *pWidget, const QCursor &cursor);
    /** Does some checks on certain platforms before calling QGraphicsWidget::setCursor(...). */
    SHARED_LIBRARY_STUFF void setCursor(QGraphicsWidget *pWidget, const QCursor &cursor);
    /** Does some checks on certain platforms before calling QWidget::unsetCursor(). */
    SHARED_LIBRARY_STUFF void unsetCursor(QWidget *pWidget);
    /** Does some checks on certain platforms before calling QGraphicsWidget::unsetCursor(). */
    SHARED_LIBRARY_STUFF void unsetCursor(QGraphicsWidget *pWidget);
}

#endif /* !FEQT_INCLUDED_SRC_globals_UICursor_h */
