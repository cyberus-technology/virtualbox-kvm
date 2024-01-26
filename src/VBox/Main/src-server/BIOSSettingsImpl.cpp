/* $Id: BIOSSettingsImpl.cpp $ */
/** @file
 * VirtualBox COM class implementation - Machine BIOS settings.
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

#define LOG_GROUP LOG_GROUP_MAIN_BIOSSETTINGS
#include "BIOSSettingsImpl.h"
#include "MachineImpl.h"
#include "GuestOSTypeImpl.h"

#include <iprt/cpp/utils.h>
#include <VBox/settings.h>

#include "AutoStateDep.h"
#include "AutoCaller.h"
#include "LoggingNew.h"


////////////////////////////////////////////////////////////////////////////////
//
// BIOSSettings private data definition
//
////////////////////////////////////////////////////////////////////////////////

struct BIOSSettings::Data
{
    Data()
        : pMachine(NULL)
    { }

    Machine * const             pMachine;
    ComObjPtr<BIOSSettings>     pPeer;

    // use the XML settings structure in the members for simplicity
    Backupable<settings::BIOSSettings> bd;
};

// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

DEFINE_EMPTY_CTOR_DTOR(BIOSSettings)

HRESULT BIOSSettings::FinalConstruct()
{
    return BaseFinalConstruct();
}

void BIOSSettings::FinalRelease()
{
    uninit();
    BaseFinalRelease();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

/**
 * Initializes the BIOS settings object.
 *
 * @returns COM result indicator
 */
HRESULT BIOSSettings::init(Machine *aParent)
{
    LogFlowThisFuncEnter();
    LogFlowThisFunc(("aParent: %p\n", aParent));

    ComAssertRet(aParent, E_INVALIDARG);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    m = new Data();

    /* share the parent weakly */
    unconst(m->pMachine) = aParent;

    m->bd.allocate();

    autoInitSpan.setSucceeded();

    LogFlowThisFuncLeave();
    return S_OK;
}

/**
 *  Initializes the BIOS settings object given another BIOS settings object
 *  (a kind of copy constructor). This object shares data with
 *  the object passed as an argument.
 *
 *  @note This object must be destroyed before the original object
 *  it shares data with is destroyed.
 */
HRESULT BIOSSettings::init(Machine *aParent, BIOSSettings *that)
{
    LogFlowThisFuncEnter();
    LogFlowThisFunc(("aParent: %p, that: %p\n", aParent, that));

    ComAssertRet(aParent && that, E_INVALIDARG);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    m = new Data();

    unconst(m->pMachine) = aParent;
    m->pPeer = that;

    AutoWriteLock thatlock(that COMMA_LOCKVAL_SRC_POS);
    m->bd.share(that->m->bd);

    autoInitSpan.setSucceeded();

    LogFlowThisFuncLeave();
    return S_OK;
}

/**
 *  Initializes the guest object given another guest object
 *  (a kind of copy constructor). This object makes a private copy of data
 *  of the original object passed as an argument.
 */
HRESULT BIOSSettings::initCopy(Machine *aParent, BIOSSettings *that)
{
    LogFlowThisFuncEnter();
    LogFlowThisFunc(("aParent: %p, that: %p\n", aParent, that));

    ComAssertRet(aParent && that, E_INVALIDARG);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    m = new Data();

    unconst(m->pMachine) = aParent;
    // mPeer is left null

    AutoWriteLock thatlock(that COMMA_LOCKVAL_SRC_POS); /** @todo r=andy Shouldn't a read lock be sufficient here? */
    m->bd.attachCopy(that->m->bd);

    autoInitSpan.setSucceeded();

    LogFlowThisFuncLeave();
    return S_OK;
}

/**
 *  Uninitializes the instance and sets the ready flag to FALSE.
 *  Called either from FinalRelease() or by the parent when it gets destroyed.
 */
void BIOSSettings::uninit()
{
    LogFlowThisFuncEnter();

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    m->bd.free();

    unconst(m->pPeer) = NULL;
    unconst(m->pMachine) = NULL;

    delete m;
    m = NULL;

    LogFlowThisFuncLeave();
}

// IBIOSSettings properties
/////////////////////////////////////////////////////////////////////////////


HRESULT BIOSSettings::getLogoFadeIn(BOOL *enabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *enabled = m->bd->fLogoFadeIn;

    return S_OK;
}

HRESULT BIOSSettings::setLogoFadeIn(BOOL enable)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(m->pMachine);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->bd.backup();
    m->bd->fLogoFadeIn = RT_BOOL(enable);

    alock.release();
    AutoWriteLock mlock(m->pMachine COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
    m->pMachine->i_setModified(Machine::IsModified_BIOS);

    return S_OK;
}


HRESULT BIOSSettings::getLogoFadeOut(BOOL *enabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *enabled = m->bd->fLogoFadeOut;

    return S_OK;
}

HRESULT BIOSSettings::setLogoFadeOut(BOOL enable)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(m->pMachine);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->bd.backup();
    m->bd->fLogoFadeOut = RT_BOOL(enable);

    alock.release();
    AutoWriteLock mlock(m->pMachine COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
    m->pMachine->i_setModified(Machine::IsModified_BIOS);

    return S_OK;
}


HRESULT BIOSSettings::getLogoDisplayTime(ULONG *displayTime)
{
    if (!displayTime)
        return E_POINTER;

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *displayTime = m->bd->ulLogoDisplayTime;

    return S_OK;
}

HRESULT BIOSSettings::setLogoDisplayTime(ULONG displayTime)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(m->pMachine);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->bd.backup();
    m->bd->ulLogoDisplayTime = displayTime;

    alock.release();
    AutoWriteLock mlock(m->pMachine COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
    m->pMachine->i_setModified(Machine::IsModified_BIOS);

    return S_OK;
}


HRESULT BIOSSettings::getLogoImagePath(com::Utf8Str &imagePath)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    imagePath = m->bd->strLogoImagePath;
    return S_OK;
}

HRESULT BIOSSettings::setLogoImagePath(const com::Utf8Str &imagePath)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(m->pMachine);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->bd.backup();
    m->bd->strLogoImagePath = imagePath;

    alock.release();
    AutoWriteLock mlock(m->pMachine COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
    m->pMachine->i_setModified(Machine::IsModified_BIOS);

    return S_OK;
}

HRESULT BIOSSettings::getBootMenuMode(BIOSBootMenuMode_T *bootMenuMode)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *bootMenuMode = m->bd->biosBootMenuMode;
    return S_OK;
}

HRESULT BIOSSettings::setBootMenuMode(BIOSBootMenuMode_T bootMenuMode)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(m->pMachine);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->bd.backup();
    m->bd->biosBootMenuMode = bootMenuMode;

    alock.release();
    AutoWriteLock mlock(m->pMachine COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
    m->pMachine->i_setModified(Machine::IsModified_BIOS);

    return S_OK;
}


HRESULT BIOSSettings::getACPIEnabled(BOOL *enabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *enabled = m->bd->fACPIEnabled;

    return S_OK;
}

HRESULT BIOSSettings::setACPIEnabled(BOOL enable)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(m->pMachine);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->bd.backup();
    m->bd->fACPIEnabled = RT_BOOL(enable);

    alock.release();
    AutoWriteLock mlock(m->pMachine COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
    m->pMachine->i_setModified(Machine::IsModified_BIOS);

    return S_OK;
}


HRESULT BIOSSettings::getIOAPICEnabled(BOOL *aIOAPICEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aIOAPICEnabled = m->bd->fIOAPICEnabled;

    return S_OK;
}

HRESULT BIOSSettings::setIOAPICEnabled(BOOL aIOAPICEnabled)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(m->pMachine);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->bd.backup();
    m->bd->fIOAPICEnabled = RT_BOOL(aIOAPICEnabled);

    alock.release();
    AutoWriteLock mlock(m->pMachine COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
    m->pMachine->i_setModified(Machine::IsModified_BIOS);

    return S_OK;
}


HRESULT BIOSSettings::getAPICMode(APICMode_T *aAPICMode)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAPICMode = m->bd->apicMode;

    return S_OK;
}

HRESULT BIOSSettings::setAPICMode(APICMode_T aAPICMode)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(m->pMachine);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->bd.backup();
    m->bd->apicMode = aAPICMode;

    alock.release();
    AutoWriteLock mlock(m->pMachine COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
    m->pMachine->i_setModified(Machine::IsModified_BIOS);

    return S_OK;
}


HRESULT BIOSSettings::getPXEDebugEnabled(BOOL *enabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *enabled = m->bd->fPXEDebugEnabled;

    return S_OK;
}

HRESULT BIOSSettings::setPXEDebugEnabled(BOOL enable)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(m->pMachine);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->bd.backup();
    m->bd->fPXEDebugEnabled = RT_BOOL(enable);

    alock.release();
    AutoWriteLock mlock(m->pMachine COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
    m->pMachine->i_setModified(Machine::IsModified_BIOS);

    return S_OK;
}


HRESULT BIOSSettings::getTimeOffset(LONG64 *offset)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *offset = m->bd->llTimeOffset;

    return S_OK;
}

HRESULT BIOSSettings::setTimeOffset(LONG64 offset)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(m->pMachine);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->bd.backup();
    m->bd->llTimeOffset = offset;

    alock.release();
    AutoWriteLock mlock(m->pMachine COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
    m->pMachine->i_setModified(Machine::IsModified_BIOS);

    return S_OK;
}


HRESULT BIOSSettings::getSMBIOSUuidLittleEndian(BOOL *enabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *enabled = m->bd->fSmbiosUuidLittleEndian;

    return S_OK;
}

HRESULT BIOSSettings::setSMBIOSUuidLittleEndian(BOOL enable)
{
    /* the machine needs to be mutable */
    AutoMutableStateDependency adep(m->pMachine);
    if (FAILED(adep.hrc())) return adep.hrc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->bd.backup();
    m->bd->fSmbiosUuidLittleEndian = RT_BOOL(enable);

    alock.release();
    AutoWriteLock mlock(m->pMachine COMMA_LOCKVAL_SRC_POS);  // mParent is const, needs no locking
    m->pMachine->i_setModified(Machine::IsModified_BIOS);

    return S_OK;
}


// IBIOSSettings methods
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
HRESULT BIOSSettings::i_loadSettings(const settings::BIOSSettings &data)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.hrc());

    AutoReadLock mlock(m->pMachine COMMA_LOCKVAL_SRC_POS);
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    // simply copy
    m->bd.assignCopy(&data);
    return S_OK;
}

/**
 *  Saves settings to the given machine node.
 *
 *  @param data Configuration settings.
 *
 *  @note Locks this object for reading.
 */
HRESULT BIOSSettings::i_saveSettings(settings::BIOSSettings &data)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.hrc());

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    data = *m->bd.data();

    return S_OK;
}

void BIOSSettings::i_rollback()
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    m->bd.rollback();
}

void BIOSSettings::i_commit()
{
    /* sanity */
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.hrc());

    /* sanity too */
    AutoCaller peerCaller(m->pPeer);
    AssertComRCReturnVoid(peerCaller.hrc());

    /* lock both for writing since we modify both (mPeer is "master" so locked
     * first) */
    AutoMultiWriteLock2 alock(m->pPeer, this COMMA_LOCKVAL_SRC_POS);

    if (m->bd.isBackedUp())
    {
        m->bd.commit();
        if (m->pPeer)
        {
            /* attach new data to the peer and reshare it */
            AutoWriteLock peerlock(m->pPeer COMMA_LOCKVAL_SRC_POS);
            m->pPeer->m->bd.attach(m->bd);
        }
    }
}

void BIOSSettings::i_copyFrom(BIOSSettings *aThat)
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
    m->bd.assignCopy(aThat->m->bd);
}

void BIOSSettings::i_applyDefaults(GuestOSType *aOsType)
{
    /* sanity */
    AutoCaller autoCaller(this);
    AssertComRCReturnVoid(autoCaller.hrc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Initialize default BIOS settings here */
    if (aOsType)
        m->bd->fIOAPICEnabled = aOsType->i_recommendedIOAPIC();
    else
        m->bd->fIOAPICEnabled = true;
}

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
