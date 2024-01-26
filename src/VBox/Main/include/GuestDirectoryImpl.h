/* $Id: GuestDirectoryImpl.h $ */
/** @file
 * VirtualBox Main - Guest directory handling implementation.
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

#ifndef MAIN_INCLUDED_GuestDirectoryImpl_h
#define MAIN_INCLUDED_GuestDirectoryImpl_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "GuestDirectoryWrap.h"
#include "GuestFsObjInfoImpl.h"
#include "GuestProcessImpl.h"

class GuestSession;

/**
 * TODO
 */
class ATL_NO_VTABLE GuestDirectory :
    public GuestDirectoryWrap,
    public GuestObject
{
public:
    /** @name COM and internal init/term/mapping cruft.
     * @{ */
    DECLARE_COMMON_CLASS_METHODS(GuestDirectory)

    int     init(Console *pConsole, GuestSession *pSession, ULONG aObjectID, const GuestDirectoryOpenInfo &openInfo);
    void    uninit(void);

    HRESULT FinalConstruct(void);
    void    FinalRelease(void);
    /** @}  */

public:
    /** @name Implemented virtual methods from GuestObject.
     * @{ */
    int            i_callbackDispatcher(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCb);
    int            i_onUnregister(void);
    int            i_onSessionStatusChange(GuestSessionStatus_T enmSessionStatus);
    /** @}  */

public:
    /** @name Public internal methods.
     * @{ */
    int            i_closeInternal(int *pvrcGuest);
    int            i_read(ComObjPtr<GuestFsObjInfo> &fsObjInfo, int *pvrcGuest);
    int            i_readInternal(GuestFsObjData &objData, int *prcGuest);
    /** @}  */

public:
    /** @name Public static internal methods.
     * @{ */
    static Utf8Str i_guestErrorToString(int vrcGuest, const char *pcszWhat);
    /** @}  */

private:

    /** Wrapped @name IGuestDirectory properties
     * @{ */
    HRESULT getDirectoryName(com::Utf8Str &aDirectoryName);
    HRESULT getFilter(com::Utf8Str &aFilter);
    /** @}  */

    /** Wrapped @name IGuestDirectory methods.
     * @{ */
    HRESULT close();
    HRESULT read(ComPtr<IFsObjInfo> &aObjInfo);
    /** @}  */

    struct Data
    {
        /** The directory's open info. */
        GuestDirectoryOpenInfo     mOpenInfo;
        /** The process tool instance to use. */
        GuestProcessTool           mProcessTool;
        /** Object data cache.
         *  Its mName attribute acts as a beacon if the cache is valid or not. */
        GuestFsObjData             mObjData;
    } mData;
};

#endif /* !MAIN_INCLUDED_GuestDirectoryImpl_h */

