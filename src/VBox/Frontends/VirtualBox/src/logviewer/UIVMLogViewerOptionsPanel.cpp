/* $Id: UIVMLogViewerOptionsPanel.cpp $ */
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
#include <QComboBox>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QFontDialog>
#include <QCheckBox>
#include <QLabel>
#include <QSpinBox>

/* GUI includes: */
#include "QIToolButton.h"
#include "UIIconPool.h"
#include "UIVMLogViewerOptionsPanel.h"
#include "UIVMLogViewerWidget.h"


UIVMLogViewerOptionsPanel::UIVMLogViewerOptionsPanel(QWidget *pParent, UIVMLogViewerWidget *pViewer)
    : UIVMLogViewerPanel(pParent, pViewer)
    , m_pLineNumberCheckBox(0)
    , m_pWrapLinesCheckBox(0)
    , m_pFontSizeSpinBox(0)
    , m_pFontSizeLabel(0)
    , m_pOpenFontDialogButton(0)
    , m_pResetToDefaultsButton(0)
    , m_iDefaultFontSize(9)
{
    prepare();
}

void UIVMLogViewerOptionsPanel::setShowLineNumbers(bool bShowLineNumbers)
{
    if (!m_pLineNumberCheckBox)
        return;
    if (m_pLineNumberCheckBox->isChecked() == bShowLineNumbers)
        return;
    m_pLineNumberCheckBox->setChecked(bShowLineNumbers);
}

void UIVMLogViewerOptionsPanel::setWrapLines(bool bWrapLines)
{
    if (!m_pWrapLinesCheckBox)
        return;
    if (m_pWrapLinesCheckBox->isChecked() == bWrapLines)
        return;
    m_pWrapLinesCheckBox->setChecked(bWrapLines);
}

void UIVMLogViewerOptionsPanel::setFontSizeInPoints(int fontSizeInPoints)
{
    if (!m_pFontSizeSpinBox)
        return;
    if (m_pFontSizeSpinBox->value() == fontSizeInPoints)
        return;
    m_pFontSizeSpinBox->setValue(fontSizeInPoints);
}

QString UIVMLogViewerOptionsPanel::panelName() const
{
    return "OptionsPanel";
}

void UIVMLogViewerOptionsPanel::prepareWidgets()
{
    if (!mainLayout())
        return;

    /* Create line-number check-box: */
    m_pLineNumberCheckBox = new QCheckBox;
    if (m_pLineNumberCheckBox)
    {
        m_pLineNumberCheckBox->setChecked(true);
        mainLayout()->addWidget(m_pLineNumberCheckBox, 0, Qt::AlignLeft);
    }

    /* Create wrap-lines check-box: */
    m_pWrapLinesCheckBox = new QCheckBox;
    if (m_pWrapLinesCheckBox)
    {
        m_pWrapLinesCheckBox->setChecked(false);
        mainLayout()->addWidget(m_pWrapLinesCheckBox, 0, Qt::AlignLeft);
    }

    /* Create font-size spin-box: */
    m_pFontSizeSpinBox = new QSpinBox;
    if (m_pFontSizeSpinBox)
    {
        mainLayout()->addWidget(m_pFontSizeSpinBox, 0, Qt::AlignLeft);
        m_pFontSizeSpinBox->setValue(m_iDefaultFontSize);
        m_pFontSizeSpinBox->setMaximum(44);
        m_pFontSizeSpinBox->setMinimum(6);
    }

    /* Create font-size label: */
    m_pFontSizeLabel = new QLabel;
    if (m_pFontSizeLabel)
    {
        mainLayout()->addWidget(m_pFontSizeLabel, 0, Qt::AlignLeft);
        if (m_pFontSizeSpinBox)
            m_pFontSizeLabel->setBuddy(m_pFontSizeSpinBox);
    }

    /* Create combo/button layout: */
    QHBoxLayout *pButtonLayout = new QHBoxLayout;
    if (pButtonLayout)
    {
        pButtonLayout->setContentsMargins(0, 0, 0, 0);
        pButtonLayout->setSpacing(0);

        /* Create open font dialog button: */
        m_pOpenFontDialogButton = new QIToolButton;
        if (m_pOpenFontDialogButton)
        {
            pButtonLayout->addWidget(m_pOpenFontDialogButton, 0);
            m_pOpenFontDialogButton->setIcon(UIIconPool::iconSet(":/log_viewer_choose_font_16px.png"));
        }

        /* Create reset font to default button: */
        m_pResetToDefaultsButton = new QIToolButton;
        if (m_pResetToDefaultsButton)
        {
            pButtonLayout->addWidget(m_pResetToDefaultsButton, 0);
            m_pResetToDefaultsButton->setIcon(UIIconPool::iconSet(":/log_viewer_reset_font_16px.png"));
        }

        mainLayout()->addLayout(pButtonLayout);
    }

    mainLayout()->addStretch(2);
}

void UIVMLogViewerOptionsPanel::prepareConnections()
{
    if (m_pLineNumberCheckBox)
        connect(m_pLineNumberCheckBox, &QCheckBox::toggled, this, &UIVMLogViewerOptionsPanel::sigShowLineNumbers);
    if (m_pWrapLinesCheckBox)
        connect(m_pWrapLinesCheckBox, &QCheckBox::toggled, this, &UIVMLogViewerOptionsPanel::sigWrapLines);
    if (m_pFontSizeSpinBox)
        connect(m_pFontSizeSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                this, &UIVMLogViewerOptionsPanel::sigChangeFontSizeInPoints);
    if (m_pOpenFontDialogButton)
        connect(m_pOpenFontDialogButton, &QIToolButton::clicked, this, &UIVMLogViewerOptionsPanel::sltOpenFontDialog);
    if (m_pResetToDefaultsButton)
        connect(m_pResetToDefaultsButton, &QIToolButton::clicked, this, &UIVMLogViewerOptionsPanel::sigResetToDefaults);
}

void UIVMLogViewerOptionsPanel::retranslateUi()
{
    UIVMLogViewerPanel::retranslateUi();

    m_pLineNumberCheckBox->setText(UIVMLogViewerWidget::tr("Show Line Numbers"));
    m_pLineNumberCheckBox->setToolTip(UIVMLogViewerWidget::tr("When checked, show line numbers"));

    m_pWrapLinesCheckBox->setText(UIVMLogViewerWidget::tr("Wrap Lines"));
    m_pWrapLinesCheckBox->setToolTip(UIVMLogViewerWidget::tr("When checked, wrap lines"));

    m_pFontSizeLabel->setText(UIVMLogViewerWidget::tr("Font Size"));
    m_pFontSizeSpinBox->setToolTip(UIVMLogViewerWidget::tr("Log viewer font size"));

    m_pOpenFontDialogButton->setToolTip(UIVMLogViewerWidget::tr("Open a font dialog to select font face for the logviewer"));
    m_pResetToDefaultsButton->setToolTip(UIVMLogViewerWidget::tr("Reset options to application defaults"));
}

void UIVMLogViewerOptionsPanel::sltOpenFontDialog()
{
    QFont currentFont;
    UIVMLogViewerWidget* parentWidget = qobject_cast<UIVMLogViewerWidget*>(parent());
    if (!parentWidget)
        return;

    currentFont = parentWidget->currentFont();
    bool ok;
    QFont font =
        QFontDialog::getFont(&ok, currentFont, this, "Logviewer font");

    if (ok)
        emit sigChangeFont(font);
}
