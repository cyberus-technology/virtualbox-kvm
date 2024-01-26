/** @file
 * MS COM / XPCOM Abstraction Layer - Common Definitions.
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

#ifndef VBOX_INCLUDED_com_defs_h
#define VBOX_INCLUDED_com_defs_h
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

#if defined (RT_OS_OS2)

# if defined(RT_MAX) && RT_MAX != 22
#  undef RT_MAX
#  define REDEFINE_RT_MAX
# endif
# undef RT_MAX

/* Make sure OS/2 Toolkit headers are pulled in to have BOOL/ULONG/etc. typedefs
 * already defined in order to be able to redefine them using #define. */
# define INCL_BASE
# define INCL_PM
# include <os2.h>

/* OS/2 Toolkit defines TRUE and FALSE */
# undef FALSE
# undef TRUE

/* */
# undef RT_MAX
# ifdef REDEFINE_RT_MAX
#  define RT_MAX(Value1, Value2)                  ( (Value1) >= (Value2) ? (Value1) : (Value2) )
# endif

#endif /* defined(RT_OS_OS2) */

/* Include iprt/types.h (which also includes iprt/types.h) now to make sure iprt
 * gets to stdint.h first, otherwise a system/xpcom header might beat us and
 * we'll be without the macros that are optional in C++. */
#include <iprt/types.h>



/** @defgroup grp_com_defs  Common Definitions
 * @ingroup grp_com
 * @{
 */

#if !defined(VBOX_WITH_XPCOM)

#ifdef RT_OS_WINDOWS

// Windows COM
/////////////////////////////////////////////////////////////////////////////

# include <iprt/win/objbase.h>
# ifndef VBOX_COM_NO_ATL

/* Do not use actual ATL, just provide a superficial lookalike ourselves. */
#  include <VBox/com/microatl.h>
# endif /* VBOX_COM_NO_ATL */

# define NS_DECL_ISUPPORTS
# define NS_IMPL_ISUPPORTS1_CI(a, b)

/* these are XPCOM only, one for every interface implemented */
# define NS_DECL_ISUPPORTS

/** Returns @c true if @a rc represents a warning result code */
# define SUCCEEDED_WARNING(rc)   (SUCCEEDED(rc) && (rc) != S_OK)

/** Tests is a COM result code indicates that the process implementing the
 * interface is dead.
 *
 * COM status codes:
 *      0x800706ba - RPC_S_SERVER_UNAVAILABLE.  Killed before call was made.
 *      0x800706be - RPC_S_CALL_FAILED.         Killed after call was made.
 *      0x800706bf - RPC_S_CALL_FAILED_DNE.     Not observed, but should be
 *                                              matter of timing.
 *      0x80010108 - RPC_E_DISCONNECTED.        Observed deregistering
 *                                              python event listener.
 *      0x800706b5 - RPC_S_UNKNOWN_IF.          Observed deregistering python
 *                                              event listener
 */
#define FAILED_DEAD_INTERFACE(rc) \
    (   (rc) == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) \
     || (rc) == HRESULT_FROM_WIN32(RPC_S_CALL_FAILED) \
     || (rc) == HRESULT_FROM_WIN32(RPC_S_CALL_FAILED_DNE) \
     || (rc) == RPC_E_DISCONNECTED \
    )

/** Immutable BSTR string */
typedef const OLECHAR *CBSTR;

/** Input BSTR argument of interface method declaration. */
#define IN_BSTR BSTR

/** Input GUID argument of interface method declaration. */
#define IN_GUID GUID
/** Output GUID argument of interface method declaration. */
#define OUT_GUID GUID *

/** Makes the name of the getter interface function (n must be capitalized). */
#define COMGETTER(n)    get_##n
/** Makes the name of the setter interface function (n must be capitalized). */
#define COMSETTER(n)    put_##n

/**
 * Declares an input safearray parameter in the COM method implementation. Also
 * used to declare the COM attribute setter parameter. Corresponds to either of
 * the following XIDL definitions:
 * <pre>
 *  &lt;param name="arg" ... dir="in" safearray="yes"/&gt;
 *  ...
 *  &lt;attribute name="arg" ... safearray="yes"/&gt;
 * </pre>
 *
 * The method implementation should use the com::SafeArray helper class to work
 * with parameters declared using this define.
 *
 * @param aType Array element type.
 * @param aArg  Parameter/attribute name.
 */
#define ComSafeArrayIn(aType, aArg)     SAFEARRAY *aArg

/**
 * Expands to @c true if the given input safearray parameter is a "null pointer"
 * which makes it impossible to use it for reading safearray data.
 */
#define ComSafeArrayInIsNull(aArg)      ((aArg) == NULL)

/**
 * Wraps the given parameter name to generate an expression that is suitable for
 * passing the parameter to functions that take input safearray parameters
 * declared using the ComSafeArrayIn macro.
 *
 * @param aArg  Parameter name to wrap. The given parameter must be declared
 *              within the calling function using the ComSafeArrayIn macro.
 */
#define ComSafeArrayInArg(aArg)         aArg

/**
 * Declares an output safearray parameter in the COM method implementation. Also
 * used to declare the COM attribute getter parameter. Corresponds to either of
 * the following XIDL definitions:
 * <pre>
 *  &lt;param name="arg" ... dir="out" safearray="yes"/&gt;
 *  &lt;param name="arg" ... dir="return" safearray="yes"/&gt;
 *  ...
 *  &lt;attribute name="arg" ... safearray="yes"/&gt;
 * </pre>
 *
 * The method implementation should use the com::SafeArray helper class to work
 * with parameters declared using this define.
 *
 * @param aType Array element type.
 * @param aArg  Parameter/attribute name.
 */
#define ComSafeArrayOut(aType, aArg)    SAFEARRAY **aArg

/**
 * Expands to @c true if the given output safearray parameter is a "null
 * pointer" which makes it impossible to use it for returning a safearray.
 */
#define ComSafeArrayOutIsNull(aArg)     ((aArg) == NULL)

/**
 * Wraps the given parameter name to generate an expression that is suitable for
 * passing the parameter to functions that take output safearray parameters
 * declared using the ComSafeArrayOut marco.
 *
 * @param aArg  Parameter name to wrap. The given parameter must be declared
 *              within the calling function using the ComSafeArrayOut macro.
 */
#define ComSafeArrayOutArg(aArg)        aArg

/**
 * Version of ComSafeArrayIn for GUID.
 * @param aArg Parameter name to wrap.
 */
#define ComSafeGUIDArrayIn(aArg)        SAFEARRAY *aArg

/**
 * Version of ComSafeArrayInIsNull for GUID.
 * @param aArg Parameter name to wrap.
 */
#define ComSafeGUIDArrayInIsNull(aArg)  ComSafeArrayInIsNull(aArg)

/**
 * Version of ComSafeArrayInArg for GUID.
 * @param aArg Parameter name to wrap.
 */
#define ComSafeGUIDArrayInArg(aArg)     ComSafeArrayInArg(aArg)

/**
 * Version of ComSafeArrayOut for GUID.
 * @param aArg Parameter name to wrap.
 */
#define ComSafeGUIDArrayOut(aArg)       SAFEARRAY **aArg

/**
 * Version of ComSafeArrayOutIsNull for GUID.
 * @param aArg Parameter name to wrap.
 */
#define ComSafeGUIDArrayOutIsNull(aArg) ComSafeArrayOutIsNull(aArg)

/**
 * Version of ComSafeArrayOutArg for GUID.
 * @param aArg Parameter name to wrap.
 */
#define ComSafeGUIDArrayOutArg(aArg)    ComSafeArrayOutArg(aArg)

/**
 * Gets size of safearray parameter.
 * @param aArg Parameter name.
 */
#define ComSafeArraySize(aArg)          ((aArg) == NULL ? 0 : (aArg)->rgsabound[0].cElements)

/**
 * Apply RT_NOREF_PV to a safearray parameter.
 * @param aArg Parameter name.
 */
#define ComSafeArrayNoRef(aArg)         RT_NOREF_PV(aArg)

/**
 *  Returns the const reference to the IID (i.e., |const GUID &|) of the given
 *  interface.
 *
 *  @param I    interface class
 */
#define COM_IIDOF(I) __uuidof(I)

/**
 * For using interfaces before including the interface definitions. This will
 * deal with XPCOM using 'class' and COM using 'struct' when defining
 * interfaces.
 *
 * @param   I   interface name.
 */
#define COM_STRUCT_OR_CLASS(I)  struct I

#else /* defined(RT_OS_WINDOWS) */

#error "VBOX_WITH_XPCOM must be defined on a platform other than Windows!"

#endif /* defined(RT_OS_WINDOWS) */

#else /* !defined(VBOX_WITH_XPCOM) */

// XPCOM
/////////////////////////////////////////////////////////////////////////////

#if defined(RT_OS_DARWIN) || (defined(QT_VERSION) && (QT_VERSION >= 0x040000))
  /* CFBase.h defines these &
   * qglobal.h from Qt4 defines these */
# undef FALSE
# undef TRUE
#endif  /* RT_OS_DARWIN || QT_VERSION */

#include <nsID.h>

#define HRESULT     nsresult
#define SUCCEEDED   NS_SUCCEEDED
#define FAILED      NS_FAILED

#define SUCCEEDED_WARNING(rc)   (NS_SUCCEEDED(rc) && (rc) != NS_OK)

#define FAILED_DEAD_INTERFACE(rc)  (   (rc) == NS_ERROR_ABORT \
                                    || (rc) == NS_ERROR_CALL_FAILED \
                                   )

#define IUnknown nsISupports

#define BOOL    PRBool
#define BYTE    PRUint8
#define SHORT   PRInt16
#define USHORT  PRUint16
#define LONG    PRInt32
#define ULONG   PRUint32
#define LONG64  PRInt64
#define ULONG64 PRUint64
/* XPCOM has only 64bit floats */
#define FLOAT   PRFloat64
#define DOUBLE  PRFloat64

#define FALSE   PR_FALSE
#define TRUE    PR_TRUE

#define OLECHAR wchar_t

/* note: typedef to semantically match BSTR on Win32 */
typedef PRUnichar *BSTR;
typedef const PRUnichar *CBSTR;
typedef BSTR *LPBSTR;

/** Input BSTR argument the interface method declaration. */
#define IN_BSTR CBSTR

/**
 * Type to define a raw GUID variable (for members use the com::Guid class
 * instead).
 */
#define GUID        nsID
/** Input GUID argument the interface method declaration. */
#define IN_GUID     const nsID &
/** Output GUID argument the interface method declaration. */
#define OUT_GUID    nsID **

/** Makes the name of the getter interface function (n must be capitalized). */
#define COMGETTER(n)    Get##n
/** Makes the name of the setter interface function (n must be capitalized). */
#define COMSETTER(n)    Set##n

/* safearray input parameter macros */
#define ComSafeArrayIn(aType, aArg)         PRUint32 aArg##Size, aType *aArg
#define ComSafeArrayInIsNull(aArg)          ((aArg) == NULL)
#define ComSafeArrayInArg(aArg)             aArg##Size, aArg

/* safearray output parameter macros */
#define ComSafeArrayOut(aType, aArg)        PRUint32 *aArg##Size, aType **aArg
#define ComSafeArrayOutIsNull(aArg)         ((aArg) == NULL)
#define ComSafeArrayOutArg(aArg)            aArg##Size, aArg

/* safearray input parameter macros for GUID */
#define ComSafeGUIDArrayIn(aArg)            PRUint32 aArg##Size, const nsID **aArg
#define ComSafeGUIDArrayInIsNull(aArg)      ComSafeArrayInIsNull(aArg)
#define ComSafeGUIDArrayInArg(aArg)         ComSafeArrayInArg(aArg)

/* safearray output parameter macros for GUID */
#define ComSafeGUIDArrayOut(aArg)           PRUint32 *aArg##Size, nsID ***aArg
#define ComSafeGUIDArrayOutIsNull(aArg)     ComSafeArrayOutIsNull(aArg)
#define ComSafeGUIDArrayOutArg(aArg)        ComSafeArrayOutArg(aArg)

/** safearray size */
#define ComSafeArraySize(aArg)              ((aArg) == NULL ? 0 : (aArg##Size))

/** NOREF a COM safe array argument. */
#define ComSafeArrayNoRef(aArg)             RT_NOREF2(aArg, aArg##Size)

/* CLSID and IID for compatibility with Win32 */
typedef nsCID   CLSID;
typedef nsIID   IID;

/* OLE error codes */
#define S_OK                ((nsresult)NS_OK)
#define S_FALSE             ((nsresult)1)
#define E_UNEXPECTED        NS_ERROR_UNEXPECTED
#define E_NOTIMPL           NS_ERROR_NOT_IMPLEMENTED
#define E_OUTOFMEMORY       NS_ERROR_OUT_OF_MEMORY
#define E_INVALIDARG        NS_ERROR_INVALID_ARG
#define E_NOINTERFACE       NS_ERROR_NO_INTERFACE
#define E_POINTER           NS_ERROR_NULL_POINTER
#define E_ABORT             NS_ERROR_ABORT
#define E_FAIL              NS_ERROR_FAILURE
/* Note: a better analog for E_ACCESSDENIED would probably be
 * NS_ERROR_NOT_AVAILABLE, but we want binary compatibility for now. */
#define E_ACCESSDENIED      ((nsresult)0x80070005L)

#define STDMETHOD(a) NS_IMETHOD a
#define STDMETHODIMP NS_IMETHODIMP
#define STDMETHOD_(ret, meth) NS_IMETHOD_(ret) meth

#define COM_IIDOF(I) NS_GET_IID(I)

#define COM_STRUCT_OR_CLASS(I) class I

/* helper functions */
extern "C"
{
BSTR SysAllocString(const OLECHAR *sz);
BSTR SysAllocStringByteLen(char const *psz, unsigned int len);
BSTR SysAllocStringLen(const OLECHAR *pch, unsigned int cch);
void SysFreeString(BSTR bstr);
int SysReAllocString(BSTR *pbstr, const OLECHAR *psz);
int SysReAllocStringLen(BSTR *pbstr, const OLECHAR *psz, unsigned int cch);
unsigned int SysStringByteLen(BSTR bstr);
unsigned int SysStringLen(BSTR bstr);
}

#ifndef VBOX_COM_NO_ATL

namespace ATL
{

#define ATL_NO_VTABLE
#define DECLARE_CLASSFACTORY(a)
#define DECLARE_CLASSFACTORY_SINGLETON(a)
#define DECLARE_REGISTRY_RESOURCEID(a)
#define DECLARE_NOT_AGGREGATABLE(a)
#define DECLARE_PROTECT_FINAL_CONSTRUCT()
#define BEGIN_COM_MAP(a)
#define COM_INTERFACE_ENTRY(a)
#define COM_INTERFACE_ENTRY2(a,b)
#define END_COM_MAP() NS_DECL_ISUPPORTS
#define COM_INTERFACE_ENTRY_AGGREGATE(a,b)

/* A few very simple ATL emulator classes to provide
 * FinalConstruct()/FinalRelease() functionality with XPCOM. */

class CComMultiThreadModel
{
};

template <class DummyThreadModel> class CComObjectRootEx
{
public:
    HRESULT FinalConstruct()
    {
        return S_OK;
    }
    void FinalRelease()
    {
    }
};

template <class Base> class CComObject : public Base
{
public:
    virtual ~CComObject() { this->FinalRelease(); }
};

} /* namespace ATL */


/**
 *  'Constructor' for the component class.
 *  This constructor, as opposed to NS_GENERIC_FACTORY_CONSTRUCTOR,
 *  assumes that the component class is derived from the CComObjectRootEx<>
 *  template, so it calls FinalConstruct() right after object creation
 *  and ensures that FinalRelease() will be called right before destruction.
 *  The result from FinalConstruct() is returned to the caller.
 */
#define NS_GENERIC_FACTORY_CONSTRUCTOR_WITH_RC(_InstanceClass)                \
static NS_IMETHODIMP                                                          \
_InstanceClass##Constructor(nsISupports *aOuter, REFNSIID aIID,               \
                            void **aResult)                                   \
{                                                                             \
    nsresult rv;                                                              \
                                                                              \
    *aResult = NULL;                                                          \
    if (NULL != aOuter) {                                                     \
        rv = NS_ERROR_NO_AGGREGATION;                                         \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    ATL::CComObject<_InstanceClass> *inst = new ATL::CComObject<_InstanceClass>(); \
    if (NULL == inst) {                                                       \
        rv = NS_ERROR_OUT_OF_MEMORY;                                          \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    NS_ADDREF(inst); /* protect FinalConstruct() */                           \
    rv = inst->FinalConstruct();                                              \
    if (NS_SUCCEEDED(rv))                                                     \
        rv = inst->QueryInterface(aIID, aResult);                             \
    NS_RELEASE(inst);                                                         \
                                                                              \
    return rv;                                                                \
}

/**
 *  'Constructor' that uses an existing getter function that gets a singleton.
 *  The getter function must have the following prototype:
 *      nsresult _GetterProc(_InstanceClass **inst)
 *  This constructor, as opposed to NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR,
 *  lets the getter function return a result code that is passed back to the
 *  caller that tries to instantiate the object.
 *  NOTE: assumes that getter does an AddRef - so additional AddRef is not done.
 */
#define NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR_WITH_RC(_InstanceClass, _GetterProc) \
static NS_IMETHODIMP                                                          \
_InstanceClass##Constructor(nsISupports *aOuter, REFNSIID aIID,               \
                            void **aResult)                                   \
{                                                                             \
    nsresult rv;                                                              \
                                                                              \
    _InstanceClass *inst = NULL;       /* initialized to shut up gcc */       \
                                                                              \
    *aResult = NULL;                                                          \
    if (NULL != aOuter) {                                                     \
        rv = NS_ERROR_NO_AGGREGATION;                                         \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    rv = _GetterProc(&inst);                                                  \
    if (NS_FAILED(rv))                                                        \
        return rv;                                                            \
                                                                              \
    /* sanity check */                                                        \
    if (NULL == inst)                                                         \
        return NS_ERROR_OUT_OF_MEMORY;                                        \
                                                                              \
    /* NS_ADDREF(inst); */                                                    \
    if (NS_SUCCEEDED(rv)) {                                                   \
        rv = inst->QueryInterface(aIID, aResult);                             \
    }                                                                         \
    NS_RELEASE(inst);                                                         \
                                                                              \
    return rv;                                                                \
}

#endif /* !VBOX_COM_NO_ATL */

#endif /* !defined(VBOX_WITH_XPCOM) */

/**
 *  Declares a wchar_t string literal from the argument.
 *  Necessary to overcome MSC / GCC differences.
 *  @param s    expression to stringify
 */
#if defined(_MSC_VER)
#   define WSTR_LITERAL(s)  L#s
#elif defined(__GNUC__)
#   define WSTR_LITERAL(s)  L""#s
#else
#   error "Unsupported compiler!"
#endif

namespace com
{

#ifndef VBOX_COM_NO_ATL

// use this macro to implement scriptable interfaces
#ifdef RT_OS_WINDOWS
#define VBOX_SCRIPTABLE_IMPL(iface)                                          \
    public ATL::IDispatchImpl<iface, &IID_##iface, &LIBID_VirtualBox,        \
                              kTypeLibraryMajorVersion, kTypeLibraryMinorVersion>

#define VBOX_SCRIPTABLE_DISPATCH_IMPL(iface)                                 \
    STDMETHOD(QueryInterface)(REFIID riid, void **ppObj)                     \
    {                                                                        \
        if (riid == IID_##iface)                                             \
        {                                                                    \
            *ppObj = (iface *)this;                                          \
            AddRef();                                                        \
            return S_OK;                                                     \
        }                                                                    \
        if (riid == IID_IUnknown)                                            \
        {                                                                    \
            *ppObj = (IUnknown *)this;                                       \
            AddRef();                                                        \
            return S_OK;                                                     \
        }                                                                    \
        if (riid == IID_IDispatch)                                           \
        {                                                                    \
            *ppObj = (IDispatch *)this;                                      \
            AddRef();                                                        \
            return S_OK;                                                     \
        }                                                                    \
        *ppObj = NULL;                                                       \
        return E_NOINTERFACE;                                                \
    }
#else
#define VBOX_SCRIPTABLE_IMPL(iface)                     \
    public iface
#define VBOX_SCRIPTABLE_DISPATCH_IMPL(iface)
#endif

#endif /* !VBOX_COM_NO_ATL */

} /* namespace com */

/** @} */

#endif /* !VBOX_INCLUDED_com_defs_h */

