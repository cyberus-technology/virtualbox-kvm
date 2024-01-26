/* $Id: ProgressProxyImpl.h $ */
/** @file
 * IProgress implementation for Machine::LaunchVMProcess in VBoxSVC.
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

#ifndef MAIN_INCLUDED_ProgressProxyImpl_h
#define MAIN_INCLUDED_ProgressProxyImpl_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "ProgressImpl.h"
#include "AutoCaller.h"


/**
 * The ProgressProxy class allows proxying the important Progress calls and
 * attributes to a different IProgress object for a period of time.
 */
class ATL_NO_VTABLE ProgressProxy :
    public Progress
{
public:
    VIRTUALBOXBASE_ADD_ERRORINFO_SUPPORT(ProgressProxy, IProgress)

    DECLARE_NOT_AGGREGATABLE(ProgressProxy)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(ProgressProxy)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY(IProgress)
        COM_INTERFACE_ENTRY2(IDispatch, IProgress)
        VBOX_TWEAK_INTERFACE_ENTRY(IProgress)
    END_COM_MAP()

    HRESULT FinalConstruct();
    void    FinalRelease();
    HRESULT init(
#ifndef VBOX_COM_INPROC
                 VirtualBox *pParent,
#endif
                 IUnknown *pInitiator,
                 Utf8Str strDescription,
                 BOOL fCancelable);
    HRESULT init(
#ifndef VBOX_COM_INPROC
                 VirtualBox *pParent,
#endif
                 IUnknown *pInitiator,
                 Utf8Str strDescription,
                 BOOL fCancelable,
                 ULONG uTotalOperationsWeight,
                 Utf8Str strFirstOperationDescription,
                 ULONG uFirstOperationWeight,
                 ULONG cOtherProgressObjectOperations);
    void    uninit();

    // IProgress properties
    STDMETHOD(COMGETTER(Cancelable))(BOOL *aCancelable);
    STDMETHOD(COMGETTER(Percent))(ULONG *aPercent);
    STDMETHOD(COMGETTER(TimeRemaining))(LONG *aTimeRemaining);
    STDMETHOD(COMGETTER(Completed))(BOOL *aCompleted);
    STDMETHOD(COMGETTER(Canceled))(BOOL *aCanceled);
    STDMETHOD(COMGETTER(ResultCode))(LONG *aResultCode);
    STDMETHOD(COMGETTER(ErrorInfo))(IVirtualBoxErrorInfo **aErrorInfo);
    //STDMETHOD(COMGETTER(OperationCount))(ULONG *aOperationCount); - not necessary
    STDMETHOD(COMGETTER(Operation))(ULONG *aOperation);
    STDMETHOD(COMGETTER(OperationDescription))(BSTR *aOperationDescription);
    STDMETHOD(COMGETTER(OperationPercent))(ULONG *aOperationPercent);
    STDMETHOD(COMSETTER(Timeout))(ULONG aTimeout);
    STDMETHOD(COMGETTER(Timeout))(ULONG *aTimeout);

    // IProgress methods
    STDMETHOD(WaitForCompletion)(LONG aTimeout);
    STDMETHOD(WaitForOperationCompletion)(ULONG aOperation, LONG aTimeout);
    STDMETHOD(Cancel)();
    STDMETHOD(SetCurrentOperationProgress)(ULONG aPercent);
    STDMETHOD(SetNextOperation)(IN_BSTR bstrNextOperationDescription, ULONG ulNextOperationsWeight);

    // public methods only for internal purposes

    HRESULT notifyComplete(HRESULT aResultCode);
    HRESULT notifyComplete(HRESULT aResultCode,
                           const GUID &aIID,
                           const char *pcszComponent,
                           const char *aText, ...);
    bool setOtherProgressObject(IProgress *pOtherProgress);

protected:
    void clearOtherProgressObjectInternal(bool fEarly);
    void copyProgressInfo(IProgress *pOtherProgress, bool fEarly);

private:
    /** The other progress object.  This can be NULL. */
    ComPtr<IProgress> mptrOtherProgress;
    /** Set if the other progress object has multiple operations. */
    bool mfMultiOperation;
    /** The weight the other progress object started at. */
    ULONG muOtherProgressStartWeight;
    /** The weight of other progress object. */
    ULONG muOtherProgressWeight;
    /** The operation number the other progress object started at. */
    ULONG muOtherProgressStartOperation;

};

#endif /* !MAIN_INCLUDED_ProgressProxyImpl_h */

