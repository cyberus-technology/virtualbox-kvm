/* $Id: UIMediumEnumerator.h $ */
/** @file
 * VBox Qt GUI - UIMediumEnumerator class declaration.
 */

/*
 * Copyright (C) 2013-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_medium_UIMediumEnumerator_h
#define FEQT_INCLUDED_SRC_medium_UIMediumEnumerator_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QObject>
#include <QSet>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UILibraryDefs.h"
#include "UIMedium.h"

/* COM includes: */
# include "CMedium.h"
# include "CMediumAttachment.h"

/* Forward declarations: */
class UITask;
class UIThreadPool;

/** A map of CMedium objects ordered by their IDs. */
typedef QMap<QUuid, CMedium> CMediumMap;

/** QObject extension operating as medium-enumeration object.
  * Manages access to cached UIMedium information via public API.
  * Updates cache on corresponding Main events using thread-pool interface. */
class SHARED_LIBRARY_STUFF UIMediumEnumerator : public QIWithRetranslateUI3<QObject>
{
    Q_OBJECT;

signals:

    /** Notifies listeners about UIMedium with @a uMediumID created. */
    void sigMediumCreated(const QUuid &uMediumID);
    /** Notifies listeners about UIMedium with @a uMediumID deleted. */
    void sigMediumDeleted(const QUuid &uMediumID);

    /** Notifies listeners about consolidated medium-enumeration process has started. */
    void sigMediumEnumerationStarted();
    /** Notifies listeners about UIMedium with @a uMediumID updated. */
    void sigMediumEnumerated(const QUuid &uMediumID);
    /** Notifies listeners about consolidated medium-enumeration process has finished. */
    void sigMediumEnumerationFinished();

public:

    /** Constructs medium-enumerator object. */
    UIMediumEnumerator();

    /** Returns cached UIMedium ID list. */
    QList<QUuid> mediumIDs() const;
    /** Returns a wrapper of cached UIMedium with specified @a uMediumID. */
    UIMedium medium(const QUuid &uMediumID) const;

    /** Creates UIMedium thus caching it internally on the basis of passed @a guiMedium information. */
    void createMedium(const UIMedium &guiMedium);

    /** Returns whether full consolidated medium-enumeration process is requested. */
    bool isFullMediumEnumerationRequested() const { return m_fFullMediumEnumerationRequested; }
    /** Returns whether any consolidated medium-enumeration process is in progress. */
    bool isMediumEnumerationInProgress() const { return m_fMediumEnumerationInProgress; }
    /** Makes a request to enumerate specified @a comMedia.
      * @note  Empty passed map means that full/overall medium-enumeration is requested.
      *        In that case previous map will be replaced with the new one, values
      *        present in both maps will be merged from the previous to new one.
      * @note  Non-empty passed map means that additional medium-enumeration is requested.
      *        In that case previous map will be extended with the new one, values
      *        present in both maps will be merged from the previous to new one. */
    void enumerateMedia(const CMediumVector &comMedia = CMediumVector());
    /** Refresh all the lightweight UIMedium information for all the cached UIMedium(s).
      * @note  Please note that this is a lightweight version, which doesn't perform
      *        heavy state/accessibility checks thus doesn't require to be performed
      *        by a worker COM-aware thread. */
    void refreshMedia();

protected:

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;

private slots:

    /** Handles machine-data-change event for a machine with specified @a uMachineId. */
    void sltHandleMachineDataChange(const QUuid &uMachineId);

    /** Handles signal about storage controller change.
      * @param  uMachineId         Brings the ID of machine corresponding controller belongs to.
      * @param  strControllerName  Brings the name of controller this event is related to. */
    void sltHandleStorageControllerChange(const QUuid &uMachineId, const QString &strControllerName);
    /** Handles signal about storage device change.
      * @param  comAttachment  Brings corresponding attachment.
      * @param  fRemoved       Brings whether medium is removed or added.
      * @param  fSilent        Brings whether this change has gone silent for guest. */
    void sltHandleStorageDeviceChange(CMediumAttachment comAttachment, bool fRemoved, bool fSilent);
    /** Handles signal about storage medium @a comAttachment state change. */
    void sltHandleMediumChange(CMediumAttachment comAttachment);
    /** Handles signal about storage @a comMedium config change. */
    void sltHandleMediumConfigChange(CMedium comMedium);
    /** Handles signal about storage medium is (un)registered.
      * @param  uMediumId      Brings corresponding medium ID.
      * @param  enmMediumType  Brings corresponding medium type.
      * @param  fRegistered    Brings whether medium is registered or unregistered. */
    void sltHandleMediumRegistered(const QUuid &uMediumId, KDeviceType enmMediumType, bool fRegistered);

    /** Handles medium-enumeration @a pTask complete signal. */
    void sltHandleMediumEnumerationTaskComplete(UITask *pTask);

private:

    /** Creates medium-enumeration task for certain @a guiMedium. */
    void createMediumEnumerationTask(const UIMedium &guiMedium);
    /** Adds NULL UIMedium to specified @a outputMedia map. */
    void addNullMediumToMap(UIMediumMap &outputMedia);
    /** Adds @a inputMedia to specified @a outputMedia map. */
    void addMediaToMap(const CMediumVector &inputMedia, UIMediumMap &outputMedia);

    /** Parses incoming @a comAttachment, enumerating the media it has attached.
      * @param  result  Brings the list of previously enumerated media
      *                 IDs to be appended with newly enumerated. */
    void parseAttachment(CMediumAttachment comAttachment, QList<QUuid> &result);
    /** Parses incoming @a comMedium, enumerating the media it represents.
      * @param  result  Brings the list of previously enumerated media
      *                 IDs to be appended with newly enumerated. */
    void parseMedium(CMedium comMedium, QList<QUuid> &result);

    /** Enumerates all the known media attached to machine with certain @a uMachineId.
      * @param  result  Brings the list of previously enumerated media
      *                 IDs to be appended with newly enumerated. */
    void enumerateAllMediaOfMachineWithId(const QUuid &uMachineId, QList<QUuid> &result);
    /** Enumerates all the children media of medium with certain @a uMediumId.
      * @param  result  Brings the list of previously enumerated media
      *                 IDs to be appended with newly enumerated. */
    void enumerateAllMediaOfMediumWithId(const QUuid &uMediumId, QList<QUuid> &result);

    /** Holds whether full consolidated medium-enumeration process is requested. */
    bool  m_fFullMediumEnumerationRequested;
    /** Holds whether any consolidated medium-enumeration process is in progress. */
    bool  m_fMediumEnumerationInProgress;

    /** Holds a set of current medium-enumeration tasks. */
    QSet<UITask*>  m_tasks;

    /** Holds a map of currently cached (enumerated) media. */
    UIMediumMap  m_media;
    /** Holds a set of currently registered media IDs. */
    QSet<QUuid>  m_registeredMediaIds;
};

#endif /* !FEQT_INCLUDED_SRC_medium_UIMediumEnumerator_h */
