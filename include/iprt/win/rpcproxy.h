/** @file
 * Safe way to include rpcproxy.h (not C++ clean).
 */

/*
 * Copyright (C) 2016-2023 Oracle and/or its affiliates.
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_win_rpcproxy_h
#define IPRT_INCLUDED_win_rpcproxy_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <objbase.h>

#ifdef __cplusplus

typedef struct IRpcStubBufferVtbl
{
    BEGIN_INTERFACE
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IRpcStubBuffer *, REFIID, void **);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IRpcStubBuffer *);
    ULONG   (STDMETHODCALLTYPE *Release)(IRpcStubBuffer *);
    HRESULT (STDMETHODCALLTYPE *Connect)(IRpcStubBuffer *, IUnknown *);
    void    (STDMETHODCALLTYPE *Disconnect)(IRpcStubBuffer *);
    HRESULT (STDMETHODCALLTYPE *Invoke)(IRpcStubBuffer *, RPCOLEMESSAGE *, IRpcChannelBuffer *);
    IRpcStubBuffer * (STDMETHODCALLTYPE *IsIIDSupported)(IRpcStubBuffer *, REFIID);
    ULONG   (STDMETHODCALLTYPE *CountRefs)(IRpcStubBuffer *);
    HRESULT (STDMETHODCALLTYPE *DebugServerQueryInterface)(IRpcStubBuffer *, void **);
    void    (STDMETHODCALLTYPE *DebugServerRelease)(IRpcStubBuffer *, void *);
} IRpcStubBufferVtbl;

typedef struct IPSFactoryBufferVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IPSFactoryBuffer *, REFIID, void **);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IPSFactoryBuffer *);
    ULONG   (STDMETHODCALLTYPE *Release)(IPSFactoryBuffer *);
    HRESULT (STDMETHODCALLTYPE *CreateProxy)(IPSFactoryBuffer *, IUnknown *, REFIID riid, IRpcProxyBuffer **, void **);
    HRESULT (STDMETHODCALLTYPE *CreateStub)(IPSFactoryBuffer *, REFIID, IUnknown *, IRpcStubBuffer **);
} IPSFactoryBufferVtbl;

#endif /* __cplusplus */

#include <rpcproxy.h>

#endif /* !IPRT_INCLUDED_win_rpcproxy_h */

