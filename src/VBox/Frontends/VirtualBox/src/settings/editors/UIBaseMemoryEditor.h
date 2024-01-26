/* $Id: UIBaseMemoryEditor.h $ */
/** @file
 * VBox Qt GUI - UIBaseMemoryEditor class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_editors_UIBaseMemoryEditor_h
#define FEQT_INCLUDED_SRC_settings_editors_UIBaseMemoryEditor_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UILibraryDefs.h"

/* Forward declarations: */
class QGridLayout;
class QLabel;
class QSpinBox;
class UIBaseMemorySlider;

/** QWidget subclass used as a base memory editor. */
class SHARED_LIBRARY_STUFF UIBaseMemoryEditor : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    /** Notifies listeners about value has became @a fValid. */
    void sigValidChanged(bool fValid);
    /** Notifies listeners about @a iValue has changed. */
    void sigValueChanged(int iValue);

public:

    /** Constructs editor passing @a pParent to the base-class. */
    UIBaseMemoryEditor(QWidget *pParent = 0);

    /** Defines editor @a iValue. */
    void setValue(int iValue);
    /** Returns editor value. */
    int value() const;

    /** Returns the maximum optimal RAM. */
    uint maxRAMOpt() const;
    /** Returns the maximum allowed RAM. */
    uint maxRAMAlw() const;

    /** Returns minimum layout hint. */
    int minimumLabelHorizontalHint() const;
    /** Defines minimum layout @a iIndent. */
    void setMinimumLayoutIndent(int iIndent);

protected:

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;

private slots:

    /** Handles slider value changes. */
    void sltHandleSliderChange();
    /** Handles spin-box value changes. */
    void sltHandleSpinBoxChange();

private:

    /** Prepares all. */
    void prepare();

    /** Revalidates and emits validity change signal. */
    void revalidate();

    /** Holds the value to be selected. */
    int  m_iValue;

    /** @name Widgets
     * @{ */
        /** Holds the main layout instance. */
        QGridLayout        *m_pLayout;
        /** Holds the memory label instance. */
        QLabel             *m_pLabelMemory;
        /** Holds the memory slider instance. */
        UIBaseMemorySlider *m_pSlider;
        /** Holds minimum memory label instance. */
        QLabel             *m_pLabelMemoryMin;
        /** Holds maximum memory label instance. */
        QLabel             *m_pLabelMemoryMax;
        /** Holds the memory spin-box instance. */
        QSpinBox           *m_pSpinBox;
    /** @} */
};

#endif /* !FEQT_INCLUDED_SRC_settings_editors_UIBaseMemoryEditor_h */
