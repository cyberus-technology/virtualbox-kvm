/* $Id: UIVisoCreatorOptionsPanel.h $ */
/** @file
 * VBox Qt GUI - UIVisoCreatorOptionsPanel class declaration.
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

#ifndef FEQT_INCLUDED_SRC_medium_viso_UIVisoCreatorOptionsPanel_h
#define FEQT_INCLUDED_SRC_medium_viso_UIVisoCreatorOptionsPanel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Forward declarations: */
class QCheckBox;
class QILabel;

/* GUI includes: */
#include "UIDialogPanel.h"

class UIVisoCreatorOptionsPanel : public UIDialogPanel
{
    Q_OBJECT;

signals:

    void sigShowHiddenObjects(bool fShow);

public:

    UIVisoCreatorOptionsPanel(QWidget *pParent = 0);
    ~UIVisoCreatorOptionsPanel();
    virtual QString panelName() const RT_OVERRIDE;
    void setShowHiddenbjects(bool fShow);

protected:

    void retranslateUi() RT_OVERRIDE;


private slots:

    void sltHandlShowHiddenObjectsChange(int iState);

private:

    void prepareObjects();
    void prepareConnections();

    QCheckBox *m_pShowHiddenObjectsCheckBox;
    QILabel *m_pShowHiddenObjectsLabel;
};

#endif /* !FEQT_INCLUDED_SRC_medium_viso_UIVisoCreatorOptionsPanel_h */
