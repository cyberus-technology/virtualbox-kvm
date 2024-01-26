/* $Id: QILineEdit.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QILineEdit class declaration.
 */

/*
 * Copyright (C) 2008-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_extensions_QILineEdit_h
#define FEQT_INCLUDED_SRC_extensions_QILineEdit_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes */
#include <QIcon>
#include <QLineEdit>

/* GUI includes: */
#include "UILibraryDefs.h"

class QLabel;

/** QLineEdit extension with advanced functionality. */
class SHARED_LIBRARY_STUFF QILineEdit : public QLineEdit
{
    Q_OBJECT;

public:

    /** Constructs line-edit passing @a pParent to the base-class. */
    QILineEdit(QWidget *pParent = 0);
    /** Constructs line-edit passing @a pParent to the base-class.
      * @param  strText  Brings the line-edit text. */
    QILineEdit(const QString &strText, QWidget *pParent = 0);

    /** Defines whether this is @a fAllowed to copy contents when disabled. */
    void setAllowToCopyContentsWhenDisabled(bool fAllowed);

    /** Forces line-edit to adjust minimum width acording to passed @a strText. */
    void setMinimumWidthByText(const QString &strText);
    /** Forces line-edit to adjust fixed width acording to passed @a strText. */
    void setFixedWidthByText(const QString &strText);

    /** Puts an icon to mark some error on the right hand side of the line edit. @p is used as tooltip of the icon. */
    void mark(bool fError, const QString &strErrorMessage = QString());

protected:

    /** Handles any Qt @a pEvent. */
    virtual bool event(QEvent *pEvent) RT_OVERRIDE;

    /** Handles resize @a pEvent. */
    virtual void resizeEvent(QResizeEvent *pResizeEvent) RT_OVERRIDE;

private slots:

    /** Copies text into clipboard. */
    void copy();

private:

    /** Prepares all. */
    void prepare();

    /** Calculates suitable @a strText size. */
    QSize fitTextWidth(const QString &strText) const;

    /** Holds whether this is allowed to copy contents when disabled. */
    bool     m_fAllowToCopyContentsWhenDisabled;
    /** Holds the copy to clipboard action. */
    QAction *m_pCopyAction;

    QLabel *m_pIconLabel;
    QIcon   m_markIcon;
    bool    m_fMarkForError;
    QString m_strErrorMessage;
};

class SHARED_LIBRARY_STUFF UIMarkableLineEdit : public QWidget
{
    Q_OBJECT;

signals:

    void textChanged(const QString &strText);

public:

    UIMarkableLineEdit(QWidget *pParent = 0);
    void mark(bool fError, const QString &strErrorMessage = QString());

    /** @name Pass through functions for QILineEdit.
      * @{ */
        void setText(const QString &strText);
        QString text() const;
        void setValidator(const QValidator *pValidator);
        bool hasAcceptableInput() const;
        void setPlaceholderText(const QString &strText);
    /** @} */

private:

    void prepare();

    QILineEdit *m_pLineEdit;
    QLabel *m_pIconLabel;
};

#endif /* !FEQT_INCLUDED_SRC_extensions_QILineEdit_h */
