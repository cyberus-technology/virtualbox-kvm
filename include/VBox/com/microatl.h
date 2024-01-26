/** @file
 * ATL lookalike, just the tiny subset we actually need.
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

#ifndef VBOX_INCLUDED_com_microatl_h
#define VBOX_INCLUDED_com_microatl_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/cdefs.h>   /* VBOX_STRICT */
#include <iprt/assert.h>
#include <iprt/critsect.h>
#include <iprt/errcore.h> /* RT_FAILURE() */

#include <iprt/win/windows.h>

#include <new>


namespace ATL
{

#define ATL_NO_VTABLE __declspec(novtable)

class CAtlModule;
__declspec(selectany) CAtlModule *_pAtlModule = NULL;

class CComModule;
__declspec(selectany) CComModule *_pModule = NULL;

typedef HRESULT (WINAPI FNCREATEINSTANCE)(void *pv, REFIID riid, void **ppv);
typedef FNCREATEINSTANCE *PFNCREATEINSTANCE;
typedef HRESULT (WINAPI FNINTERFACEMAPHELPER)(void *pv, REFIID riid, void **ppv, DWORD_PTR dw);
typedef FNINTERFACEMAPHELPER *PFNINTERFACEMAPHELPER;
typedef void (__stdcall FNATLTERMFUNC)(void *pv);
typedef FNATLTERMFUNC *PFNATLTERMFUNC;

struct _ATL_TERMFUNC_ELEM
{
    PFNATLTERMFUNC pfn;
    void *pv;
    _ATL_TERMFUNC_ELEM *pNext;
};

struct _ATL_INTMAP_ENTRY
{
    const IID *piid;    // interface ID
    DWORD_PTR dw;
    PFNINTERFACEMAPHELPER pFunc; // NULL: end of array, 1: offset based map entry, other: function pointer
};

#define COM_SIMPLEMAPENTRY ((ATL::PFNINTERFACEMAPHELPER)1)

#define DECLARE_CLASSFACTORY_EX(c) typedef ATL::CComCreator<ATL::CComObjectNoLock<c> > _ClassFactoryCreatorClass;
#define DECLARE_CLASSFACTORY() DECLARE_CLASSFACTORY_EX(ATL::CComClassFactory)
#define DECLARE_CLASSFACTORY_SINGLETON(o) DECLARE_CLASSFACTORY_EX(ATL::CComClassFactorySingleton<o>)
#define DECLARE_AGGREGATABLE(c) \
public: \
    typedef ATL::CComCreator2<ATL::CComCreator<ATL::CComObject<c> >, ATL::CComCreator<ATL::CComAggObject<c> > > _CreatorClass;
#define DECLARE_NOT_AGGREGATABLE(c) \
public: \
    typedef ATL::CComCreator2<ATL::CComCreator<ATL::CComObject<c> >, ATL::CComFailCreator<CLASS_E_NOAGGREGATION> > _CreatorClass;

#define DECLARE_PROTECT_FINAL_CONSTRUCT() \
    void InternalFinalConstructAddRef() \
    { \
        InternalAddRef(); \
    } \
    void InternalFinalConstructRelease() \
    { \
        InternalRelease(); \
    }

#define BEGIN_COM_MAP(c) \
public: \
    typedef c _ComClass; \
    HRESULT _InternalQueryInterface(REFIID iid, void **ppvObj) throw() \
    { \
        return InternalQueryInterface(this, _GetEntries(), iid, ppvObj); \
    } \
    const static ATL::_ATL_INTMAP_ENTRY *WINAPI _GetEntries() throw() \
    { \
        static const ATL::_ATL_INTMAP_ENTRY _aInterfaces[] = \
        {

#define COM_INTERFACE_ENTRY(c) \
    { &__uuidof(c), (DWORD_PTR)(static_cast<c *>((_ComClass *)8))-8, COM_SIMPLEMAPENTRY },

#define COM_INTERFACE_ENTRY2(c, c2) \
    { &__uuidof(c), (DWORD_PTR)(static_cast<c *>(static_cast<c2 *>((_ComClass *)8)))-8, COM_SIMPLEMAPENTRY },

#define COM_INTERFACE_ENTRY_AGGREGATE(iid, pUnk) \
    { &iid, (DWORD_PTR)RT_UOFFSETOF(_ComClass, pUnk), _Delegate},

#define END_COM_MAP() \
            { NULL, 0, NULL} \
        }; \
        return _aInterfaces; \
    } \
    virtual ULONG STDMETHODCALLTYPE AddRef(void) throw() = 0; \
    virtual ULONG STDMETHODCALLTYPE Release(void) throw() = 0; \
    STDMETHOD(QueryInterface)(REFIID, void **) throw() = 0;

struct _ATL_OBJMAP_ENTRY
{
    const CLSID *pclsid;
    PFNCREATEINSTANCE pfnGetClassObject;
    PFNCREATEINSTANCE pfnCreateInstance;
    IUnknown *pCF;
    DWORD dwRegister;
};

#define BEGIN_OBJECT_MAP(o) static ATL::_ATL_OBJMAP_ENTRY o[] = {
#define END_OBJECT_MAP()   {NULL, NULL, NULL, NULL, 0}};
#define OBJECT_ENTRY(clsid, c) {&clsid, c::_ClassFactoryCreatorClass::CreateInstance, c::_CreatorClass::CreateInstance, NULL, 0 },


class CComCriticalSection
{
public:
    CComCriticalSection() throw()
    {
        memset(&m_CritSect, 0, sizeof(m_CritSect));
    }
    ~CComCriticalSection()
    {
    }
    HRESULT Lock() throw()
    {
        RTCritSectEnter(&m_CritSect);
        return S_OK;
    }
    HRESULT Unlock() throw()
    {
        RTCritSectLeave(&m_CritSect);
        return S_OK;
    }
    HRESULT Init() throw()
    {
        HRESULT hrc = S_OK;
        if (RT_FAILURE(RTCritSectInit(&m_CritSect)))
            hrc = E_FAIL;
        return hrc;
    }

    HRESULT Term() throw()
    {
        RTCritSectDelete(&m_CritSect);
        return S_OK;
    }

    RTCRITSECT m_CritSect;
};

template<class TLock>
class CComCritSectLockManual
{
public:
    CComCritSectLockManual(CComCriticalSection &cs)
        : m_cs(cs)
        , m_fLocked(false)
    {
    }

    ~CComCritSectLockManual() throw()
    {
        if (m_fLocked)
            Unlock();
    }

    HRESULT Lock()
    {
        Assert(!m_fLocked);
        HRESULT hrc = m_cs.Lock();
        if (FAILED(hrc))
            return hrc;
        m_fLocked = true;
        return S_OK;
    }

    void Unlock() throw()
    {
        Assert(m_fLocked);
        m_cs.Unlock();
        m_fLocked = false;
    }


private:
    TLock &m_cs;
    bool m_fLocked;

    CComCritSectLockManual(const CComCritSectLockManual&) throw(); // Do not call.
    CComCritSectLockManual &operator=(const CComCritSectLockManual &) throw(); // Do not call.
};


#ifdef RT_EXCEPTIONS_ENABLED
/** This is called CComCritSecLock in real ATL... */
template<class TLock>
class CComCritSectLock : public CComCritSectLockManual<TLock>
{
public:
    CComCritSectLock(CComCriticalSection &cs, bool fInitialLock = true)
        : CComCritSectLockManual(cs)
    {
        if (fInitialLock)
        {
            HRESULT hrc = Lock();
            if (FAILED(hrc))
                throw hrc;
        }
    }

private:
    CComCritSectLock(const CComCritSectLock&) throw(); // Do not call.
    CComCritSectLock &operator=(const CComCritSectLock &) throw(); // Do not call.
};
#endif

class CComFakeCriticalSection
{
public:
    HRESULT Lock() throw()
    {
        return S_OK;
    }
    HRESULT Unlock() throw()
    {
        return S_OK;
    }
    HRESULT Init() throw()
    {
        return S_OK;
    }
    HRESULT Term() throw()
    {
        return S_OK;
    }
};

class CComAutoCriticalSection : public CComCriticalSection
{
public:
    CComAutoCriticalSection()
    {
        HRESULT hrc = CComCriticalSection::Init();
        if (FAILED(hrc))
            throw hrc;
    }
    ~CComAutoCriticalSection() throw()
    {
        CComCriticalSection::Term();
    }
private :
    HRESULT Init() throw(); // Do not call.
    HRESULT Term() throw(); // Do not call.
};

class CComAutoDeleteCriticalSection : public CComCriticalSection
{
public:
    CComAutoDeleteCriticalSection(): m_fInit(false)
    {
    }

    ~CComAutoDeleteCriticalSection() throw()
    {
        if (!m_fInit)
            return;
        m_fInit = false;
        CComCriticalSection::Term();
    }

    HRESULT Init() throw()
    {
        Assert(!m_fInit);
        HRESULT hrc = CComCriticalSection::Init();
        if (SUCCEEDED(hrc))
            m_fInit = true;
        return hrc;
    }

    HRESULT Lock()
    {
        Assert(m_fInit);
        return CComCriticalSection::Lock();
    }

    HRESULT Unlock()
    {
        Assert(m_fInit);
        return CComCriticalSection::Unlock();
    }

private:
    HRESULT Term() throw();
    bool m_fInit;
};


class CComMultiThreadModelNoCS
{
public:
    static ULONG WINAPI Increment(LONG *pL) throw()
    {
        return InterlockedIncrement(pL);
    }
    static ULONG WINAPI Decrement(LONG *pL) throw()
    {
        return InterlockedDecrement(pL);
    }
    typedef CComFakeCriticalSection AutoCriticalSection;
    typedef CComFakeCriticalSection AutoDeleteCriticalSection;
    typedef CComMultiThreadModelNoCS ThreadModelNoCS;
};

class CComMultiThreadModel
{
public:
    static ULONG WINAPI Increment(LONG *pL) throw()
    {
        return InterlockedIncrement(pL);
    }
    static ULONG WINAPI Decrement(LONG *pL) throw()
    {
        return InterlockedDecrement(pL);
    }
    typedef CComAutoCriticalSection AutoCriticalSection;
    typedef CComAutoDeleteCriticalSection AutoDeleteCriticalSection;
    typedef CComMultiThreadModelNoCS ThreadModelNoCS;
};

class ATL_NO_VTABLE CAtlModule
{
public:
    static GUID m_LibID;
    CComCriticalSection m_csStaticDataInitAndTypeInfo;

    CAtlModule() throw()
    {
        // One instance only per linking namespace!
        AssertMsg(!_pAtlModule, ("CAtlModule: trying to create more than one instance per linking namespace\n"));

        fInit = false;

        m_cLock = 0;
        m_pTermFuncs = NULL;
        _pAtlModule = this;

        if (FAILED(m_csStaticDataInitAndTypeInfo.Init()))
        {
            AssertMsgFailed(("CAtlModule: failed to init critsect\n"));
            return;
        }
        fInit = true;
    }

    void Term() throw()
    {
        if (!fInit)
            return;

        // Call all term functions.
        if (m_pTermFuncs)
        {
            _ATL_TERMFUNC_ELEM *p = m_pTermFuncs;
            _ATL_TERMFUNC_ELEM *pNext;
            while (p)
            {
                p->pfn(p->pv);
                pNext = p->pNext;
                delete p;
                p = pNext;
            }
            m_pTermFuncs = NULL;
        }
        m_csStaticDataInitAndTypeInfo.Term();
        fInit = false;
    }

    virtual ~CAtlModule() throw()
    {
        Term();
    }

    virtual LONG Lock() throw()
    {
        return CComMultiThreadModel::Increment(&m_cLock);
    }

    virtual LONG Unlock() throw()
    {
        return CComMultiThreadModel::Decrement(&m_cLock);
    }

    virtual LONG GetLockCount() throw()
    {
        return m_cLock;
    }

    HRESULT AddTermFunc(PFNATLTERMFUNC pfn, void *pv)
    {
        _ATL_TERMFUNC_ELEM *pNew = new(std::nothrow) _ATL_TERMFUNC_ELEM;
        if (!pNew)
            return E_OUTOFMEMORY;
        pNew->pfn = pfn;
        pNew->pv = pv;
        CComCritSectLockManual<CComCriticalSection> lock(m_csStaticDataInitAndTypeInfo);
        HRESULT hrc = lock.Lock();
        if (SUCCEEDED(hrc))
        {
            pNew->pNext = m_pTermFuncs;
            m_pTermFuncs = pNew;
        }
        else
        {
            delete pNew;
            AssertMsgFailed(("CComModule::AddTermFunc: failed to lock critsect\n"));
        }
        return hrc;
    }

protected:
    bool fInit;
    LONG m_cLock;
    _ATL_TERMFUNC_ELEM *m_pTermFuncs;
};

__declspec(selectany) GUID CAtlModule::m_LibID = {0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0} };

struct _ATL_COM_MODULE
{
    HINSTANCE m_hInstTypeLib;
    CComCriticalSection m_csObjMap;
};

#ifndef _delayimp_h
extern "C" IMAGE_DOS_HEADER __ImageBase;
#endif

class CAtlComModule : public _ATL_COM_MODULE
{
public:
    static bool m_fInitFailed;
    CAtlComModule() throw()
    {
        m_hInstTypeLib = reinterpret_cast<HINSTANCE>(&__ImageBase);

        if (FAILED(m_csObjMap.Init()))
        {
            AssertMsgFailed(("CAtlComModule: critsect init failed\n"));
            m_fInitFailed = true;
            return;
        }
    }

    ~CAtlComModule()
    {
        Term();
    }

    void Term()
    {
        m_csObjMap.Term();
    }
};

__declspec(selectany) bool CAtlComModule::m_fInitFailed = false;
__declspec(selectany) CAtlComModule _AtlComModule;

template <class T> class ATL_NO_VTABLE CAtlModuleT : public CAtlModule
{
public:
    CAtlModuleT() throw()
    {
        T::InitLibId();
    }

    static void InitLibId() throw()
    {
    }
};

/**
 *
 * This class not _not_ be statically instantiated as a global variable!  It may
 * use VBoxRT before it's initialized otherwise, messing up logging and whatnot.
 *
 * When possible create the instance inside the TrustedMain() or main() as a
 * stack variable.  In DLLs use 'new' to instantiate it in the DllMain function.
 */
class CComModule : public CAtlModuleT<CComModule>
{
public:
    CComModule()
    {
        // One instance only per linking namespace!
        AssertMsg(!_pModule, ("CComModule: trying to create more than one instance per linking namespace\n"));
        _pModule = this;
        m_pObjMap = NULL;
    }

    ~CComModule()
    {
    }

    _ATL_OBJMAP_ENTRY *m_pObjMap;
    HRESULT Init(_ATL_OBJMAP_ENTRY *p, HINSTANCE h, const GUID *pLibID = NULL) throw()
    {
        RT_NOREF1(h);

        if (pLibID)
            m_LibID = *pLibID;

        // Go over the object map to do some sanity checking, making things
        // crash early if something is seriously busted.
        _ATL_OBJMAP_ENTRY *pEntry;
        if (p != (_ATL_OBJMAP_ENTRY *)-1)
        {
            m_pObjMap = p;
            if (m_pObjMap)
            {
                pEntry = m_pObjMap;
                while (pEntry->pclsid)
                    pEntry++;
            }
        }
        return S_OK;
    }

    void Term() throw()
    {
        _ATL_OBJMAP_ENTRY *pEntry;
        if (m_pObjMap)
        {
            pEntry = m_pObjMap;
            while (pEntry->pclsid)
            {
                if (pEntry->pCF)
                    pEntry->pCF->Release();
                pEntry->pCF = NULL;
                pEntry++;
            }
        }

        CAtlModuleT<CComModule>::Term();
    }

    HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, void **ppv) throw()
    {
        *ppv = NULL;
        HRESULT hrc = S_OK;

        if (m_pObjMap)
        {
            const _ATL_OBJMAP_ENTRY *pEntry = m_pObjMap;

            while (pEntry->pclsid)
            {
                if (pEntry->pfnGetClassObject && rclsid == *pEntry->pclsid)
                {
                    if (!pEntry->pCF)
                    {
                        CComCritSectLockManual<CComCriticalSection> lock(_AtlComModule.m_csObjMap);
                        hrc = lock.Lock();
                        if (FAILED(hrc))
                        {
                            AssertMsgFailed(("CComModule::GetClassObject: failed to lock critsect\n"));
                            break;
                        }

                        if (!pEntry->pCF)
                        {
                            hrc = pEntry->pfnGetClassObject(pEntry->pfnCreateInstance, __uuidof(IUnknown), (void **)&pEntry->pCF);
                        }
                    }

                    if (pEntry->pCF)
                    {
                        hrc = pEntry->pCF->QueryInterface(riid, ppv);
                    }
                    break;
                }
                pEntry++;
            }
        }

        return hrc;
    }

    // For EXE only: register all class factories with COM.
    HRESULT RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags) throw()
    {
        HRESULT hrc = S_OK;
        _ATL_OBJMAP_ENTRY *pEntry;
        if (m_pObjMap)
        {
            pEntry = m_pObjMap;
            while (pEntry->pclsid && SUCCEEDED(hrc))
            {
                if (pEntry->pfnGetClassObject)
                {
                    IUnknown *p;
                    hrc = pEntry->pfnGetClassObject(pEntry->pfnCreateInstance, __uuidof(IUnknown), (void **)&p);
                    if (SUCCEEDED(hrc))
                        hrc = CoRegisterClassObject(*(pEntry->pclsid), p, dwClsContext, dwFlags, &pEntry->dwRegister);
                    if (p)
                        p->Release();
                }
                pEntry++;
            }
        }
        return hrc;
    }
    // For EXE only: revoke all class factories with COM.
    HRESULT RevokeClassObjects() throw()
    {
        HRESULT hrc = S_OK;
        _ATL_OBJMAP_ENTRY *pEntry;
        if (m_pObjMap != NULL)
        {
            pEntry = m_pObjMap;
            while (pEntry->pclsid && SUCCEEDED(hrc))
            {
                if (pEntry->dwRegister)
                    hrc = CoRevokeClassObject(pEntry->dwRegister);
                pEntry++;
            }
        }
        return hrc;
    }
};


template <class T> class CComCreator
{
public:
    static HRESULT WINAPI CreateInstance(void *pv, REFIID riid, void **ppv)
    {
        AssertReturn(ppv, E_POINTER);
        *ppv = NULL;
        HRESULT hrc = E_OUTOFMEMORY;
        T *p = new(std::nothrow) T(pv);
        if (p)
        {
            p->SetVoid(pv);
            p->InternalFinalConstructAddRef();
            hrc = p->_AtlInitialConstruct();
            if (SUCCEEDED(hrc))
                hrc = p->FinalConstruct();
            p->InternalFinalConstructRelease();
            if (SUCCEEDED(hrc))
                hrc = p->QueryInterface(riid, ppv);
            if (FAILED(hrc))
                delete p;
        }
        return hrc;
    }
};

template <HRESULT hrc> class CComFailCreator
{
public:
    static HRESULT WINAPI CreateInstance(void *, REFIID, void **ppv)
    {
        AssertReturn(ppv, E_POINTER);
        *ppv = NULL;

        return hrc;
    }
};

template <class T1, class T2> class CComCreator2
{
public:
    static HRESULT WINAPI CreateInstance(void *pv, REFIID riid, void **ppv)
    {
        AssertReturn(ppv, E_POINTER);

        return !pv ? T1::CreateInstance(NULL, riid, ppv) : T2::CreateInstance(pv, riid, ppv);
    }
};

template <class Base> class CComObjectCached : public Base
{
public:
    CComObjectCached(void * = NULL)
    {
    }
    virtual ~CComObjectCached()
    {
        // Catch refcount screwups by setting refcount to -(LONG_MAX/2).
        m_iRef = -(LONG_MAX/2);
        FinalRelease();
    }
    STDMETHOD_(ULONG, AddRef)() throw()
    {
        // If you get errors about undefined InternalAddRef then Base does not
        // derive from CComObjectRootEx.
        ULONG l = InternalAddRef();
        if (l == 2)
        {
            AssertMsg(_pAtlModule, ("ATL: referring to ATL module without having one declared in this linking namespace\n"));
            _pAtlModule->Lock();
        }
        return l;
    }
    STDMETHOD_(ULONG, Release)() throw()
    {
        // If you get errors about undefined InternalRelease then Base does not
        // derive from CComObjectRootEx.
        ULONG l = InternalRelease();
        if (l == 0)
            delete this;
        else if (l == 1)
        {
            AssertMsg(_pAtlModule, ("ATL: referring to ATL module without having one declared in this linking namespace\n"));
            _pAtlModule->Unlock();
        }
        return l;
    }
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj) throw()
    {
        // If you get errors about undefined _InternalQueryInterface then
        // double check BEGIN_COM_MAP in the class definition.
        return _InternalQueryInterface(iid, ppvObj);
    }
    static HRESULT WINAPI CreateInstance(CComObjectCached<Base> **pp) throw()
    {
        AssertReturn(pp, E_POINTER);
        *pp = NULL;

        HRESULT hrc = E_OUTOFMEMORY;
        CComObjectCached<Base> *p = new(std::nothrow) CComObjectCached<Base>();
        if (p)
        {
            p->SetVoid(NULL);
            p->InternalFinalConstructAddRef();
            hrc = p->_AtlInitialConstruct();
            if (SUCCEEDED(hrc))
                hrc = p->FinalConstruct();
            p->InternalFinalConstructRelease();
            if (FAILED(hrc))
                delete p;
            else
                *pp = p;
        }
        return hrc;
    }
};

template <class Base> class CComObjectNoLock : public Base
{
public:
    CComObjectNoLock(void * = NULL)
    {
    }
    virtual ~CComObjectNoLock()
    {
        // Catch refcount screwups by setting refcount to -(LONG_MAX/2).
        m_iRef = -(LONG_MAX/2);
        FinalRelease();
    }
    STDMETHOD_(ULONG, AddRef)() throw()
    {
        // If you get errors about undefined InternalAddRef then Base does not
        // derive from CComObjectRootEx.
        return InternalAddRef();
    }
    STDMETHOD_(ULONG, Release)() throw()
    {
        // If you get errors about undefined InternalRelease then Base does not
        // derive from CComObjectRootEx.
        ULONG l = InternalRelease();
        if (l == 0)
            delete this;
        return l;
    }
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj) throw()
    {
        // If you get errors about undefined _InternalQueryInterface then
        // double check BEGIN_COM_MAP in the class definition.
        return _InternalQueryInterface(iid, ppvObj);
    }
};

class CComTypeInfoHolder
{
    /** @todo implement type info caching, making stuff more efficient - would we benefit? */
public:
    const GUID *m_pGUID;
    const GUID *m_pLibID;
    WORD m_iMajor;
    WORD m_iMinor;
    ITypeInfo *m_pTInfo;

    HRESULT GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
    {
        if (iTInfo != 0)
            return DISP_E_BADINDEX;
        return GetTI(lcid, ppTInfo);
    }
    HRESULT GetIDsOfNames(REFIID riid, LPOLESTR *pwszNames, UINT cNames, LCID lcid, DISPID *pDispID)
    {
        RT_NOREF1(riid); /* should be IID_NULL */
        HRESULT hrc = FetchTI(lcid);
        if (m_pTInfo)
            hrc = m_pTInfo->GetIDsOfNames(pwszNames, cNames, pDispID);
        return hrc;
    }
    HRESULT Invoke(IDispatch *p, DISPID DispID, REFIID riid, LCID lcid, WORD iFlags, DISPPARAMS *pDispParams,
                   VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
    {
        RT_NOREF1(riid); /* should be IID_NULL */
        HRESULT hrc = FetchTI(lcid);
        if (m_pTInfo)
            hrc = m_pTInfo->Invoke(p, DispID, iFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
        return hrc;
    }
private:
    static void __stdcall Cleanup(void *pv)
    {
        AssertReturnVoid(pv);
        CComTypeInfoHolder *p = (CComTypeInfoHolder *)pv;
        if (p->m_pTInfo != NULL)
            p->m_pTInfo->Release();
        p->m_pTInfo = NULL;
    }

    HRESULT GetTI(LCID lcid)
    {
        AssertMsg(_pAtlModule, ("ATL: referring to ATL module without having one declared in this linking namespace\n"));
        Assert(m_pLibID && m_pGUID);
        if (m_pTInfo)
            return S_OK;

        CComCritSectLockManual<CComCriticalSection> lock(_pAtlModule->m_csStaticDataInitAndTypeInfo);
        HRESULT hrc = lock.Lock();
        if (SUCCEEDED(hrc))
        {
            ITypeLib *pTypeLib = NULL;
            Assert(*m_pLibID != GUID_NULL);
            hrc = LoadRegTypeLib(*m_pLibID, m_iMajor, m_iMinor, lcid, &pTypeLib);
            if (SUCCEEDED(hrc))
            {
                ITypeInfo *pTypeInfo;
                hrc = pTypeLib->GetTypeInfoOfGuid(*m_pGUID, &pTypeInfo);
                if (SUCCEEDED(hrc))
                {
                    ITypeInfo2 *pTypeInfo2;
                    if (SUCCEEDED(pTypeInfo->QueryInterface(__uuidof(ITypeInfo2), (void **)&pTypeInfo2)))
                    {
                        pTypeInfo->Release();
                        pTypeInfo = pTypeInfo2;
                    }
                    m_pTInfo = pTypeInfo;
                    _pAtlModule->AddTermFunc(Cleanup, (void *)this);
                }
                pTypeLib->Release();
            }
        }
        return hrc;
    }
    HRESULT GetTI(LCID lcid, ITypeInfo **ppTInfo)
    {
        AssertReturn(ppTInfo, E_POINTER);
        HRESULT hrc = S_OK;
        if (!m_pTInfo)
            hrc = GetTI(lcid);
        if (m_pTInfo)
        {
            m_pTInfo->AddRef();
            hrc = S_OK;
        }
        *ppTInfo = m_pTInfo;
        return hrc;
    }
    HRESULT FetchTI(LCID lcid)
    {
        if (!m_pTInfo)
            return GetTI(lcid);
        return S_OK;
    }
};

template <class ThreadModel> class CComObjectRootEx
{
public:
    typedef ThreadModel _ThreadModel;
    CComObjectRootEx()
    {
        m_iRef = 0L;
    }
    ~CComObjectRootEx()
    {
    }
    ULONG InternalAddRef()
    {
        Assert(m_iRef != -1L);
        return ThreadModel::Increment(&m_iRef);
    }
    ULONG InternalRelease()
    {
#ifdef VBOX_STRICT
        LONG c = ThreadModel::Decrement(&m_iRef);
        AssertMsg(c >= -(LONG_MAX / 2), /* See  ~CComObjectNoLock, ~CComObject & ~CComAggObject. */
                  ("Release called on object which has been already destroyed!\n"));
        return c;
#else
        return ThreadModel::Decrement(&m_iRef);
#endif
    }
    ULONG OuterAddRef()
    {
        return m_pOuterUnknown->AddRef();
    }
    ULONG OuterRelease()
    {
        return m_pOuterUnknown->Release();
    }
    HRESULT OuterQueryInterface(REFIID iid, void **ppvObject)
    {
        return m_pOuterUnknown->QueryInterface(iid, ppvObject);
    }
    HRESULT _AtlInitialConstruct()
    {
        return m_CritSect.Init();
    }
    void Lock()
    {
        m_CritSect.Lock();
    }
    void Unlock()
    {
        m_CritSect.Unlock();
    }
    void SetVoid(void *)
    {
    }
    void InternalFinalConstructAddRef()
    {
    }
    void InternalFinalConstructRelease()
    {
        Assert(m_iRef == 0);
    }
    HRESULT FinalConstruct()
    {
        return S_OK;
    }
    void FinalRelease()
    {
    }
    static HRESULT WINAPI InternalQueryInterface(void *pThis, const _ATL_INTMAP_ENTRY *pEntries, REFIID iid, void **ppvObj)
    {
        AssertReturn(pThis, E_INVALIDARG);
        AssertReturn(pEntries, E_INVALIDARG);
        AssertReturn(ppvObj, E_POINTER);
        *ppvObj = NULL;
        if (iid == IID_IUnknown)
        {
            // For IUnknown use first interface, must be simple map entry.
            Assert(pEntries->pFunc == COM_SIMPLEMAPENTRY);
            IUnknown *pObj = (IUnknown *)((INT_PTR)pThis + pEntries->dw);
            pObj->AddRef();
            *ppvObj = pObj;
            return S_OK;
        }
        while (pEntries->pFunc)
        {
            if (iid == *pEntries->piid)
            {
                if (pEntries->pFunc == COM_SIMPLEMAPENTRY)
                {
                    IUnknown *pObj = (IUnknown *)((INT_PTR)pThis + pEntries->dw);
                    pObj->AddRef();
                    *ppvObj = pObj;
                    return S_OK;
                }
                else
                    return pEntries->pFunc(pThis, iid, ppvObj, pEntries->dw);
            }
            pEntries++;
        }
        return E_NOINTERFACE;
    }
    static HRESULT WINAPI _Delegate(void *pThis, REFIID iid, void **ppvObj, DWORD_PTR dw)
    {
        AssertPtrReturn(pThis, E_NOINTERFACE);
        IUnknown *pObj = *(IUnknown **)((DWORD_PTR)pThis + dw);
        // If this assertion fails then the object has a delegation with a NULL
        // object pointer, which is highly unusual often means that the pointer
        // was not set up correctly. Check the COM interface map of the class
        // for bugs with initializing.
        AssertPtrReturn(pObj, E_NOINTERFACE);
        return pObj->QueryInterface(iid, ppvObj);
    }

    union
    {
        LONG m_iRef;
        IUnknown *m_pOuterUnknown;
    };
private:
    typename ThreadModel::AutoDeleteCriticalSection m_CritSect;
};

template <class Base> class CComObject : public Base
{
public:
    CComObject(void * = NULL) throw()
    {
        AssertMsg(_pAtlModule, ("ATL: referring to ATL module without having one declared in this linking namespace\n"));
        _pAtlModule->Lock();
    }
    virtual ~CComObject() throw()
    {
        AssertMsg(_pAtlModule, ("ATL: referring to ATL module without having one declared in this linking namespace\n"));
        // Catch refcount screwups by setting refcount to -(LONG_MAX/2).
        m_iRef = -(LONG_MAX/2);
        FinalRelease();
        _pAtlModule->Unlock();
    }
    STDMETHOD_(ULONG, AddRef)()
    {
        // If you get errors about undefined InternalAddRef then Base does not
        // derive from CComObjectRootEx.
        return InternalAddRef();
    }
    STDMETHOD_(ULONG, Release)()
    {
        // If you get errors about undefined InternalRelease then Base does not
        // derive from CComObjectRootEx.
        ULONG l = InternalRelease();
        if (l == 0)
            delete this;
        return l;
    }
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj) throw()
    {
        // If you get errors about undefined _InternalQueryInterface then
        // double check BEGIN_COM_MAP in the class definition.
        return _InternalQueryInterface(iid, ppvObj);
    }

    static HRESULT WINAPI CreateInstance(CComObject<Base> **pp) throw()
    {
        AssertReturn(pp, E_POINTER);
        *pp = NULL;

        HRESULT hrc = E_OUTOFMEMORY;
        CComObject<Base> *p = NULL;
        try
        {
            p = new CComObject<Base>();
        }
        catch (std::bad_alloc &)
        {
            p = NULL;
        }
        if (p)
        {
            p->InternalFinalConstructAddRef();
            try
            {
                hrc = p->_AtlInitialConstruct();
                if (SUCCEEDED(hrc))
                    hrc = p->FinalConstruct();
            }
            catch (std::bad_alloc &)
            {
                hrc = E_OUTOFMEMORY;
            }
            p->InternalFinalConstructRelease();
            if (FAILED(hrc))
            {
                delete p;
                p = NULL;
            }
        }
        *pp = p;
        return hrc;
    }
};

template <class T, const IID *piid, const GUID *pLibID, WORD iMajor = 1, WORD iMinor = 0> class ATL_NO_VTABLE IDispatchImpl : public T
{
public:
    // IDispatch
    STDMETHOD(GetTypeInfoCount)(UINT *pcTInfo)
    {
        if (!pcTInfo)
            return E_POINTER;
        *pcTInfo = 1;
        return S_OK;
    }
    STDMETHOD(GetTypeInfo)(UINT cTInfo, LCID lcid, ITypeInfo **ppTInfo)
    {
        return tih.GetTypeInfo(cTInfo, lcid, ppTInfo);
    }
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR *pwszNames, UINT cNames, LCID lcid, DISPID *pDispID)
    {
        return tih.GetIDsOfNames(riid, pwszNames, cNames, lcid, pDispID);
    }
    STDMETHOD(Invoke)(DISPID DispID, REFIID riid, LCID lcid, WORD iFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
    {
        return tih.Invoke((IDispatch *)this, DispID, riid, lcid, iFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    }
protected:
    static CComTypeInfoHolder tih;
    static HRESULT GetTI(LCID lcid, ITypeInfo **ppTInfo)
    {
        return tih.GetTI(lcid, ppTInfo);
    }
};

template <class T, const IID *piid, const GUID *pLibID, WORD iMajor, WORD iMinor> CComTypeInfoHolder IDispatchImpl<T, piid, pLibID, iMajor, iMinor>::tih = { piid, pLibID, iMajor, iMinor, NULL };


template <class Base> class CComContainedObject : public Base
{
public:
    CComContainedObject(void *pv)
    {
        m_pOuterUnknown = (IUnknown *)pv;
    }

    STDMETHOD_(ULONG, AddRef)() throw()
    {
        return OuterAddRef();
    }
    STDMETHOD_(ULONG, Release)() throw()
    {
        return OuterRelease();
    }
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj) throw()
    {
        return OuterQueryInterface(iid, ppvObj);
    }
};

template <class Aggregated> class CComAggObject :
    public IUnknown,
    public CComObjectRootEx<typename Aggregated::_ThreadModel::ThreadModelNoCS>
{
public:
    CComAggObject(void *pv) :
        m_Aggregated(pv)
    {
        AssertMsg(_pAtlModule, ("ATL: referring to ATL module without having one declared in this linking namespace\n"));
        _pAtlModule->Lock();
    }
    virtual ~CComAggObject()
    {
        AssertMsg(_pAtlModule, ("ATL: referring to ATL module without having one declared in this linking namespace\n"));
        // Catch refcount screwups by setting refcount to -(LONG_MAX/2).
        m_iRef = -(LONG_MAX/2);
        FinalRelease();
        _pAtlModule->Unlock();
    }
    HRESULT _AtlInitialConstruct()
    {
        HRESULT hrc = m_Aggregated._AtlInitialConstruct();
        if (SUCCEEDED(hrc))
        {
            hrc = CComObjectRootEx<typename Aggregated::_ThreadModel::ThreadModelNoCS>::_AtlInitialConstruct();
        }
        return hrc;
    }
    HRESULT FinalConstruct()
    {
        CComObjectRootEx<Aggregated::_ThreadModel::ThreadModelNoCS>::FinalConstruct();
        return m_Aggregated.FinalConstruct();
    }
    void FinalRelease()
    {
        CComObjectRootEx<Aggregated::_ThreadModel::ThreadModelNoCS>::FinalRelease();
        m_Aggregated.FinalRelease();
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        return InternalAddRef();
    }
    STDMETHOD_(ULONG, Release)()
    {
        ULONG l = InternalRelease();
        if (l == 0)
            delete this;
        return l;
    }
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj)
    {
        AssertReturn(ppvObj, E_POINTER);
        *ppvObj = NULL;

        HRESULT hrc = S_OK;
        if (iid == __uuidof(IUnknown))
        {
            *ppvObj = (void *)(IUnknown *)this;
            AddRef();
        }
        else
            hrc = m_Aggregated._InternalQueryInterface(iid, ppvObj);
        return hrc;
    }
    static HRESULT WINAPI CreateInstance(LPUNKNOWN pUnkOuter, CComAggObject<Aggregated> **pp)
    {
        AssertReturn(pp, E_POINTER);
        *pp = NULL;

        HRESULT hrc = E_OUTOFMEMORY;
        CComAggObject<Aggregated> *p = new(std::nothrow) CComAggObject<Aggregated>(pUnkOuter);
        if (p)
        {
            p->SetVoid(NULL);
            p->InternalFinalConstructAddRef();
            hrc = p->_AtlInitialConstruct();
            if (SUCCEEDED(hrc))
                hrc = p->FinalConstruct();
            p->InternalFinalConstructRelease();
            if (FAILED(hrc))
                delete p;
            else
                *pp = p;
        }
        return hrc;
    }

    CComContainedObject<Aggregated> m_Aggregated;
};

class CComClassFactory:
    public IClassFactory,
    public CComObjectRootEx<CComMultiThreadModel>
{
public:
    BEGIN_COM_MAP(CComClassFactory)
        COM_INTERFACE_ENTRY(IClassFactory)
    END_COM_MAP()

    virtual ~CComClassFactory()
    {
    }

    // IClassFactory
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void **ppvObj)
    {
        Assert(m_pfnCreateInstance);
        HRESULT hrc = E_POINTER;
        if (ppvObj)
        {
            *ppvObj = NULL;
            if (pUnkOuter && riid != __uuidof(IUnknown))
            {
                AssertMsgFailed(("CComClassFactory: cannot create an aggregated object other than IUnknown\n"));
                hrc = CLASS_E_NOAGGREGATION;
            }
            else
                hrc = m_pfnCreateInstance(pUnkOuter, riid, ppvObj);
        }
        return hrc;
    }

    STDMETHOD(LockServer)(BOOL fLock)
    {
        AssertMsg(_pAtlModule, ("ATL: referring to ATL module without having one declared in this linking namespace\n"));
        if (fLock)
            _pAtlModule->Lock();
        else
            _pAtlModule->Unlock();
        return S_OK;
    }

    // Set creator for use by the factory.
    void SetVoid(void *pv)
    {
        m_pfnCreateInstance = (PFNCREATEINSTANCE)pv;
    }

    PFNCREATEINSTANCE m_pfnCreateInstance;
};

template <class T> class CComClassFactorySingleton : public CComClassFactory
{
public:
    CComClassFactorySingleton() :
        m_hrc(S_OK),
        m_pObj(NULL)
    {
    }
    virtual ~CComClassFactorySingleton()
    {
        if (m_pObj)
            m_pObj->Release();
    }
    // IClassFactory
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void **pvObj)
    {
        HRESULT hrc = E_POINTER;
        if (ppvObj)
        {
            *ppvObj = NULL;
            // Singleton factories do not support aggregation.
            AssertReturn(!pUnkOuter, CLASS_E_NOAGGREGATION);

            // Test if singleton is already created. Do it outside the lock,
            // relying on atomic checks. Remember the inherent race!
            if (SUCCEEDED(m_hrc) && !m_pObj)
            {
                Lock();
                // Make sure that the module is in use, otherwise the
                // module can terminate while we're creating a new
                // instance, which leads to strange errors.
                LockServer(true);
                __try
                {
                    // Repeat above test to avoid races when multiple threads
                    // want to create a singleton simultaneously.
                    if (SUCCEEDED(m_hrc) && !m_pObj)
                    {
                        CComObjectCached<T> *p;
                        m_hrc = CComObjectCached<T>::CreateInstance(&p);
                        if (SUCCEEDED(m_hrc))
                        {
                            m_hrc = p->QueryInterface(IID_IUnknown, (void **)&m_pObj);
                            if (FAILED(m_hrc))
                            {
                                delete p;
                            }
                        }
                    }
                }
                __finally
                {
                    Unlock();
                    LockServer(false);
                }
            }
            if (SUCCEEDED(m_hrc))
            {
                hrc = m_pObj->QueryInterface(riid, ppvObj);
            }
            else
            {
                hrc = m_hrc;
            }
        }
        return hrc;
    }
    HRESULT m_hrc;
    IUnknown *m_pObj;
};


template <class T, const CLSID *pClsID = &CLSID_NULL> class CComCoClass
{
public:
    DECLARE_CLASSFACTORY()
    DECLARE_AGGREGATABLE(T)
    static const CLSID& WINAPI GetObjectCLSID()
    {
        return *pClsID;
    }
    template <class Q>
    static HRESULT CreateInstance(Q **pp)
    {
        return T::_CreatorClass::CreateInstance(NULL, __uuidof(Q), (void **)pp);
    }
};

} /* namespace ATL */

#endif /* !VBOX_INCLUDED_com_microatl_h */

