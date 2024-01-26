/* $Id: UIGuestOSTypeSelectionButton.h $ */
/** @file
 * VBox Qt GUI - UIGuestOSTypeSelectionButton class declaration.
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

#ifndef FEQT_INCLUDED_SRC_widgets_UIGuestOSTypeSelectionButton_h
#define FEQT_INCLUDED_SRC_widgets_UIGuestOSTypeSelectionButton_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QPushButton>

/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QMenu;
class QSignalMapper;

/** QPushButton sub-class for choosing guest OS family/type inside appliance editor widget. */
class UIGuestOSTypeSelectionButton : public QIWithRetranslateUI<QPushButton>
{
    Q_OBJECT;

public:

    /** Constructs a button passing @a pParent to the base-class. */
    UIGuestOSTypeSelectionButton(QWidget *pParent);

    /** Returns whether the menu is shown. */
    bool isMenuShown() const;

    /** Returns current guest OS type ID. */
    QString osTypeId() const { return m_strOSTypeId; }

public slots:

    /** Defines current guest @a strOSTypeId. */
    void setOSTypeId(const QString &strOSTypeId);

protected:

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;

private:

    /** Populates menu. */
    void populateMenu();

    /** Holds the current guest OS type ID. */
    QString  m_strOSTypeId;

    /** Holds the menu instance. */
    QMenu         *m_pMainMenu;
    /** Holds the signal mapper instance. */
    QSignalMapper *m_pSignalMapper;
};

#endif /* !FEQT_INCLUDED_SRC_widgets_UIGuestOSTypeSelectionButton_h */
