/* $Id: UIVisoConfigurationPanel.h $ */
/** @file
 * VBox Qt GUI - UIVisoConfigurationPanel class declaration.
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

#ifndef FEQT_INCLUDED_SRC_medium_viso_UIVisoConfigurationPanel_h
#define FEQT_INCLUDED_SRC_medium_viso_UIVisoConfigurationPanel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIDialogPanel.h"

/* Forward declarations: */
class QComboBox;
class QILabel;
class QILineEdit;
class QIToolButton;

class UIVisoConfigurationPanel : public UIDialogPanel
{
    Q_OBJECT;

signals:

    void sigVisoNameChanged(const QString &strVisoName);
    void sigCustomVisoOptionsChanged(const QStringList &customVisoOptions);

public:
    UIVisoConfigurationPanel(QWidget *pParent = 0);
    ~UIVisoConfigurationPanel();
    virtual QString panelName() const RT_OVERRIDE;
    void setVisoName(const QString& strVisoName);
    void setVisoCustomOptions(const QStringList& visoCustomOptions);

protected:

    void retranslateUi() RT_OVERRIDE;

private slots:

    void sltHandleVisoNameChanged();
    void sltHandleDeleteCurrentCustomOption();

private:

    void prepareObjects();
    void prepareConnections();
    void addCustomVisoOption();
    void emitCustomVisoOptions();

    QILabel      *m_pVisoNameLabel;
    QILabel      *m_pCustomOptionsLabel;
    QILineEdit   *m_pVisoNameLineEdit;
    QComboBox    *m_pCustomOptionsComboBox;
    QIToolButton *m_pDeleteButton;
};

#endif /* !FEQT_INCLUDED_SRC_medium_viso_UIVisoConfigurationPanel_h */
