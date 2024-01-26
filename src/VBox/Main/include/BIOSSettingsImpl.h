/* $Id: BIOSSettingsImpl.h $ */

/** @file
 *
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

#ifndef MAIN_INCLUDED_BIOSSettingsImpl_h
#define MAIN_INCLUDED_BIOSSettingsImpl_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "BIOSSettingsWrap.h"

class GuestOSType;

namespace settings
{
    struct BIOSSettings;
}

class ATL_NO_VTABLE BIOSSettings :
    public BIOSSettingsWrap
{
public:

    DECLARE_COMMON_CLASS_METHODS(BIOSSettings)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(Machine *parent);
    HRESULT init(Machine *parent, BIOSSettings *that);
    HRESULT initCopy(Machine *parent, BIOSSettings *that);
    void uninit();

    // public methods for internal purposes only
    HRESULT i_loadSettings(const settings::BIOSSettings &data);
    HRESULT i_saveSettings(settings::BIOSSettings &data);

    void i_rollback();
    void i_commit();
    void i_copyFrom(BIOSSettings *aThat);
    void i_applyDefaults(GuestOSType *aOsType);

private:

    // wrapped IBIOSettings properties
    HRESULT getLogoFadeIn(BOOL *enabled);
    HRESULT setLogoFadeIn(BOOL enable);
    HRESULT getLogoFadeOut(BOOL *enabled);
    HRESULT setLogoFadeOut(BOOL enable);
    HRESULT getLogoDisplayTime(ULONG *displayTime);
    HRESULT setLogoDisplayTime(ULONG displayTime);
    HRESULT getLogoImagePath(com::Utf8Str &imagePath);
    HRESULT setLogoImagePath(const com::Utf8Str &imagePath);
    HRESULT getBootMenuMode(BIOSBootMenuMode_T *bootMenuMode);
    HRESULT setBootMenuMode(BIOSBootMenuMode_T bootMenuMode);
    HRESULT getACPIEnabled(BOOL *enabled);
    HRESULT setACPIEnabled(BOOL enable);
    HRESULT getIOAPICEnabled(BOOL *aIOAPICEnabled);
    HRESULT setIOAPICEnabled(BOOL aIOAPICEnabled);
    HRESULT getAPICMode(APICMode_T *aAPICMode);
    HRESULT setAPICMode(APICMode_T aAPICMode);
    HRESULT getTimeOffset(LONG64 *offset);
    HRESULT setTimeOffset(LONG64 offset);
    HRESULT getPXEDebugEnabled(BOOL *enabled);
    HRESULT setPXEDebugEnabled(BOOL enable);
    HRESULT getSMBIOSUuidLittleEndian(BOOL *enabled);
    HRESULT setSMBIOSUuidLittleEndian(BOOL enable);

    struct Data;
    Data *m;
};

#endif /* !MAIN_INCLUDED_BIOSSettingsImpl_h */

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
