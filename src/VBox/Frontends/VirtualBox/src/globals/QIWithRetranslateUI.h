/* $Id: QIWithRetranslateUI.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIWithRetranslateUI class declaration.
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

#ifndef FEQT_INCLUDED_SRC_globals_QIWithRetranslateUI_h
#define FEQT_INCLUDED_SRC_globals_QIWithRetranslateUI_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QApplication>
#include <QDialog>
#include <QEvent>
#include <QGraphicsWidget>
#include <QObject>
#include <QWidget>

/* GUI includes: */
#include "UILibraryDefs.h"
#include "UITranslator.h"


/** Template for automatic language translations of underlying QWidget. */
template <class Base>
class QIWithRetranslateUI : public Base
{
public:

    /** Constructs translatable widget passing @a pParent to the base-class. */
    QIWithRetranslateUI(QWidget *pParent = 0) : Base(pParent)
    {
        qApp->installEventFilter(this);
    }

protected:

    /** Pre-handles standard Qt @a pEvent for passed @a pObject. */
    virtual bool eventFilter(QObject *pObject, QEvent *pEvent)
    {
        /* If translation is NOT currently in progress handle
         * LanguageChange events for qApp or this object: */
        if (   !UITranslator::isTranslationInProgress()
            && pEvent->type() == QEvent::LanguageChange
            && (pObject == qApp || pObject == this))
            retranslateUi();

        /* Call to base-class: */
        return Base::eventFilter(pObject, pEvent);
    }

    /** Handles translation event. */
    virtual void retranslateUi() = 0;
};

/** Explicit QIWithRetranslateUI instantiation for QWidget & QDialog classes.
  * @note  On Windows it's important that all template cases are instantiated just once across
  *        the linking space. In case we have particular template case instantiated from both
  *        library and executable sides, - we have multiple definition case and need to strictly
  *        ask compiler to do it just once and link such cases against library only.
  *        I would also note that it would be incorrect to just make whole the template exported
  *        to library because latter can have lack of required instantiations (current case). */
template class SHARED_LIBRARY_STUFF QIWithRetranslateUI<QWidget>;
template class SHARED_LIBRARY_STUFF QIWithRetranslateUI<QDialog>;


/** Template for automatic language translations of underlying QWidget with certain flags. */
template <class Base>
class QIWithRetranslateUI2 : public Base
{
public:

    /** Constructs translatable widget passing @a pParent and @a enmFlags to the base-class. */
    QIWithRetranslateUI2(QWidget *pParent = 0, Qt::WindowFlags enmFlags = Qt::WindowFlags()) : Base(pParent, enmFlags)
    {
        qApp->installEventFilter(this);
    }

protected:

    /** Pre-handles standard Qt @a pEvent for passed @a pObject. */
    virtual bool eventFilter(QObject *pObject, QEvent *pEvent)
    {
        /* If translation is NOT currently in progress handle
         * LanguageChange events for qApp or this object: */
        if (   !UITranslator::isTranslationInProgress()
            && pEvent->type() == QEvent::LanguageChange
            && (pObject == qApp || pObject == this))
            retranslateUi();

        /* Call to base-class: */
        return Base::eventFilter(pObject, pEvent);
    }

    /** Handles translation event. */
    virtual void retranslateUi() = 0;
};


/** Template for automatic language translations of underlying QObject. */
template <class Base>
class QIWithRetranslateUI3 : public Base
{
public:

    /** Constructs translatable widget passing @a pParent to the base-class. */
    QIWithRetranslateUI3(QObject *pParent = 0)
        : Base(pParent)
    {
        qApp->installEventFilter(this);
    }

protected:

    /** Pre-handles standard Qt @a pEvent for passed @a pObject. */
    virtual bool eventFilter(QObject *pObject, QEvent *pEvent)
    {
        /* If translation is NOT currently in progress handle
         * LanguageChange events for qApp or this object: */
        if (   !UITranslator::isTranslationInProgress()
            && pEvent->type() == QEvent::LanguageChange
            && (pObject == qApp || pObject == this))
            retranslateUi();

        /* Call to base-class: */
        return Base::eventFilter(pObject, pEvent);
    }

    /** Handles translation event. */
    virtual void retranslateUi() = 0;
};

/** Explicit QIWithRetranslateUI3 instantiation for QObject class.
  * @note  On Windows it's important that all template cases are instantiated just once across
  *        the linking space. In case we have particular template case instantiated from both
  *        library and executable sides, - we have multiple definition case and need to strictly
  *        ask compiler to do it just once and link such cases against library only.
  *        I would also note that it would be incorrect to just make whole the template exported
  *        to library because latter can have lack of required instantiations (current case). */
template class SHARED_LIBRARY_STUFF QIWithRetranslateUI3<QObject>;


/** Template for automatic language translations of underlying QGraphicsWidget. */
template <class Base>
class QIWithRetranslateUI4 : public Base
{
public:

    /** Constructs translatable widget passing @a pParent to the base-class. */
    QIWithRetranslateUI4(QGraphicsWidget *pParent = 0)
        : Base(pParent)
    {
        qApp->installEventFilter(this);
    }

protected:

    /** Pre-handles standard Qt @a pEvent for passed @a pObject. */
    virtual bool eventFilter(QObject *pObject, QEvent *pEvent)
    {
        /* If translation is NOT currently in progress handle
         * LanguageChange events for qApp or this object: */
        if (   !UITranslator::isTranslationInProgress()
            && pEvent->type() == QEvent::LanguageChange
            && (pObject == qApp || pObject == this))
            retranslateUi();

        /* Call to base-class: */
        return Base::eventFilter(pObject, pEvent);
    }

    /** Handles translation event. */
    virtual void retranslateUi() = 0;
};


#endif /* !FEQT_INCLUDED_SRC_globals_QIWithRetranslateUI_h */

