/* $Id: UIDescriptionEditor.h $ */
/** @file
 * VBox Qt GUI - UIDescriptionEditor class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_editors_UIDescriptionEditor_h
#define FEQT_INCLUDED_SRC_settings_editors_UIDescriptionEditor_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QTextEdit;

/** QWidget subclass used as machine description editor. */
class SHARED_LIBRARY_STUFF UIDescriptionEditor : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /** Constructs editor passing @a pParent to the base-class. */
    UIDescriptionEditor(QWidget *pParent = 0);

    /** Defines editor @a strValue. */
    void setValue(const QString &strValue);
    /** Returns editor value. */
    QString value() const;

protected:

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;

private:

    /** Prepares all. */
    void prepare();

    /** Holds the value to be set. */
    QString  m_strValue;

    /** Holds the check-box instance. */
    QTextEdit *m_pTextEdit;
};

#endif /* !FEQT_INCLUDED_SRC_settings_editors_UIDescriptionEditor_h */
