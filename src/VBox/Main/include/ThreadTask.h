/** @file
 * VirtualBox ThreadTask class definition
 */

/*
 * Copyright (C) 2015-2023 Oracle and/or its affiliates.
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

#ifndef MAIN_INCLUDED_ThreadTask_h
#define MAIN_INCLUDED_ThreadTask_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "VBox/com/string.h"

/**
 * The class ThreadVoidData is used as a base class for any data which we want to pass into a thread
 */
struct ThreadVoidData
{
public:
    ThreadVoidData() { }
    virtual ~ThreadVoidData() { }
};


class ThreadTask
{
public:
    ThreadTask(const Utf8Str &t)
        : m_strTaskName(t)
        , mAsync(false)
    { }

    virtual ~ThreadTask()
    { }

    HRESULT createThread(void);
    HRESULT createThreadWithType(RTTHREADTYPE enmType);

    inline Utf8Str getTaskName() const { return m_strTaskName; }
    bool isAsync() { return mAsync; }

protected:
    HRESULT createThreadInternal(RTTHREADTYPE enmType);
    static DECLCALLBACK(int) taskHandlerThreadProc(RTTHREAD thread, void *pvUser);

    ThreadTask() : m_strTaskName("GenericTask")
    { }

    Utf8Str m_strTaskName;
    bool mAsync;

private:
    virtual void handler() = 0;
};

#endif /* !MAIN_INCLUDED_ThreadTask_h */

