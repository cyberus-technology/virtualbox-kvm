/* $Id: UIDialogPanel.h $ */
/** @file
 * VBox Qt GUI - UIVMLogViewer class declaration.
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

#ifndef FEQT_INCLUDED_SRC_globals_UIDialogPanel_h
#define FEQT_INCLUDED_SRC_globals_UIDialogPanel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>
#include <QKeySequence>
/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QHBoxLayout;
class QIToolButton;


/** QWidget extension acting as the base class for all the dialog panels like file manager, logviewer etc. */
class SHARED_LIBRARY_STUFF UIDialogPanel : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    UIDialogPanel(QWidget *pParent = 0);
    void setCloseButtonShortCut(QKeySequence shortCut);
    virtual QString panelName() const = 0;

signals:

    void sigHidePanel(UIDialogPanel *pPanel);
    void sigShowPanel(UIDialogPanel *pPanel);

protected:

    virtual void prepare();
    virtual void prepareWidgets();
    virtual void prepareConnections();

    /* Access functions for children classes. */
    QHBoxLayout*               mainLayout();

    /** Handles the translation event. */
    void retranslateUi() RT_OVERRIDE;

    /** Handles the Qt show @a pEvent. */
    void showEvent(QShowEvent *pEvent) RT_OVERRIDE;
    /** Handles the Qt hide @a pEvent. */
    void hideEvent(QHideEvent *pEvent) RT_OVERRIDE;
    void addVerticalSeparator();

private:

    QHBoxLayout   *m_pMainLayout;
    QIToolButton  *m_pCloseButton;
};

#endif /* !FEQT_INCLUDED_SRC_globals_UIDialogPanel_h */
