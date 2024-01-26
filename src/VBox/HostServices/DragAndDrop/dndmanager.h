/** @file
 * Drag and Drop manager.
 */

/*
 * Copyright (C) 2011-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_SRC_DragAndDrop_dndmanager_h
#define VBOX_INCLUDED_SRC_DragAndDrop_dndmanager_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/GuestHost/DragAndDrop.h>
#include <VBox/HostServices/Service.h>
#include <VBox/HostServices/DragAndDropSvc.h>

#include <iprt/cpp/ministring.h>
#include <iprt/cpp/list.h>

typedef DECLCALLBACKTYPE(int, FNDNDPROGRESS,(uint32_t uState, uint32_t uPercentage, int rc, void *pvUser));
typedef FNDNDPROGRESS *PFNDNDPROGRESS;

/**
 * DnD message class. This class forms the base of all other more specialized
 * message classes.
 */
class DnDMessage : public HGCM::Message
{
public:

    DnDMessage(void)
        : m_cRefs(0) { }

    DnDMessage(uint32_t uMsg, uint32_t cParms, VBOXHGCMSVCPARM aParms[])
        : Message(uMsg, cParms, aParms)
        , m_cRefs(0) { }

    virtual ~DnDMessage(void) { }

    uint32_t AddRef(void) { Assert(m_cRefs < 32); return ++m_cRefs; }
    uint32_t Release(void) { if (m_cRefs) return --m_cRefs; return m_cRefs; }
    uint32_t RefCount(void) const { return m_cRefs; }

protected:

    /** The message's current reference count. */
    uint32_t m_cRefs;
};

/**
 * DnD message class for generic messages which didn't need any special
 * handling.
 */
class DnDGenericMessage: public DnDMessage
{
public:
    DnDGenericMessage(uint32_t uMsg, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
        : DnDMessage(uMsg, cParms, paParms) { }
};

/**
 * DnD message class for informing the guest to cancel any current (and pending) activities.
 */
class DnDHGCancelMessage: public DnDMessage
{
public:

    DnDHGCancelMessage(void)
    {
        int rc2 = initData(DragAndDropSvc::HOST_DND_FN_CANCEL,
                           0 /* cParms */, 0 /* aParms */);
        AssertRC(rc2);
    }
};

/**
 * DnD manager. Manage creation and queuing of messages for the various DnD
 * messages types.
 */
class DnDManager
{
public:

    DnDManager(PFNDNDPROGRESS pfnProgressCallback, void *pvProgressUser)
        : m_pfnProgressCallback(pfnProgressCallback)
        , m_pvProgressUser(pvProgressUser)
    {}

    virtual ~DnDManager(void)
    {
        Reset(true /* fForce */);
    }

    int AddMsg(DnDMessage *pMessage, bool fAppend = true);
    int AddMsg(uint32_t uMsg, uint32_t cParms, VBOXHGCMSVCPARM paParms[], bool fAppend = true);

#ifdef DEBUG
    void DumpQueue();
#endif

    int GetNextMsgInfo(bool fAddRef, uint32_t *puType, uint32_t *pcParms);
    int GetNextMsg(uint32_t uMsg, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);

    void Reset(bool fForce);

protected:

    /** DnD message queue (FIFO). */
    RTCList<DnDMessage *> m_queueMsg;
    /** Pointer to host progress callback. Optional, can be NULL. */
    PFNDNDPROGRESS        m_pfnProgressCallback;
    /** Pointer to progress callback user context. Can be NULL if not used. */
    void                 *m_pvProgressUser;
};
#endif /* !VBOX_INCLUDED_SRC_DragAndDrop_dndmanager_h */

