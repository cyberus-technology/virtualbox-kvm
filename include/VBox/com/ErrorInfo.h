/** @file
 * MS COM / XPCOM Abstraction Layer - ErrorInfo class declaration.
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

#ifndef VBOX_INCLUDED_com_ErrorInfo_h
#define VBOX_INCLUDED_com_ErrorInfo_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "VBox/com/ptr.h"
#include "VBox/com/string.h"
#include "VBox/com/Guid.h"
#include "VBox/com/assert.h"


/** @defgroup grp_com_errinfo   ErrorInfo Classes
 * @ingroup grp_com
 * @{
 */

COM_STRUCT_OR_CLASS(IProgress);
COM_STRUCT_OR_CLASS(IVirtualBoxErrorInfo);

namespace com
{

/**
 * General discussion:
 *
 * In COM all errors are stored on a per thread basis. In general this means
 * only _one_ active error is possible per thread. A new error will overwrite
 * the previous one. To prevent this use MultiResult or ErrorInfoKeeper (see
 * below). The implementations in MSCOM/XPCOM differ slightly, but the details
 * are handled by this glue code.
 *
 * We have different classes which are involved in the error management. I try
 * to describe them separately to make clear what they are there for.
 *
 * ErrorInfo:
 *
 *  This class is able to retrieve the per thread error and store it into its
 *  member variables. This class can also handle non-VirtualBox errors (like
 *  standard COM errors).
 *
 * ProgressErrorInfo:
 *
 *  This is just a simple wrapper class to get the ErrorInfo stored within an
 *  IProgress object. That is the error which was stored when the progress
 *  object was in use and not an error produced by IProgress itself.
 *
 * IVirtualBoxErrorInfo:
 *
 *  The VirtualBox interface class for accessing error information from Main
 *  clients. This class is also used for storing the error information in the
 *  thread context.
 *
 * ErrorInfoKeeper:
 *
 *  A helper class which stores the current per thread info internally. After
 *  calling methods which may produce other errors it is possible to restore
 *  the previous error and therefore restore the situation before calling the
 *  other methods.
 *
 * MultiResult:
 *
 *  Creating an instance of MultiResult turns error chain saving on. All errors
 *  which follow will be saved in a chain for later access.
 *
 * COMErrorInfo (Qt/Gui only):
 *
 *  The Qt GUI does some additional work for saving errors. Because we create
 *  wrappers for _every_ COM call, it is possible to automatically save the
 *  error info after the execution. This allow some additional info like saving
 *  the callee. Please note that this error info is saved on the client side
 *  and therefore locally to the object instance. See COMBaseWithEI,
 *  COMErrorInfo and the generated COMWrappers.cpp in the GUI.
 *
 * Errors itself are set in VirtualBoxBase::setErrorInternal. First a
 * IVirtualBoxErrorInfo object is created and the given error is saved within.
 * If MultiResult is active the current per thread error is fetched and
 * attached to the new created IVirtualBoxErrorInfo object. Next this object is
 * set as the new per thread error.
 *
 * Some general hints:
 *
 * - Always use setError, especially when you are working in an asynchronous thread
 *   to indicate an error. Otherwise the error information itself will not make
 *   it into the client.
 *
 */

/**
 *  The ErrorInfo class provides a convenient way to retrieve error
 *  information set by the most recent interface method, that was invoked on
 *  the current thread and returned an unsuccessful result code.
 *
 *  Once the instance of this class is created, the error information for
 *  the current thread is cleared.
 *
 *  There is no sense to use instances of this class after the last
 *  invoked interface method returns a success.
 *
 *  The class usage pattern is as follows:
 *  <code>
 *      IFoo *foo;
 *      ...
 *      HRESULT rc = foo->SomeMethod();
 *      if (FAILED(rc)) {
 *          ErrorInfo info(foo);
 *          if (info.isFullAvailable()) {
 *              printf("error message = %ls\n", info.getText().raw());
 *          }
 *      }
 *  </code>
 *
 *  This class fetches error information using the IErrorInfo interface on
 *  Win32 (MS COM) or the nsIException interface on other platforms (XPCOM),
 *  or the extended IVirtualBoxErrorInfo interface when when it is available
 *  (i.e. a given IErrorInfo or nsIException instance implements it).
 *  Currently, IVirtualBoxErrorInfo is only available for VirtualBox components.
 *
 *  ErrorInfo::isFullAvailable() and ErrorInfo::isBasicAvailable() determine
 *  what level of error information is available. If #isBasicAvailable()
 *  returns true, it means that only IErrorInfo or nsIException is available as
 *  the source of information (depending on the platform), but not
 *  IVirtualBoxErrorInfo. If #isFullAvailable() returns true, it means that all
 *  three interfaces are available. If both methods return false, no error info
 *  is available at all.
 *
 *  Here is a table of correspondence between this class methods and
 *  and IErrorInfo/nsIException/IVirtualBoxErrorInfo attributes/methods:
 *
 *  ErrorInfo       IErrorInfo      nsIException    IVirtualBoxErrorInfo
 *  --------------------------------------------------------------------
 *  getResultCode   --              result          resultCode
 *  getIID          GetGUID         --              interfaceID
 *  getComponent    GetSource       --              component
 *  getText         GetDescription  message         text
 *
 *  '--' means that this interface does not provide the corresponding portion
 *  of information, therefore it is useless to query it if only
 *  #isBasicAvailable() returns true. As it can be seen, the amount of
 *  information provided at the basic level, depends on the platform
 *  (MS COM or XPCOM).
 */
class ErrorInfo
{
public:

    /**
     *  Constructs a new, "interfaceless" ErrorInfo instance that takes
     *  the error information possibly set on the current thread by an
     *  interface method of some COM component or by the COM subsystem.
     *
     *  This constructor is useful, for example, after an unsuccessful attempt
     *  to instantiate (create) a component, so there is no any valid interface
     *  pointer available.
     */
    explicit ErrorInfo()
        : mIsBasicAvailable(false),
          mIsFullAvailable(false),
          mResultCode(S_OK),
          mResultDetail(0),
          m_pNext(NULL)
    {
        init();
    }

    ErrorInfo(IUnknown *pObj, const GUID &aIID)
        : mIsBasicAvailable(false),
          mIsFullAvailable(false),
          mResultCode(S_OK),
          mResultDetail(0),
          m_pNext(NULL)
    {
        init(pObj, aIID);
    }

    /** Specialization for the IVirtualBoxErrorInfo smart pointer */
    ErrorInfo(const ComPtr<IVirtualBoxErrorInfo> &aPtr)
        : mIsBasicAvailable(false), mIsFullAvailable(false)
        , mResultCode(S_OK), mResultDetail(0)
        { init(aPtr); }

    /**
     *  Constructs a new ErrorInfo instance from the IVirtualBoxErrorInfo
     *  interface pointer. If this pointer is not NULL, both #isFullAvailable()
     *  and #isBasicAvailable() will return |true|.
     *
     *  @param aInfo    pointer to the IVirtualBoxErrorInfo interface that
     *                  holds error info to be fetched by this instance
     */
    ErrorInfo(IVirtualBoxErrorInfo *aInfo)
        : mIsBasicAvailable(false), mIsFullAvailable(false)
        , mResultCode(S_OK), mResultDetail(0)
        { init(aInfo); }

    ErrorInfo(const ErrorInfo &x)
    {
        copyFrom(x);
    }

    virtual ~ErrorInfo()
    {
        cleanup();
    }

    ErrorInfo& operator=(const ErrorInfo& x)
    {
        cleanup();
        copyFrom(x);
        return *this;
    }

    /**
     *  Returns whether basic error info is actually available for the current
     *  thread. If the instance was created from an interface pointer that
     *  supports basic error info and successfully provided it, or if it is an
     *  "interfaceless" instance and there is some error info for the current
     *  thread, the returned value will be true.
     *
     *  See the class description for details about the basic error info level.
     *
     *  The appropriate methods of this class provide meaningful info only when
     *  this method returns true (otherwise they simply return NULL-like values).
     */
    bool isBasicAvailable() const
    {
        return mIsBasicAvailable;
    }

    /**
     *  Returns whether full error info is actually available for the current
     *  thread. If the instance was created from an interface pointer that
     *  supports full error info and successfully provided it, or if it is an
     *  "interfaceless" instance and there is some error info for the current
     *  thread, the returned value will be true.
     *
     *  See the class description for details about the full error info level.
     *
     *  The appropriate methods of this class provide meaningful info only when
     *  this method returns true (otherwise they simply return NULL-like values).
     */
    bool isFullAvailable() const
    {
        return mIsFullAvailable;
    }

    /**
     *  Returns the COM result code of the failed operation.
     */
    HRESULT getResultCode() const
    {
        return mResultCode;
    }

    /**
     *  Returns the (optional) result detail code of the failed operation.
     */
    LONG getResultDetail() const
    {
        return mResultDetail;
    }

    /**
     *  Returns the IID of the interface that defined the error.
     */
    const Guid& getInterfaceID() const
    {
        return mInterfaceID;
    }

    /**
     *  Returns the name of the component that generated the error.
     */
    const Bstr& getComponent() const
    {
        return mComponent;
    }

    /**
     *  Returns the textual description of the error.
     */
    const Bstr& getText() const
    {
        return mText;
    }

    /**
     *  Returns the next error information object or @c NULL if there is none.
     */
    const ErrorInfo* getNext() const
    {
        return m_pNext;
    }

    /**
     *  Returns the name of the interface that defined the error
     */
    const Bstr& getInterfaceName() const
    {
        return mInterfaceName;
    }

    /**
     *  Returns the IID of the interface that returned the error.
     *
     *  This method returns a non-null IID only if the instance was created
     *  using template \<class I\> ErrorInfo(I *i) or
     *  template \<class I\> ErrorInfo(const ComPtr<I> &i) constructor.
     *
     *  @todo broken ErrorInfo documentation links, possibly misleading.
     */
    const Guid& getCalleeIID() const
    {
        return mCalleeIID;
    }

    /**
     *  Returns the name of the interface that returned the error
     *
     *  This method returns a non-null name only if the instance was created
     *  using template \<class I\> ErrorInfo(I *i) or
     *  template \<class I\> ErrorInfo(const ComPtr<I> &i) constructor.
     *
     *  @todo broken ErrorInfo documentation links, possibly misleading.
     */
    const Bstr& getCalleeName() const
    {
        return mCalleeName;
    }

    HRESULT getVirtualBoxErrorInfo(ComPtr<IVirtualBoxErrorInfo> &pVirtualBoxErrorInfo);

    /**
     *  Resets all collected error information. #isBasicAvailable() and
     *  #isFullAvailable will return @c true after this method is called.
     */
    void setNull()
    {
        cleanup();
    }

protected:

    ErrorInfo(bool /* aDummy */)
        : mIsBasicAvailable(false),
          mIsFullAvailable(false),
          mResultCode(S_OK),
          m_pNext(NULL)
    { }

    void copyFrom(const ErrorInfo &x);
    void cleanup();

    void init(bool aKeepObj = false);
    void init(IUnknown *aUnk, const GUID &aIID, bool aKeepObj = false);
    void init(IVirtualBoxErrorInfo *aInfo);

    bool mIsBasicAvailable : 1;
    bool mIsFullAvailable : 1;

    HRESULT mResultCode;
    LONG    mResultDetail;
    Guid    mInterfaceID;
    Bstr    mComponent;
    Bstr    mText;

    ErrorInfo *m_pNext;

    Bstr mInterfaceName;
    Guid mCalleeIID;
    Bstr mCalleeName;

    ComPtr<IUnknown> mErrorInfo;
};

/**
 *  A convenience subclass of ErrorInfo that, given an IProgress interface
 *  pointer, reads its errorInfo attribute and uses the returned
 *  IVirtualBoxErrorInfo instance to construct itself.
 */
class ProgressErrorInfo : public ErrorInfo
{
public:

    /**
     *  Constructs a new instance by fetching error information from the
     *  IProgress interface pointer. If the progress object is not NULL,
     *  its completed attribute is true, resultCode represents a failure,
     *  and the errorInfo attribute returns a valid IVirtualBoxErrorInfo pointer,
     *  both #isFullAvailable() and #isBasicAvailable() will return true.
     *
     *  @param  progress    the progress object representing a failed operation
     */
    ProgressErrorInfo(IProgress *progress);
};

/**
 *  A convenience subclass of ErrorInfo that allows to preserve the current
 *  error info. Instances of this class fetch an error info object set on the
 *  current thread and keep a reference to it, which allows to restore it
 *  later using the #restore() method. This is useful to preserve error
 *  information returned by some method for the duration of making another COM
 *  call that may set its own error info and overwrite the existing
 *  one. Preserving and restoring error information makes sense when some
 *  method wants to return error information set by other call as its own
 *  error information while it still needs to make another call before return.
 *
 *  Instead of calling #restore() explicitly you may let the object destructor
 *  do it for you, if you correctly limit the object's lifetime.
 *
 *  The usage pattern is:
 *  <code>
 *      rc = foo->method();
 *      if (FAILED(rc))
 *      {
 *           ErrorInfoKeeper eik;
 *           ...
 *           // bar may return error info as well
 *           bar->method();
 *           ...
 *           // no need to call #restore() explicitly here because the eik's
 *           // destructor will restore error info fetched after the failed
 *           // call to foo before returning to the caller
 *           return rc;
 *      }
 *  </code>
 */
class ErrorInfoKeeper : public ErrorInfo
{
public:

    /**
     *  Constructs a new instance that will fetch the current error info if
     *  @a aIsNull is @c false (by default) or remain uninitialized (null)
     *  otherwise.
     *
     *  @param aIsNull  @c true to prevent fetching error info and leave
     *                  the instance uninitialized.
     */
    ErrorInfoKeeper(bool aIsNull = false)
        : ErrorInfo(false), mForgot(aIsNull)
    {
        if (!aIsNull)
            init(true /* aKeepObj */);
    }

    /**
     *  Constructs a new instance from an ErrorInfo object, to inject a full
     *  error info created elsewhere.
     *
     *  @param aInfo    @c true to prevent fetching error info and leave
     *                  the instance uninitialized.
     */
    ErrorInfoKeeper(const ErrorInfo &aInfo)
        : ErrorInfo(false), mForgot(false)
    {
        copyFrom(aInfo);
    }

    /**
     *  Destroys this instance and automatically calls #restore() which will
     *  either restore error info fetched by the constructor or do nothing
     *  if #forget() was called before destruction.
     */
    ~ErrorInfoKeeper() { if (!mForgot) restore(); }

    /**
     *  Tries to (re-)fetch error info set on the current thread.  On success,
     *  the previous error information, if any, will be overwritten with the
     *  new error information. On failure, or if there is no error information
     *  available, this instance will be reset to null.
     */
    void fetch()
    {
        setNull();
        mForgot = false;
        init(true /* aKeepObj */);
    }

    /**
     *  Restores error info fetched by the constructor and forgets it
     *  afterwards. Does nothing if the error info was forgotten by #forget().
     *
     *  @return COM result of the restore operation.
     */
    HRESULT restore();

    /**
     *  Forgets error info fetched by the constructor to prevent it from
     *  being restored by #restore() or by the destructor.
     */
    void forget() { mForgot = true; }

    /**
     *  Forgets error info fetched by the constructor to prevent it from
     *  being restored by #restore() or by the destructor, and returns the
     *  stored error info object to the caller.
     */
    ComPtr<IUnknown> takeError() { mForgot = true; return mErrorInfo; }

private:

    bool mForgot : 1;
};

} /* namespace com */

/** @} */

#endif /* !VBOX_INCLUDED_com_ErrorInfo_h */

