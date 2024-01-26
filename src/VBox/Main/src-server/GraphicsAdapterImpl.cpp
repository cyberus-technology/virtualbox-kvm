/* $Id: GraphicsAdapterImpl.cpp $ */
/** @file
 * Implementation of IGraphicsAdapter in VBoxSVC.
 */

/*
 * Copyright (C) 2004-2023 Oracle and/or its affiliates.
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

#define LOG_GROUP LOG_GROUP_MAIN_GRAPHICSADAPTER

#include "LoggingNew.h"

#include "GraphicsAdapterImpl.h"
#include "MachineImpl.h"

#include "AutoStateDep.h"
#include "AutoCaller.h"

#include <iprt/cpp/utils.h>


// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

GraphicsAdapter::GraphicsAdapter() :
    mParent(NULL)
{}

GraphicsAdapter::~GraphicsAdapter()
{}

HRESULT GraphicsAdapter::FinalConstruct()
{
    LogFlowThisFunc(("\n"));
    return BaseFinalConstruct();
}

void GraphicsAdapter::FinalRelease()
{
    LogFlowThisFunc(("\n"));
    uninit();
    BaseFinalRelease();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

/**
 *  Initializes the graphics adapter object.
 *
 *  @param aParent  Handle of the parent object.
 */
HRESULT GraphicsAdapter::init(Machine *aParent)
{
    LogFlowThisFunc(("aParent=%p\n", aParent));

    ComAssertRet(aParent, E_INVALIDARG);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    unconst(mParent) = aParent;
    /* mPeer is left null */

    mData.allocate();

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return S_OK;
}

/**
 *  Initializes the graphics adapter object given another graphics adapter
 *  object (a kind of copy constructor). This object shares data with
 *  the object passed as an argument.
 *
 *  @note This object must be destroyed before the original object
 *  it shares data with is destroyed.
 *
 *  @note Locks @a aThat object for reading.
 */
HRESULT GraphicsAdapter::init(Machine *aParent, GraphicsAdapter *aThat)
{
    LogFlowThisFunc(("aParent=%p, aThat=%p\n", aParent, aThat));

    ComAssertRet(aParent && aThat, E_INVALIDARG);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    unconst(mParent) = aParent;
    unconst(mPeer) = aThat;

    AutoCaller thatCaller(aThat);
    AssertComRCReturnRC(thatCaller.hrc());

    AutoReadLock thatLock(aThat COMMA_LOCKVAL_SRC_POS);
    mData.share(aThat->mData);

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return S_OK;
}

/**
 *  Initializes the graphics adapter object given another graphics adapter
 *  object (a kind of copy constructor). This object makes a private copy
 *  of data of the original object passed as an argument.
 *
 *  @note Locks @a aThat object for reading.
 */
HRESULT GraphicsAdapter::initCopy(Machine *aParent, GraphicsAdapter *aThat)
{
    LogFlowThisFunc(("aParent=%p, aThat=%p\n", aParent, aThat));

    ComAssertRet(aParent && aThat, E_INVALIDARG);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    unconst(mParent) = aParent;
    /* mPeer is left null */

    AutoCaller thatCaller(aThat);
    AssertComRCReturnRC(thatCaller.hrc());

    AutoReadLock thatLock(aThat COMMA_LOCKVAL_SRC_POS);
    mData.attachCopy(aThat->mData);

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return S_OK;
}

/**
 *  Uninitializes the instance and sets the ready flag to FALSE.
 *  Called either from FinalRelease() or by the parent when it gets destroyed.
 */
void GraphicsAdapter::uninit()
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    mData.free();

    unconst(mPeer) = NULL;
    unconst(mParent) = NULL;
}

// Wrapped IGraphicsAdapter properties
/////////////////////////////////////////////////////////////////////////////

HRESULT GraphicsAdapter::getGraphicsControllerType(GraphicsControllerType_T *aGraphicsControllerType)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aGraphicsControllerType = mData->graphicsControllerType;

    return S_OK;
}

HRESULT GraphicsAdapter::setGraphicsControllerType(GraphicsControllerType_T aGraphicsControllerType)
{
    switch (aGraphicsControllerType)
    {
        case GraphicsControllerType_Null:
        case GraphicsControllerType_VBoxVGA:
#ifdef VBOX_WITH_VMSVGA
        case GraphicsControllerType_VMSVGA:
        case GraphicsControllerType_VBoxSVGA:
#endif
            break;
        default:
            return setError(E_INVALIDARG, tr("The graphics controller type (%d) is invalid"), aGraphicsControllerType);
    }

    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(mParent);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    mParent->i_setModified(Machine::IsModified_GraphicsAdapter);
    mData.backup();
    mData->graphicsControllerType = aGraphicsControllerType;

    return S_OK;
}

HRESULT GraphicsAdapter::getVRAMSize(ULONG *aVRAMSize)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aVRAMSize = mData->ulVRAMSizeMB;

    return S_OK;
}

HRESULT GraphicsAdapter::setVRAMSize(ULONG aVRAMSize)
{
    /* check VRAM limits */
    if (aVRAMSize > SchemaDefs::MaxGuestVRAM)
        return setError(E_INVALIDARG,
                        tr("Invalid VRAM size: %lu MB (must be in range [%lu, %lu] MB)"),
                        aVRAMSize, SchemaDefs::MinGuestVRAM, SchemaDefs::MaxGuestVRAM);

    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(mParent);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    mParent->i_setModified(Machine::IsModified_GraphicsAdapter);
    mData.backup();
    mData->ulVRAMSizeMB = aVRAMSize;

    return S_OK;
}

HRESULT GraphicsAdapter::getAccelerate3DEnabled(BOOL *aAccelerate3DEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAccelerate3DEnabled = mData->fAccelerate3D;

    return S_OK;
}

HRESULT GraphicsAdapter::setAccelerate3DEnabled(BOOL aAccelerate3DEnabled)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(mParent);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /** @todo check validity! */

    mParent->i_setModified(Machine::IsModified_GraphicsAdapter);
    mData.backup();
    mData->fAccelerate3D = !!aAccelerate3DEnabled;

    return S_OK;
}


HRESULT GraphicsAdapter::getAccelerate2DVideoEnabled(BOOL *aAccelerate2DVideoEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* bugref:9691 The legacy VHWA acceleration has been disabled completely. */
    *aAccelerate2DVideoEnabled = FALSE;

    return S_OK;
}

HRESULT GraphicsAdapter::setAccelerate2DVideoEnabled(BOOL aAccelerate2DVideoEnabled)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(mParent);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /** @todo check validity! */

    mParent->i_setModified(Machine::IsModified_GraphicsAdapter);
    mData.backup();
    mData->fAccelerate2DVideo = !!aAccelerate2DVideoEnabled;

    return S_OK;
}

HRESULT GraphicsAdapter::getMonitorCount(ULONG *aMonitorCount)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aMonitorCount = mData->cMonitors;

    return S_OK;
}

HRESULT GraphicsAdapter::setMonitorCount(ULONG aMonitorCount)
{
    /* make sure monitor count is a sensible number */
    if (aMonitorCount < 1 || aMonitorCount > SchemaDefs::MaxGuestMonitors)
        return setError(E_INVALIDARG,
                        tr("Invalid monitor count: %lu (must be in range [%lu, %lu])"),
                        aMonitorCount, 1, SchemaDefs::MaxGuestMonitors);

    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(mParent);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    mParent->i_setModified(Machine::IsModified_GraphicsAdapter);
    mData.backup();
    mData->cMonitors = aMonitorCount;

    return S_OK;
}

// Wrapped IGraphicsAdapter methods
/////////////////////////////////////////////////////////////////////////////

// public methods only for internal purposes
/////////////////////////////////////////////////////////////////////////////

/**
 *  Loads settings from the given machine node.
 *  May be called once right after this object creation.
 *
 *  @param data Configuration settings.
 *
 *  @note Locks this object for writing.
 */
HRESULT GraphicsAdapter::i_loadSettings(const settings::GraphicsAdapter &data)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.hrc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    mData.assignCopy(&data);

    return S_OK;
}

/**
 *  Saves settings to the given machine node.
 *
 *  @param data Configuration settings.
 *
 *  @note Locks this object for reading.
 */
HRESULT GraphicsAdapter::i_saveSettings(settings::GraphicsAdapter &data)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.hrc());

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    data = *mData.data();

    return S_OK;
}

/**
 *  @note Locks this object for writing.
 */
void GraphicsAdapter::i_rollback()
{
    /* sanity */
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.hrc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    mData.rollback();
}

/**
 *  @note Locks this object for writing, together with the peer object (also
 *  for writing) if there is one.
 */
void GraphicsAdapter::i_commit()
{
    /* sanity */
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.hrc());

    /* sanity too */
    AutoCaller peerCaller(mPeer);
    AssertComRCReturnVoid(peerCaller.hrc());

    /* lock both for writing since we modify both (mPeer is "master" so locked
     * first) */
    AutoMultiWriteLock2 alock(mPeer, this COMMA_LOCKVAL_SRC_POS);

    if (mData.isBackedUp())
    {
        mData.commit();
        if (mPeer)
        {
            /* attach new data to the peer and reshare it */
            mPeer->mData.attach(mData);
        }
    }
}

/**
 *  @note Locks this object for writing, together with the peer object
 *  represented by @a aThat (locked for reading).
 */
void GraphicsAdapter::i_copyFrom(GraphicsAdapter *aThat)
{
    AssertReturnVoid(aThat != NULL);

    /* sanity */
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.hrc());

    /* sanity too */
    AutoCaller thatCaller(aThat);
    AssertComRCReturnVoid(thatCaller.hrc());

    /* peer is not modified, lock it for reading (aThat is "master" so locked
     * first) */
    AutoReadLock rl(aThat COMMA_LOCKVAL_SRC_POS);
    AutoWriteLock wl(this COMMA_LOCKVAL_SRC_POS);

    /* this will back up current data */
    mData.assignCopy(aThat->mData);
}
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
