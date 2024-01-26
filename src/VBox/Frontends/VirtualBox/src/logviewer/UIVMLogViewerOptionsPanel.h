/* $Id: UIVMLogViewerOptionsPanel.h $ */
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

#ifndef FEQT_INCLUDED_SRC_logviewer_UIVMLogViewerOptionsPanel_h
#define FEQT_INCLUDED_SRC_logviewer_UIVMLogViewerOptionsPanel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIVMLogViewerPanel.h"

/* Forward declarations: */
class QCheckBox;
class QSpinBox;
class QLabel;
class QIToolButton;
class UIVMLogViewerWidget;

/** UIVMLogViewerPanel extension providing GUI to manage logviewer options. */
class UIVMLogViewerOptionsPanel : public UIVMLogViewerPanel
{
    Q_OBJECT;

signals:

    void sigShowLineNumbers(bool show);
    void sigWrapLines(bool show);
    void sigChangeFontSizeInPoints(int size);
    void sigChangeFont(QFont font);
    void sigResetToDefaults();

public:

    UIVMLogViewerOptionsPanel(QWidget *pParent, UIVMLogViewerWidget *pViewer);

    void setShowLineNumbers(bool bShowLineNumbers);
    void setWrapLines(bool bWrapLines);
    void setFontSizeInPoints(int fontSizeInPoints);
    virtual QString panelName() const RT_OVERRIDE;

public slots:


protected:

    virtual void prepareWidgets() RT_OVERRIDE;
    virtual void prepareConnections() RT_OVERRIDE;

    /** Handles the translation event. */
    void retranslateUi();

private slots:

    void sltOpenFontDialog();

private:

    QCheckBox    *m_pLineNumberCheckBox;
    QCheckBox    *m_pWrapLinesCheckBox;
    QSpinBox     *m_pFontSizeSpinBox;
    QLabel       *m_pFontSizeLabel;
    QIToolButton *m_pOpenFontDialogButton;
    QIToolButton *m_pResetToDefaultsButton;

    /** Default font size in points. */
    const int    m_iDefaultFontSize;

};

#endif /* !FEQT_INCLUDED_SRC_logviewer_UIVMLogViewerOptionsPanel_h */
