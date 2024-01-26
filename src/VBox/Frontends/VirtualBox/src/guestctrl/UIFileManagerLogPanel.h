/* $Id: UIFileManagerLogPanel.h $ */
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

#ifndef FEQT_INCLUDED_SRC_guestctrl_UIFileManagerLogPanel_h
#define FEQT_INCLUDED_SRC_guestctrl_UIFileManagerLogPanel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIGuestControlDefs.h"
#include "UIDialogPanel.h"

/* Forward declarations: */
class QTextEdit;
class UIFileManager;

/** UIDialogPanel extension to display file manager logs. */
class UIFileManagerLogPanel : public UIDialogPanel
{
    Q_OBJECT;

public:

    UIFileManagerLogPanel(QWidget *pParent = 0);
    void appendLog(const QString &str, const QString &strMachineName, FileManagerLogType eLogType);
    virtual QString panelName() const RT_OVERRIDE;

protected:

    virtual void prepareWidgets() RT_OVERRIDE;
    virtual void prepareConnections() RT_OVERRIDE;

    /** Handles the translation event. */
    void retranslateUi();

private slots:


private:

    QTextEdit *m_pLogTextEdit;
};

#endif /* !FEQT_INCLUDED_SRC_guestctrl_UIFileManagerLogPanel_h */
