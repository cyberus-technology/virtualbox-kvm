/* $Id: QISplitter.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QISplitter class declaration.
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

#ifndef FEQT_INCLUDED_SRC_extensions_QISplitter_h
#define FEQT_INCLUDED_SRC_extensions_QISplitter_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QSplitter>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QSplitterHandle;

/** QSplitter subclass with extended functionality. */
class SHARED_LIBRARY_STUFF QISplitter : public QSplitter
{
    Q_OBJECT;

public:

    /** Handle types. */
    enum Type { Flat, Shade, Native };

    /** Constructs splitter passing @a pParent to the base-class. */
    QISplitter(QWidget *pParent = 0);
    /** Constructs splitter passing @a enmOrientation and @a pParent to the base-class.
      * @param  enmType  Brings the splitter handle type. */
    QISplitter(Qt::Orientation enmOrientation, Type enmType, QWidget *pParent = 0);

    /** Configure custom color defined as @a color. */
    void configureColor(const QColor &color);
    /** Configure custom colors defined as @a color1 and @a color2. */
    void configureColors(const QColor &color1, const QColor &color2);

protected:

    /** Preprocesses Qt @a pEvent for passed @a pObject. */
    virtual bool eventFilter(QObject *pObject, QEvent *pEvent) RT_OVERRIDE;

    /** Handles show @a pEvent. */
    void showEvent(QShowEvent *pEvent);

    /** Creates handle. */
    QSplitterHandle *createHandle();

private:

    /** Holds the serialized base-state. */
    QByteArray m_baseState;

    /** Holds the handle type. */
    Type m_enmType;

    /** Holds whether the splitter is polished. */
    bool m_fPolished : 1;
#ifdef VBOX_WS_MAC
    /** Holds whether handle is grabbed. */
    bool m_fHandleGrabbed : 1;
#endif

    /** Holds color. */
    QColor m_color;
    /** Holds color1. */
    QColor m_color1;
    /** Holds color2. */
    QColor m_color2;
};

#endif /* !FEQT_INCLUDED_SRC_extensions_QISplitter_h */
