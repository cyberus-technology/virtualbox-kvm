/* $Id: UIStarter.h $ */
/** @file
 * VBox Qt GUI - UIStarter class declaration.
 */

/*
 * Copyright (C) 2018-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_globals_UIStarter_h
#define FEQT_INCLUDED_SRC_globals_UIStarter_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QObject>

/** QObject subclass allowing to control GUI part
  * of VirtualBox application in sync/async modes. */
class UIStarter : public QObject
{
    Q_OBJECT;

    /** Constructs UI starter. */
    UIStarter();
    /** Destructs UI starter. */
    virtual ~UIStarter();

public:

    /** Returns the singleton UI starter instance. */
    static UIStarter *instance() { return s_pInstance; }

    /** Create the singleton UI starter instance. */
    static void create();
    /** Create the singleton UI starter instance. */
    static void destroy();

    /** Init UICommon connections. */
    void init();
    /** Deinit UICommon connections. */
    void deinit();

private slots:

    /** Starts corresponding part of the UI. */
    void sltStartUI();
    /** Restarts corresponding part of the UI. */
    void sltRestartUI();
    /** Closes corresponding part of the UI. */
    void sltCloseUI();

private:

    /** Holds the singleton UI starter instance. */
    static UIStarter *s_pInstance;
};

/** Singleton UI starter 'official' name. */
#define gStarter UIStarter::instance()

#endif /* !FEQT_INCLUDED_SRC_globals_UIStarter_h */
