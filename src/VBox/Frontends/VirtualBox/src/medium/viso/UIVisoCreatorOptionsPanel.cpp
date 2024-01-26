/* $Id: UIVisoCreatorOptionsPanel.cpp $ */
/** @file
 * VBox Qt GUI - UIVisoCreatorOptionsPanel class implementation.
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

/* Qt includes: */
#include <QCheckBox>
#include <QHBoxLayout>

/* GUI includes: */
#include "QILabel.h"
#include "UIVisoCreatorOptionsPanel.h"

UIVisoCreatorOptionsPanel::UIVisoCreatorOptionsPanel(QWidget *pParent /* =0 */)
    : UIDialogPanel(pParent)
    , m_pShowHiddenObjectsCheckBox(0)
    , m_pShowHiddenObjectsLabel(0)
{
    prepareObjects();
    prepareConnections();
}

UIVisoCreatorOptionsPanel::~UIVisoCreatorOptionsPanel()
{
}

QString UIVisoCreatorOptionsPanel::panelName() const
{
    return "OptionsPanel";
}

void UIVisoCreatorOptionsPanel::setShowHiddenbjects(bool fShow)
{
    if (m_pShowHiddenObjectsCheckBox)
        m_pShowHiddenObjectsCheckBox->setChecked(fShow);
}

void UIVisoCreatorOptionsPanel::retranslateUi()
{
    if (m_pShowHiddenObjectsLabel)
        m_pShowHiddenObjectsLabel->setText(QApplication::translate("UIVisoCreatorWidget", "Show Hidden Objects"));
    if (m_pShowHiddenObjectsCheckBox)
        m_pShowHiddenObjectsCheckBox->setToolTip(QApplication::translate("UIVisoCreatorWidget", "When checked, "
                                                                         "multiple hidden objects are shown in the file browser"));
}

void UIVisoCreatorOptionsPanel::sltHandlShowHiddenObjectsChange(int iState)
{
    if (iState == static_cast<int>(Qt::Checked))
        sigShowHiddenObjects(true);
    else
        sigShowHiddenObjects(false);
}

void UIVisoCreatorOptionsPanel::prepareObjects()
{
    if (!mainLayout())
        return;

    m_pShowHiddenObjectsCheckBox = new QCheckBox;
    m_pShowHiddenObjectsLabel = new QILabel(QApplication::translate("UIVisoCreatorWidget", "Show Hidden Objects"));
    m_pShowHiddenObjectsLabel->setBuddy(m_pShowHiddenObjectsCheckBox);
    mainLayout()->addWidget(m_pShowHiddenObjectsCheckBox, 0, Qt::AlignLeft);
    mainLayout()->addWidget(m_pShowHiddenObjectsLabel, 0, Qt::AlignLeft);
    mainLayout()->addStretch(6);
    connect(m_pShowHiddenObjectsCheckBox, &QCheckBox::stateChanged,
            this, &UIVisoCreatorOptionsPanel::sltHandlShowHiddenObjectsChange);
}

void UIVisoCreatorOptionsPanel::prepareConnections()
{
}
