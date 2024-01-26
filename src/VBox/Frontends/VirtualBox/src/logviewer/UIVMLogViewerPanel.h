/* $Id: UIVMLogViewerPanel.h $ */
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

#ifndef FEQT_INCLUDED_SRC_logviewer_UIVMLogViewerPanel_h
#define FEQT_INCLUDED_SRC_logviewer_UIVMLogViewerPanel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QKeySequence>

/* GUI includes: */
#include "UIDialogPanel.h"

/* Forward declarations: */
class QPlainTextEdit;
class QTextDocument;
class UIVMLogViewerWidget;


/** UIDialonPanel extension acting as the base class for UIVMLogViewerXXXPanel widgets. */
class UIVMLogViewerPanel : public UIDialogPanel
{
    Q_OBJECT;

public:

    UIVMLogViewerPanel(QWidget *pParent, UIVMLogViewerWidget *pViewer);

protected:

    virtual void retranslateUi() RT_OVERRIDE;

    /* Access functions for children classes. */
    UIVMLogViewerWidget        *viewer();
    const UIVMLogViewerWidget  *viewer() const;
    QTextDocument  *textDocument();
    QPlainTextEdit *textEdit();
    /* Return the unmodified log. */
    const QString *logString() const;

private:

    /** Holds the reference to VM Log-Viewer this panel belongs to. */
    UIVMLogViewerWidget *m_pViewer;
};

#endif /* !FEQT_INCLUDED_SRC_logviewer_UIVMLogViewerPanel_h */
