/* $Id: UIFileManagerOptionsPanel.cpp $ */
/** @file
 * VBox Qt GUI - UIVMLogViewer class implementation.
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

/* Qt includes: */
#include <QHBoxLayout>
#include <QCheckBox>

/* GUI includes: */
#include "QIToolButton.h"
#include "UIFileManager.h"
#include "UIFileManagerOptionsPanel.h"

UIFileManagerOptionsPanel::UIFileManagerOptionsPanel(QWidget *pParent, UIFileManagerOptions *pFileManagerOptions)
    : UIDialogPanel(pParent)
    , m_pListDirectoriesOnTopCheckBox(0)
    , m_pDeleteConfirmationCheckBox(0)
    , m_pHumanReabableSizesCheckBox(0)
    , m_pShowHiddenObjectsCheckBox(0)
    , m_pFileManagerOptions(pFileManagerOptions)
{
    prepare();
}

QString UIFileManagerOptionsPanel::panelName() const
{
    return "OptionsPanel";
}

void UIFileManagerOptionsPanel::update()
{
    if (!m_pFileManagerOptions)
        return;

    if (m_pListDirectoriesOnTopCheckBox)
    {
        m_pListDirectoriesOnTopCheckBox->blockSignals(true);
        m_pListDirectoriesOnTopCheckBox->setChecked(m_pFileManagerOptions->fListDirectoriesOnTop);
        m_pListDirectoriesOnTopCheckBox->blockSignals(false);
    }

    if (m_pDeleteConfirmationCheckBox)
    {
        m_pDeleteConfirmationCheckBox->blockSignals(true);
        m_pDeleteConfirmationCheckBox->setChecked(m_pFileManagerOptions->fAskDeleteConfirmation);
        m_pDeleteConfirmationCheckBox->blockSignals(false);
    }

    if (m_pHumanReabableSizesCheckBox)
    {
        m_pHumanReabableSizesCheckBox->blockSignals(true);
        m_pHumanReabableSizesCheckBox->setChecked(m_pFileManagerOptions->fShowHumanReadableSizes);
        m_pHumanReabableSizesCheckBox->blockSignals(false);
    }

    if (m_pShowHiddenObjectsCheckBox)
    {
        m_pShowHiddenObjectsCheckBox->blockSignals(true);
        m_pShowHiddenObjectsCheckBox->setChecked(m_pFileManagerOptions->fShowHiddenObjects);
        m_pShowHiddenObjectsCheckBox->blockSignals(false);
    }
}

void UIFileManagerOptionsPanel::prepareWidgets()
{
    if (!mainLayout())
        return;

    m_pListDirectoriesOnTopCheckBox = new QCheckBox;
    if (m_pListDirectoriesOnTopCheckBox)
        mainLayout()->addWidget(m_pListDirectoriesOnTopCheckBox, 0, Qt::AlignLeft);

    m_pDeleteConfirmationCheckBox = new QCheckBox;
    if (m_pDeleteConfirmationCheckBox)
        mainLayout()->addWidget(m_pDeleteConfirmationCheckBox, 0, Qt::AlignLeft);

    m_pHumanReabableSizesCheckBox = new QCheckBox;
    if (m_pHumanReabableSizesCheckBox)
        mainLayout()->addWidget(m_pHumanReabableSizesCheckBox, 0, Qt::AlignLeft);

    m_pShowHiddenObjectsCheckBox = new QCheckBox;
    if (m_pShowHiddenObjectsCheckBox)
        mainLayout()->addWidget(m_pShowHiddenObjectsCheckBox, 0, Qt::AlignLeft);

    /* Set initial checkbox status wrt. options: */
    if (m_pFileManagerOptions)
    {
        if (m_pListDirectoriesOnTopCheckBox)
            m_pListDirectoriesOnTopCheckBox->setChecked(m_pFileManagerOptions->fListDirectoriesOnTop);
        if (m_pDeleteConfirmationCheckBox)
            m_pDeleteConfirmationCheckBox->setChecked(m_pFileManagerOptions->fAskDeleteConfirmation);
        if (m_pHumanReabableSizesCheckBox)
            m_pHumanReabableSizesCheckBox->setChecked(m_pFileManagerOptions->fShowHumanReadableSizes);
        if (m_pShowHiddenObjectsCheckBox)
            m_pShowHiddenObjectsCheckBox->setChecked(m_pFileManagerOptions->fShowHiddenObjects);

    }
    retranslateUi();
    mainLayout()->addStretch(2);
}

void UIFileManagerOptionsPanel::sltListDirectoryCheckBoxToogled(bool bChecked)
{
    if (!m_pFileManagerOptions)
        return;
    m_pFileManagerOptions->fListDirectoriesOnTop = bChecked;
    emit sigOptionsChanged();
}

void UIFileManagerOptionsPanel::sltDeleteConfirmationCheckBoxToogled(bool bChecked)
{
    if (!m_pFileManagerOptions)
        return;
    m_pFileManagerOptions->fAskDeleteConfirmation = bChecked;
    emit sigOptionsChanged();
}

void UIFileManagerOptionsPanel::sltHumanReabableSizesCheckBoxToogled(bool bChecked)
{
    if (!m_pFileManagerOptions)
        return;
    m_pFileManagerOptions->fShowHumanReadableSizes = bChecked;
    emit sigOptionsChanged();
}

void UIFileManagerOptionsPanel::sltShowHiddenObjectsCheckBoxToggled(bool bChecked)
{
    if (!m_pFileManagerOptions)
        return;
    m_pFileManagerOptions->fShowHiddenObjects = bChecked;
    emit sigOptionsChanged();
}

void UIFileManagerOptionsPanel::prepareConnections()
{
    if (m_pListDirectoriesOnTopCheckBox)
        connect(m_pListDirectoriesOnTopCheckBox, &QCheckBox::toggled,
                this, &UIFileManagerOptionsPanel::sltListDirectoryCheckBoxToogled);
    if (m_pDeleteConfirmationCheckBox)
        connect(m_pDeleteConfirmationCheckBox, &QCheckBox::toggled,
                this, &UIFileManagerOptionsPanel::sltDeleteConfirmationCheckBoxToogled);
    if (m_pHumanReabableSizesCheckBox)
        connect(m_pHumanReabableSizesCheckBox, &QCheckBox::toggled,
                this, &UIFileManagerOptionsPanel::sltHumanReabableSizesCheckBoxToogled);

    if (m_pShowHiddenObjectsCheckBox)
        connect(m_pShowHiddenObjectsCheckBox, &QCheckBox::toggled,
                this, &UIFileManagerOptionsPanel::sltShowHiddenObjectsCheckBoxToggled);
}

void UIFileManagerOptionsPanel::retranslateUi()
{
    UIDialogPanel::retranslateUi();
    if (m_pListDirectoriesOnTopCheckBox)
    {
        m_pListDirectoriesOnTopCheckBox->setText(UIFileManager::tr("List directories on top"));
        m_pListDirectoriesOnTopCheckBox->setToolTip(UIFileManager::tr("List directories before files"));
    }

    if (m_pDeleteConfirmationCheckBox)
    {
        m_pDeleteConfirmationCheckBox->setText(UIFileManager::tr("Ask before delete"));
        m_pDeleteConfirmationCheckBox->setToolTip(UIFileManager::tr("Show a confirmation dialog "
                                                                                "before deleting files and directories"));
    }

    if (m_pHumanReabableSizesCheckBox)
    {
        m_pHumanReabableSizesCheckBox->setText(UIFileManager::tr("Human readable sizes"));
        m_pHumanReabableSizesCheckBox->setToolTip(UIFileManager::tr("Show file/directory sizes in human "
                                                                                "readable format rather than in bytes"));
    }

    if (m_pShowHiddenObjectsCheckBox)
    {
        m_pShowHiddenObjectsCheckBox->setText(UIFileManager::tr("Show hidden objects"));
        m_pShowHiddenObjectsCheckBox->setToolTip(UIFileManager::tr("Show hidden files/directories"));
    }
}
