/** @file
 * MS COM / XPCOM Abstraction Layer - Smart COM pointer classes declaration.
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

#ifndef VBOX_INCLUDED_com_ptr_h
#define VBOX_INCLUDED_com_ptr_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Make sure all the stdint.h macros are included - must come first! */
#ifndef __STDC_LIMIT_MACROS
# define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_CONSTANT_MACROS
# define __STDC_CONSTANT_MACROS
#endif

#ifdef VBOX_WITH_XPCOM
# include <nsISupportsUtils.h>
#endif /* VBOX_WITH_XPCOM */

#include <VBox/com/defs.h>
#include <new> /* For bad_alloc. */


/** @defgroup grp_com_ptr   Smart COM Pointer Classes
 * @ingroup grp_com
 * @{
 */

#ifdef VBOX_WITH_XPCOM

namespace com
{
// declare a couple of XPCOM helper methods (defined in glue/com.cpp)
// so we don't have to include a ton of XPCOM implementation headers here
HRESULT GlueCreateObjectOnServer(const CLSID &clsid,
                                 const char *serverName,
                                 const nsIID &id,
                                 void **ppobj);
HRESULT GlueCreateInstance(const CLSID &clsid,
                           const nsIID &id,
                           void **ppobj);
}

#endif // VBOX_WITH_XPCOM

/**
 *  COM autopointer class which takes care of all required reference counting.
 *
 *  This automatically calls the required basic COM methods on COM pointers
 *  given to it:
 *
 *  --  AddRef() gets called automatically whenever a new COM pointer is assigned
 *      to the ComPtr instance (either in the copy constructor or by assignment);
 *
 *  --  Release() gets called automatically by the destructor and when an existing
 *      object gets released in assignment;
 *
 *  --  QueryInterface() gets called automatically when COM pointers get converted
 *      from one interface to another.
 *
 *  Example usage:
 *
 *  @code
 *
 *  {
 *      ComPtr<IMachine> pMachine = findMachine("blah");         // calls AddRef()
 *      ComPtr<IUnknown> pUnknown = pMachine;                    // calls QueryInterface()
 *  }           # ComPtr destructor of both instances calls Release()
 *
 *  @endcode
 */
template <class T>
class ComPtr
{
public:

    /**
     * Default constructor, sets up a NULL pointer.
     */
    ComPtr()
        : m_p(NULL)
    { }

    /**
     * Destructor. Calls Release() on the contained COM object.
     */
    ~ComPtr()
    {
        cleanup();
    }

    /**
     * Copy constructor from another ComPtr of any interface.
     *
     * This calls QueryInterface(T) and can result in a NULL pointer if the input
     * pointer p does not support the ComPtr interface T.
     *
     * Does not call AddRef explicitly because if QueryInterface succeeded, then
     * the refcount will have been increased by one already.
     */
    template <class T2>
    ComPtr(const ComPtr<T2> &that)
    {
        m_p = NULL;
        if (!that.isNull())
            that->QueryInterface(COM_IIDOF(T), (void **)&m_p);
    }

    /**
     * Specialization: copy constructor from another ComPtr<T>. Calls AddRef().
     */
    ComPtr(const ComPtr &that)
    {
        copyFrom(that.m_p);
    }

    /**
     * Copy constructor from another interface pointer of any interface.
     *
     * This calls QueryInterface(T) and can result in a NULL pointer if the input
     * pointer p does not support the ComPtr interface T.
     *
     * Does not call AddRef explicitly because if QueryInterface succeeded, then
     * the refcount will have been increased by one already.
     */
    template <class T2>
    ComPtr(T2 *p)
    {
        m_p = NULL;
        if (p)
            p->QueryInterface(COM_IIDOF(T), (void **)&m_p);
    }

    /**
     * Specialization: copy constructor from a plain T * pointer. Calls AddRef().
     */
    ComPtr(T *that_p)
    {
        copyFrom(that_p);
    }

    /**
     * Assignment from another ComPtr of any interface.
     *
     * This calls QueryInterface(T) and can result in a NULL pointer if the input
     * pointer p does not support the ComPtr interface T.
     *
     * Does not call AddRef explicitly because if QueryInterface succeeded, then
     * the refcount will have been increased by one already.
     */
    template <class T2>
    ComPtr& operator=(const ComPtr<T2> &that)
    {
        return operator=((T2 *)that);
    }

    /**
     * Specialization of the previous: assignment from another ComPtr<T>.
     * Calls Release() on the previous member pointer, if any, and AddRef() on the new one.
     */
    ComPtr& operator=(const ComPtr &that)
    {
        return operator=((T *)that);
    }

    /**
     * Assignment from another interface pointer of any interface.
     *
     * This calls QueryInterface(T) and can result in a NULL pointer if the input
     * pointer p does not support the ComPtr interface T.
     *
     * Does not call AddRef explicitly because if QueryInterface succeeded, then
     * the refcount will have been increased by one already.
     */
    template <class T2>
    ComPtr& operator=(T2 *p)
    {
        cleanup();
        if (p)
            p->QueryInterface(COM_IIDOF(T), (void **)&m_p);
        return *this;
    }

    /**
     * Specialization of the previous: assignment from a plain T * pointer.
     * Calls Release() on the previous member pointer, if any, and AddRef() on the new one.
     */
    ComPtr& operator=(T *p)
    {
        cleanup();
        copyFrom(p);
        return *this;
    }

    /**
     * Resets the ComPtr to NULL. Works like a NULL assignment except it avoids the templates.
     */
    void setNull()
    {
        cleanup();
    }

    /**
     * Returns true if the pointer is NULL.
     */
    bool isNull() const
    {
        return (m_p == NULL);
    }

    /**
     * Returns true if the pointer is not NULL.
     */
    bool isNotNull() const
    {
        return (m_p != NULL);
    }

    bool operator<(T *p) const
    {
        return m_p < p;
    }

    /**
     * Conversion operator, most often used to pass ComPtr instances as
     * parameters to COM method calls.
     */
    operator T *() const
    {
        return m_p;
    }

    /**
     *  Dereferences the instance (redirects the -> operator to the managed
     *  pointer).
     */
    T *operator->() const
    {
        return m_p;
    }

    /**
     * Special method which allows using a ComPtr as an output argument of a COM method.
     * The ComPtr will then accept the method's interface pointer without calling AddRef()
     * itself, since by COM convention this must has been done by the method which created
     * the object that is being accepted.
     *
     * The ComPtr destructor will then still invoke Release() so that the returned object
     * can get cleaned up properly.
     */
    T **asOutParam()
    {
        cleanup();
        return &m_p;
    }

    /**
     * Converts the contained pointer to a different interface
     * by calling QueryInterface() on it.
     * @param pp
     * @return
     */
    template <class T2>
    HRESULT queryInterfaceTo(T2 **pp) const
    {
        if (pp)
        {
            if (m_p)
                return m_p->QueryInterface(COM_IIDOF(T2), (void **)pp);
            *pp = NULL;
            return S_OK;
        }
        return E_INVALIDARG;
    }

    /**
     * Equality test operator. By COM definition, two COM objects are considered
     * equal if their IUnknown interface pointers are equal.
     */
    template <class T2>
    bool operator==(T2 *p)
    {
        IUnknown *p1 = NULL;
        bool fNeedsRelease1 = false;
        if (m_p)
            fNeedsRelease1 = (SUCCEEDED(m_p->QueryInterface(COM_IIDOF(IUnknown), (void **)&p1)));

        IUnknown *p2 = NULL;
        bool fNeedsRelease2 = false;
        if (p)
            fNeedsRelease2 = (SUCCEEDED(p->QueryInterface(COM_IIDOF(IUnknown), (void **)&p2)));

        bool f = p1 == p2;
        if (fNeedsRelease1)
            p1->Release();
        if (fNeedsRelease2)
            p2->Release();
        return f;
    }

    /**
     *  Creates an in-process object of the given class ID and starts to
     *  manage a reference to the created object in case of success.
     */
    HRESULT createInprocObject(const CLSID &clsid)
    {
        HRESULT rc;
        T *obj = NULL;
#ifndef VBOX_WITH_XPCOM
        rc = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, COM_IIDOF(T),
                              (void **)&obj);
#else /* VBOX_WITH_XPCOM */
        using namespace com;
        rc = GlueCreateInstance(clsid, NS_GET_IID(T), (void **)&obj);
#endif /* VBOX_WITH_XPCOM */
        *this = obj;
        if (SUCCEEDED(rc))
            obj->Release();
        return rc;
    }

    /**
     *  Creates a local (out-of-process) object of the given class ID and starts
     *  to manage a reference to the created object in case of success.
     *
     *  Note: In XPCOM, the out-of-process functionality is currently emulated
     *  through in-process wrapper objects (that start a dedicated process and
     *  redirect all object requests to that process). For this reason, this
     *  method is fully equivalent to #createInprocObject() for now.
     */
    HRESULT createLocalObject(const CLSID &clsid)
    {
#ifndef VBOX_WITH_XPCOM
        HRESULT rc;
        T *obj = NULL;
        rc = CoCreateInstance(clsid, NULL, CLSCTX_LOCAL_SERVER, COM_IIDOF(T),
                              (void **)&obj);
        *this = obj;
        if (SUCCEEDED(rc))
            obj->Release();
        return rc;
#else /* VBOX_WITH_XPCOM */
        return createInprocObject(clsid);
#endif /* VBOX_WITH_XPCOM */
    }

#ifdef VBOX_WITH_XPCOM
    /**
     *  Creates an object of the given class ID on the specified server and
     *  starts to manage a reference to the created object in case of success.
     *
     *  @param serverName   Name of the server to create an object within.
     */
    HRESULT createObjectOnServer(const CLSID &clsid, const char *serverName)
    {
        T *obj = NULL;
        HRESULT rc = GlueCreateObjectOnServer(clsid, serverName, NS_GET_IID(T), (void **)&obj);
        *this = obj;
        if (SUCCEEDED(rc))
            obj->Release();
        return rc;
    }
#endif

protected:
    void copyFrom(T *p)
    {
        m_p = p;
        if (m_p)
            m_p->AddRef();
    }

    void cleanup()
    {
        if (m_p)
        {
            m_p->Release();
            m_p = NULL;
        }
    }

public:
    // Do NOT access this member unless you really know what you're doing!
    T *m_p;
};

/**
 * ComObjPtr is a more specialized variant of ComPtr designed to be used for implementation
 * objects. For example, use ComPtr<IMachine> for a client pointer that calls the interface
 * but ComObjPtr<Machine> for a pointer to an implementation object.
 *
 * The methods behave the same except that ComObjPtr has the additional createObject()
 * method which allows for instantiating a new implementation object.
 *
 * Note: To convert a ComObjPtr<InterfaceImpl> to a ComObj<IInterface> you have
 * to query the interface. See the following example code for the IProgress
 * interface:
 *
 *  @code
 *
 *  {
 *      ComObjPtr<Progress> pProgress;                       // create the server side object
 *      pProgress.createObject();                            // ...
 *      pProgress->init(...);                                // ...
 *      ComPtr<IProgress> pProgress2;                        // create an interface pointer
 *      pProgress.queryInterfaceTo(pProgress2.asOutParam()); // transfer the interface
 *  }
 *
 *  @endcode
 */
template <class T>
class ComObjPtr : public ComPtr<T>
{
public:

    ComObjPtr()
        : ComPtr<T>()
    {}

    ComObjPtr(const ComObjPtr &that)
        : ComPtr<T>(that)
    {}

    ComObjPtr(T *that_p)
        : ComPtr<T>(that_p)
    {}

    ComObjPtr& operator=(const ComObjPtr &that)
    {
        ComPtr<T>::operator=(that);
        return *this;
    }

    ComObjPtr& operator=(T *that_p)
    {
        ComPtr<T>::operator=(that_p);
        return *this;
    }

    /**
     *  Creates a new server-side object of the given component class and
     *  immediately starts to manage a pointer to the created object (the
     *  previous pointer, if any, is of course released when appropriate).
     *
     *  @note Win32: when VBOX_COM_OUTOFPROC_MODULE is defined, the created
     *  object doesn't increase the lock count of the server module, as it
     *  does otherwise.
     *
     *  @note In order to make it easier to use, this method does _not_ throw
     *        bad_alloc, but instead returns E_OUTOFMEMORY.
     */
    HRESULT createObject()
    {
        HRESULT hrc;
#ifndef VBOX_WITH_XPCOM
# ifdef VBOX_COM_OUTOFPROC_MODULE
        ATL::CComObjectNoLock<T> *obj = NULL;
        try
        {
            obj = new ATL::CComObjectNoLock<T>();
        }
        catch (std::bad_alloc &)
        {
            obj = NULL;
        }
        if (obj)
        {
            obj->InternalFinalConstructAddRef();
            try
            {
                hrc = obj->FinalConstruct();
            }
            catch (std::bad_alloc &)
            {
                hrc = E_OUTOFMEMORY;
            }
            obj->InternalFinalConstructRelease();
            if (FAILED(hrc))
            {
                delete obj;
                obj = NULL;
            }
        }
        else
            hrc = E_OUTOFMEMORY;
# else
        ATL::CComObject<T> *obj = NULL;
        hrc = ATL::CComObject<T>::CreateInstance(&obj);
# endif
#else /* VBOX_WITH_XPCOM */
        ATL::CComObject<T> *obj;
# ifndef RT_EXCEPTIONS_ENABLED
        obj = new ATL::CComObject<T>();
# else
        try
        {
            obj = new ATL::CComObject<T>();
        }
        catch (std::bad_alloc &)
        {
            obj = NULL;
        }
# endif
        if (obj)
        {
# ifndef RT_EXCEPTIONS_ENABLED
            hrc = obj->FinalConstruct();
# else
            try
            {
                hrc = obj->FinalConstruct();
            }
            catch (std::bad_alloc &)
            {
                hrc = E_OUTOFMEMORY;
            }
# endif
            if (FAILED(hrc))
            {
                delete obj;
                obj = NULL;
            }
        }
        else
            hrc = E_OUTOFMEMORY;
#endif /* VBOX_WITH_XPCOM */
        *this = obj;
        return hrc;
    }
};

/** @} */

#endif /* !VBOX_INCLUDED_com_ptr_h */

