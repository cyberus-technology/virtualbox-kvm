/* $Id: UIProgressEventHandler.cpp $ */
/** @file
 * VBox Qt GUI - UIProgressEventHandler class implementation.
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

/* GUI includes: */
#include "UIExtraDataManager.h"
#include "UIMainEventListener.h"
#include "UIProgressEventHandler.h"
#include "UICommon.h"
#ifdef VBOX_WS_MAC
# include "VBoxUtils-darwin.h"
#endif /* VBOX_WS_MAC */

UIProgressEventHandler::UIProgressEventHandler(QObject *pParent, const CProgress &comProgress)
    : QObject(pParent)
    , m_comProgress(comProgress)
{
    /* Prepare: */
    prepare();
}

UIProgressEventHandler::~UIProgressEventHandler()
{
    /* Cleanup: */
    cleanup();
}

void UIProgressEventHandler::prepare()
{
    /* Prepare: */
    prepareListener();
    prepareConnections();
}

void UIProgressEventHandler::prepareListener()
{
    /* Create event listener instance: */
    m_pQtListener.createObject();
    m_pQtListener->init(new UIMainEventListener, this);
    m_comEventListener = CEventListener(m_pQtListener);

    /* Get CProgress event source: */
    CEventSource comEventSourceProgress = m_comProgress.GetEventSource();
    AssertWrapperOk(comEventSourceProgress);

    /* Enumerate all the required event-types: */
    QVector<KVBoxEventType> eventTypes;
    eventTypes
        << KVBoxEventType_OnProgressPercentageChanged
        << KVBoxEventType_OnProgressTaskCompleted;

    /* Register event listener for CProgress event source: */
    comEventSourceProgress.RegisterListener(m_comEventListener, eventTypes, FALSE /* active? */);
    AssertWrapperOk(comEventSourceProgress);

    /* Register event sources in their listeners as well: */
    m_pQtListener->getWrapped()->registerSource(comEventSourceProgress,
                                                m_comEventListener,
                                                QSet<KVBoxEventType>() << KVBoxEventType_OnProgressTaskCompleted);
}

void UIProgressEventHandler::prepareConnections()
{
    /* Create direct (sync) connections for signals of main listener: */
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigListeningFinished,
            this, &UIProgressEventHandler::sigHandlingFinished,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigProgressPercentageChange,
            this, &UIProgressEventHandler::sigProgressPercentageChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigProgressTaskComplete,
            this, &UIProgressEventHandler::sigProgressTaskComplete,
            Qt::DirectConnection);
}

void UIProgressEventHandler::cleanupConnections()
{
    /* Nothing for now. */
}

void UIProgressEventHandler::cleanupListener()
{
    /* Unregister everything: */
    m_pQtListener->getWrapped()->unregisterSources();

    /* Make sure VBoxSVC is available: */
    if (!uiCommon().isVBoxSVCAvailable())
        return;

    /* Get CProgress event source: */
    CEventSource comEventSourceProgress = m_comProgress.GetEventSource();
    AssertWrapperOk(comEventSourceProgress);

    /* Unregister event listener for CProgress event source: */
    comEventSourceProgress.UnregisterListener(m_comEventListener);
}

void UIProgressEventHandler::cleanup()
{
    /* Cleanup: */
    cleanupConnections();
    cleanupListener();
}
