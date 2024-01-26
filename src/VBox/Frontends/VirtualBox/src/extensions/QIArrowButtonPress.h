/* $Id: QIArrowButtonPress.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIArrowButtonPress class declaration.
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

#ifndef FEQT_INCLUDED_SRC_extensions_QIArrowButtonPress_h
#define FEQT_INCLUDED_SRC_extensions_QIArrowButtonPress_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "QIRichToolButton.h"
#include "QIWithRetranslateUI.h"
#include "UILibraryDefs.h"

/** QIRichToolButton extension
  * representing arrow tool-button with text-label,
  * can be used as back/next buttons in various places. */
class SHARED_LIBRARY_STUFF QIArrowButtonPress : public QIWithRetranslateUI<QIRichToolButton>
{
    Q_OBJECT;

public:

    /** Button types. */
    enum ButtonType { ButtonType_Back, ButtonType_Next };

    /** Constructs button passing @a pParent to the base-class.
      * @param  enmButtonType  Brings which type of the button it is. */
    QIArrowButtonPress(ButtonType enmButtonType, QWidget *pParent = 0);

protected:

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;

    /** Handles key-press @a pEvent. */
    virtual void keyPressEvent(QKeyEvent *pEvent) RT_OVERRIDE;

private:

    /** Holds the button-type. */
    ButtonType m_enmButtonType;
};

#endif /* !FEQT_INCLUDED_SRC_extensions_QIArrowButtonPress_h */
