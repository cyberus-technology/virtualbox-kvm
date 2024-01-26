/* $Id: UIConverter.h $ */
/** @file
 * VBox Qt GUI - UIConverter declaration.
 */

/*
 * Copyright (C) 2012-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_converter_UIConverter_h
#define FEQT_INCLUDED_SRC_converter_UIConverter_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIConverterBackend.h"

/** High-level interface for different conversions between GUI classes.
  * @todo Replace singleton with static template interface. */
class SHARED_LIBRARY_STUFF UIConverter
{
public:

    /** Returns singleton instance. */
    static UIConverter *instance() { return s_pInstance; }

    /** Creates singleton instance. */
    static void create();
    /** Destroys singleton instance. */
    static void destroy();

    /** Converts QColor <= template class. */
    template<class T> QColor toColor(const T &data) const
    {
        if (canConvert<T>())
            return ::toColor(data);
        AssertFailed();
        return QColor();
    }

    /** Converts QIcon <= template class. */
    template<class T> QIcon toIcon(const T &data) const
    {
        if (canConvert<T>())
            return ::toIcon(data);
        AssertFailed();
        return QIcon();
    }
    /** Converts QPixmap <= template class. */
    template<class T> QPixmap toWarningPixmap(const T &data) const
    {
        if (canConvert<T>())
            return ::toWarningPixmap(data);
        AssertFailed();
        return QPixmap();
    }

    /** Converts QString <= template class. */
    template<class T> QString toString(const T &data) const
    {
        if (canConvert<T>())
            return ::toString(data);
        AssertFailed();
        return QString();
    }
    /** Converts template class <= QString. */
    template<class T> T fromString(const QString &strData) const
    {
        if (canConvert<T>())
            return ::fromString<T>(strData);
        AssertFailed();
        return T();
    }

    /** Converts QString <= template class. */
    template<class T> QString toInternalString(const T &data) const
    {
        if (canConvert<T>())
            return ::toInternalString(data);
        AssertFailed();
        return QString();
    }
    /** Converts template class <= QString. */
    template<class T> T fromInternalString(const QString &strData) const
    {
        if (canConvert<T>())
            return ::fromInternalString<T>(strData);
        AssertFailed();
        return T();
    }

    /** Converts int <= template class. */
    template<class T> int toInternalInteger(const T &data) const
    {
        if (canConvert<T>())
            return ::toInternalInteger(data);
        AssertFailed();
        return 0;
    }
    /** Converts template class <= int. */
    template<class T> T fromInternalInteger(const int &iData) const
    {
        if (canConvert<T>())
            return ::fromInternalInteger<T>(iData);
        AssertFailed();
        return T();
    }

private:

    /** Constructs converter. */
    UIConverter() { s_pInstance = this; }
    /** Destructs converter. */
    virtual ~UIConverter() /* override final */ { s_pInstance = 0; }

    /** Holds the static instance. */
    static UIConverter *s_pInstance;
};

/** Singleton UI converter 'official' name. */
#define gpConverter UIConverter::instance()

#endif /* !FEQT_INCLUDED_SRC_converter_UIConverter_h */
