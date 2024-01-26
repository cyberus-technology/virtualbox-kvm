/* $Id: UIAudioControllerEditor.h $ */
/** @file
 * VBox Qt GUI - UIAudioControllerEditor class declaration.
 */

/*
 * Copyright (C) 2019-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_settings_editors_UIAudioControllerEditor_h
#define FEQT_INCLUDED_SRC_settings_editors_UIAudioControllerEditor_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UILibraryDefs.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declarations: */
class QComboBox;
class QGridLayout;
class QLabel;

/** QWidget subclass used as an audio controller editor. */
class SHARED_LIBRARY_STUFF UIAudioControllerEditor : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /** Constructs editor passing @a pParent to the base-class. */
    UIAudioControllerEditor(QWidget *pParent = 0);

    /** Defines editor @a enmValue. */
    void setValue(KAudioControllerType enmValue);
    /** Returns editor value. */
    KAudioControllerType value() const;

    /** Returns the vector of supported values. */
    QVector<KAudioControllerType> supportedValues() const { return m_supportedValues; }

    /** Returns minimum layout hint. */
    int minimumLabelHorizontalHint() const;
    /** Defines minimum layout @a iIndent. */
    void setMinimumLayoutIndent(int iIndent);

protected:

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;

private:

    /** Prepares all. */
    void prepare();
    /** Populates combo. */
    void populateCombo();

    /** Holds the value to be selected. */
    KAudioControllerType  m_enmValue;

    /** Holds the vector of supported values. */
    QVector<KAudioControllerType>  m_supportedValues;

    /** Holds the main layout instance. */
    QGridLayout *m_pLayout;
    /** Holds the label instance. */
    QLabel      *m_pLabel;
    /** Holds the combo instance. */
    QComboBox   *m_pCombo;
};

#endif /* !FEQT_INCLUDED_SRC_settings_editors_UIAudioControllerEditor_h */
