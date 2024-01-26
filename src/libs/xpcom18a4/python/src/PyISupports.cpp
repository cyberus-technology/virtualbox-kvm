/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Python XPCOM language bindings.
 *
 * The Initial Developer of the Original Code is
 * ActiveState Tool Corp.
 * Portions created by the Initial Developer are Copyright (C) 2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mark Hammond <mhammond@skippinet.com.au> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

//
// This code is part of the XPCOM extensions for Python.
//
// Written May 2000 by Mark Hammond.
//
// Based heavily on the Python COM support, which is
// (c) Mark Hammond and Greg Stein.
//
// (c) 2000, ActiveState corp.

#include "PyXPCOM_std.h"
#include "nsISupportsPrimitives.h"

static PRInt32 cInterfaces=0;
static PyObject *g_obFuncMakeInterfaceCount = NULL; // XXX - never released!!!

#ifdef VBOX_DEBUG_LIFETIMES
# include <iprt/log.h>
# include <iprt/stream.h>

/*static*/ RTLISTNODE        Py_nsISupports::g_List;
/*static*/ RTONCE            Py_nsISupports::g_Once = RTONCE_INITIALIZER;
/*static*/ RTCRITSECT        Py_nsISupports::g_CritSect;

/*static*/ DECLCALLBACK(int32_t)
Py_nsISupports::initOnceCallback(void *pvUser1)
{
    NOREF(pvUser1);
    RTListInit(&g_List);
    return RTCritSectInit(&g_CritSect);
}

/*static*/ void
Py_nsISupports::dumpList(void)
{
    RTOnce(&g_Once, initOnceCallback, NULL);
    RTCritSectEnter(&g_CritSect);

    uint32_t i = 0;
    Py_nsISupports *pCur;
    RTListForEach(&g_List, pCur, Py_nsISupports, m_ListEntry)
    {
	nsISupports *pISup = pCur->m_obj;
	PyXPCOM_LogWarning("#%u: %p iid=%RTuuid obj=%p", i, pCur, &pCur->m_iid, pISup);
	i++;
    }

    RTCritSectLeave(&g_CritSect);
}

/*static*/ void
Py_nsISupports::dumpListToStdOut()
{
    RTOnce(&g_Once, initOnceCallback, NULL);
    RTCritSectEnter(&g_CritSect);

    uint32_t i = 0;
    Py_nsISupports *pCur;
    RTListForEach(&g_List, pCur, Py_nsISupports, m_ListEntry)
    {
	nsISupports *pISup = pCur->m_obj;
	RTPrintf("#%u: %p iid=%RTuuid obj=%p\n", i, pCur, &pCur->m_iid, pISup);
	i++;
    }

    RTCritSectLeave(&g_CritSect);
}

PRInt32
_PyXPCOM_DumpInterfaces(void)
{
	Py_nsISupports::dumpListToStdOut();
	return NS_OK;
}

#endif /* _DEBUG_LIFETIMES */



PyObject *PyObject_FromNSInterface( nsISupports *aInterface,
                                    const nsIID &iid,
                                    PRBool bMakeNicePyObject /*= PR_TRUE */)
{
	return Py_nsISupports::PyObjectFromInterface(aInterface, iid,
	                                             bMakeNicePyObject);
}

PRInt32
_PyXPCOM_GetInterfaceCount(void)
{
	return cInterfaces;
}

#ifndef Py_LIMITED_API
Py_nsISupports::Py_nsISupports(nsISupports *punk, const nsIID &iid, PyTypeObject *this_type)
#else
Py_nsISupports::Py_nsISupports(nsISupports *punk, const nsIID &iid, PyXPCOM_TypeObject *this_type)
#endif
{
#ifndef Py_LIMITED_API
	ob_type = this_type;
#else
	ob_type = this_type->m_pTypeObj;
	m_pMyTypeObj = this_type;
#endif
	m_obj = punk;
	m_iid = iid;
	// refcnt of object managed by caller.
	PR_AtomicIncrement(&cInterfaces);
	PyXPCOM_DLLAddRef();
#if 1 /* VBox: Must use for 3.9+, includes _Py_NewReferences. Works for all older versions too. @bugref{10079} */
	PyObject_Init(this, ob_type);
#else
	_Py_NewReference(this);
#endif

#ifdef VBOX_DEBUG_LIFETIMES
        RTOnce(&g_Once, initOnceCallback, NULL);
        RTCritSectEnter(&g_CritSect);
        RTListAppend(&g_List, &m_ListEntry);
        RTCritSectLeave(&g_CritSect);
	PyXPCOM_LogWarning("Creating   %p: iid=%RTuuid obj=%p", this, &m_iid, punk);
#endif
}

Py_nsISupports::~Py_nsISupports()
{
#ifdef VBOX_DEBUG_LIFETIMES
	RTCritSectEnter(&g_CritSect);
	nsISupports *punk = m_obj;
	RTListNodeRemove(&m_ListEntry);
	RTCritSectLeave(&g_CritSect);
	PyXPCOM_LogWarning("Destroying %p: iid=%RTuuid obj=%p", this, &m_iid, punk);
#endif

	SafeRelease(this);
	PR_AtomicDecrement(&cInterfaces);
	PyXPCOM_DLLRelease();
}

/*static*/ nsISupports *
Py_nsISupports::GetI(PyObject *self, nsIID *ret_iid)
{
	if (self==NULL) {
		PyErr_SetString(PyExc_ValueError, "The Python object is invalid");
		return NULL;
	}
	Py_nsISupports *pis = (Py_nsISupports *)self;
	if (pis->m_obj==NULL) {
		// This should never be able to happen.
		PyErr_SetString(PyExc_ValueError, "Internal Error - The XPCOM object has been released.");
		return NULL;
	}
	if (ret_iid)
		*ret_iid = pis->m_iid;
	return pis->m_obj;
}

/*static*/ void
Py_nsISupports::SafeRelease(Py_nsISupports *ob)
{
	if (!ob)
		return;
	if (ob->m_obj)
	{
		Py_BEGIN_ALLOW_THREADS;
		ob->m_obj = nsnull;
		Py_END_ALLOW_THREADS;
	}
}

/* virtual */ PyObject *
Py_nsISupports::getattr(const char *name)
{
	if (strcmp(name, "IID")==0)
		return Py_nsIID::PyObjectFromIID( m_iid );

	// Support for __unicode__ until we get a tp_unicode slot.
	if (strcmp(name, "__unicode__")==0) {
		nsresult rv;
		PRUnichar *val = NULL;
		Py_BEGIN_ALLOW_THREADS;
		{ // scope to kill pointer while thread-lock released.
		nsCOMPtr<nsISupportsString> ss( do_QueryInterface(m_obj, &rv ));
		if (NS_SUCCEEDED(rv))
			rv = ss->ToString(&val);
		} // end-scope
		Py_END_ALLOW_THREADS;
		PyObject *ret = NS_FAILED(rv) ?
			PyXPCOM_BuildPyException(rv) :
			PyObject_FromNSString(val);
		if (val) nsMemory::Free(val);
		return ret;
	}
#ifndef Py_LIMITED_API
	PyXPCOM_TypeObject *this_type = (PyXPCOM_TypeObject *)ob_type;
#else
	PyXPCOM_TypeObject *this_type = m_pMyTypeObj;
#endif
#if PY_MAJOR_VERSION <= 2
	return Py_FindMethodInChain(&this_type->chain, this, (char *)name);
#else
	PyMethodChain *chain = &this_type->chain;
	if (name[0] == '_' && name[1] == '_') {
# ifndef Py_LIMITED_API /** @todo ? */
	    if (!strcmp(name, "__doc__")) {
		const char *doc = ob_type->tp_doc;
		if (doc)
		    return PyUnicode_FromString(doc);
	    }
# endif
	}
	while (chain) {
		PyMethodDef *ml = chain->methods;
		for (; ml->ml_name; ml++) {
			if (!strcmp(name, ml->ml_name))
			    return PyCFunction_New(ml, this);
		}
		chain = chain->link;
	}
	PyErr_SetString(PyExc_AttributeError, name);
	return NULL;
#endif
}

/* virtual */ int
Py_nsISupports::setattr(const char *name, PyObject *v)
{
	char buf[128];
#ifdef VBOX
	snprintf(buf, sizeof(buf), "%s has read-only attributes", PyXPCOM_ObTypeName(this) );
#else
	sprintf(buf, "%s has read-only attributes", PyXPCOM_ObTypeName(this) );
#endif
	PyErr_SetString(PyExc_TypeError, buf);
	return -1;
}

/*static*/ Py_nsISupports *
Py_nsISupports::Constructor(nsISupports *pInitObj, const nsIID &iid)
{
	return new Py_nsISupports(pInitObj,
				       iid,
				       type);
}

PRBool
Py_nsISupports::InterfaceFromPyISupports(PyObject *ob,
                                         const nsIID &iid,
                                         nsISupports **ppv)
{
	nsISupports *pis;
	PRBool rc = PR_FALSE;
	if ( !Check(ob) )
	{
		PyErr_Format(PyExc_TypeError, "Objects of type '%s' can not be used as COM objects", PyXPCOM_ObTypeName(ob));
		goto done;
	}
	nsIID already_iid;
	pis = GetI(ob, &already_iid);
	if ( !pis )
		goto done;	/* exception was set by GetI() */
	/* note: we don't (yet) explicitly hold a reference to pis */
	if (iid.Equals(Py_nsIID_NULL)) {
		// a bit of a hack - we are asking for the arbitary interface
		// wrapped by this object, not some other specific interface -
		// so no QI, just an AddRef();
		Py_BEGIN_ALLOW_THREADS
		pis->AddRef();
		Py_END_ALLOW_THREADS
		*ppv = pis;
	} else {
		// specific interface requested - if it is not already the
		// specific interface, QI for it and discard pis.
		if (iid.Equals(already_iid)) {
			*ppv = pis;
			pis->AddRef();
		} else {
			nsresult r;
			Py_BEGIN_ALLOW_THREADS
			r = pis->QueryInterface(iid, (void **)ppv);
			Py_END_ALLOW_THREADS
			if ( NS_FAILED(r) )
			{
				PyXPCOM_BuildPyException(r);
				goto done;
			}
			/* note: the QI added a ref for the return value */
		}
	}
	rc = PR_TRUE;
done:
	return rc;
}

PRBool
Py_nsISupports::InterfaceFromPyObject(PyObject *ob,
					   const nsIID &iid,
					   nsISupports **ppv,
					   PRBool bNoneOK,
					   PRBool bTryAutoWrap /* = PR_TRUE */)
{
	if ( ob == NULL )
	{
		// don't overwrite an error message
		if ( !PyErr_Occurred() )
			PyErr_SetString(PyExc_TypeError, "The Python object is invalid");
		return PR_FALSE;
	}
	if ( ob == Py_None )
	{
		if ( bNoneOK )
		{
			*ppv = NULL;
			return PR_TRUE;
		}
		else
		{
			PyErr_SetString(PyExc_TypeError, "None is not a invalid interface object in this context");
			return PR_FALSE;
		}
	}

	// support nsIVariant
	if (iid.Equals(NS_GET_IID(nsIVariant)) || iid.Equals(NS_GET_IID(nsIWritableVariant))) {
		// Check it is not already nsIVariant
		if (PyObject_HasAttrString(ob, "__class__")) {
			PyObject *sub_ob = PyObject_GetAttrString(ob, "_comobj_");
			if (sub_ob==NULL) {
				PyErr_Clear();
			} else {
				if (InterfaceFromPyISupports(sub_ob, iid, ppv)) {
					Py_DECREF(sub_ob);
					return PR_TRUE;
				}
				PyErr_Clear();
				Py_DECREF(sub_ob);
			}
		}
		nsresult nr = PyObject_AsVariant(ob, (nsIVariant **)ppv);
		if (NS_FAILED(nr)) {
			PyXPCOM_BuildPyException(nr);
			return PR_FALSE;
		}
		NS_ASSERTION(ppv != nsnull, "PyObject_AsVariant worked but gave null!");
		return PR_TRUE;
	}
	// end of variant support.

	if (PyObject_HasAttrString(ob, "__class__")) {
		// Get the _comobj_ attribute
		PyObject *use_ob = PyObject_GetAttrString(ob, "_comobj_");
		if (use_ob==NULL) {
			PyErr_Clear();
			if (bTryAutoWrap)
				// Try and auto-wrap it - errors will leave Py exception set,
				return PyXPCOM_XPTStub::AutoWrapPythonInstance(ob, iid, ppv);
			PyErr_SetString(PyExc_TypeError, "The Python instance can not be converted to an XPCOM object");
			return PR_FALSE;
		} else
			ob = use_ob;

	} else {
		Py_INCREF(ob);
	}
	PRBool rc = InterfaceFromPyISupports(ob, iid, ppv);
	Py_DECREF(ob);
	return rc;
}


// Interface conversions
/*static*/void
#ifndef Py_LIMITED_API
Py_nsISupports::RegisterInterface( const nsIID &iid, PyTypeObject *t)
#else
Py_nsISupports::RegisterInterface( const nsIID &iid, PyXPCOM_TypeObject *t)
#endif
{
	if (mapIIDToType==NULL)
		mapIIDToType = PyDict_New();

	if (mapIIDToType) {
		PyObject *key = Py_nsIID::PyObjectFromIID(iid);
		if (key)
			PyDict_SetItem(mapIIDToType, key, (PyObject *)t);
		Py_XDECREF(key);
	}
}

/*static */PyObject *
Py_nsISupports::PyObjectFromInterface(nsISupports *pis,
				      const nsIID &riid,
				      PRBool bMakeNicePyObject, /* = PR_TRUE */
				      PRBool bIsInternalCall /* = PR_FALSE */)
{
	// Quick exit.
	if (pis==NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	if (!bIsInternalCall) {
#ifdef NS_DEBUG
		nsISupports *queryResult = nsnull;
		Py_BEGIN_ALLOW_THREADS;
		pis->QueryInterface(riid, (void **)&queryResult);
		Py_END_ALLOW_THREADS;
		NS_ASSERTION(queryResult == pis, "QueryInterface needed");
		NS_IF_RELEASE(queryResult);
#endif
	}

#ifndef Py_LIMITED_API
	PyTypeObject *createType = NULL;
#else
	PyXPCOM_TypeObject *createType = NULL;
#endif
	// If the IID is for nsISupports, dont bother with
	// a map lookup as we know the type!
	if (!riid.Equals(NS_GET_IID(nsISupports))) {
		// Look up the map
		PyObject *obiid = Py_nsIID::PyObjectFromIID(riid);
		if (!obiid) return NULL;

		if (mapIIDToType != NULL)
#ifndef Py_LIMITED_API
			createType = (PyTypeObject *)PyDict_GetItem(mapIIDToType, obiid);
#else
			createType = (PyXPCOM_TypeObject *)PyDict_GetItem(mapIIDToType, obiid);
#endif
		Py_DECREF(obiid);
	}
	if (createType==NULL)
		createType = Py_nsISupports::type;
#ifndef Py_LIMITED_API
	// Check it is indeed one of our types.
	if (!PyXPCOM_TypeObject::IsType(createType)) {
		PyErr_SetString(PyExc_RuntimeError, "The type map is invalid");
		return NULL;
	}
	// we can now safely cast the thing to a PyComTypeObject and use it
	PyXPCOM_TypeObject *myCreateType = (PyXPCOM_TypeObject *)createType;
#else  /* Since the mapIIDToType is only updated by us, there should be no need for the above. */
	PyXPCOM_TypeObject * const myCreateType = createType;
#endif
	if (myCreateType->ctor==NULL) {
		PyErr_SetString(PyExc_TypeError, "The type does not declare a PyCom constructor");
		return NULL;
	}

	Py_nsISupports *ret = (*myCreateType->ctor)(pis, riid);
#ifdef _DEBUG_LIFETIMES
	PyXPCOM_LogF("XPCOM Object created at 0x%0xld, nsISupports at 0x%0xld",
		ret, ret->m_obj);
#endif
	if (ret && bMakeNicePyObject)
		return MakeDefaultWrapper(ret, riid);
	return ret;
}

// Call back into Python, passing a raw nsIInterface object, getting back
// the object to actually pass to Python.
PyObject *
Py_nsISupports::MakeDefaultWrapper(PyObject *pyis,
			     const nsIID &iid)
{
	NS_PRECONDITION(pyis, "NULL pyobject!");
	PyObject *obIID = NULL;
	PyObject *args = NULL;
	PyObject *mod = NULL;
	PyObject *ret = NULL;

	obIID = Py_nsIID::PyObjectFromIID(iid);
	if (obIID==NULL)
		goto done;

	if (g_obFuncMakeInterfaceCount==NULL) {
		PyObject *mod = PyImport_ImportModule("xpcom.client");
		if (mod)
			g_obFuncMakeInterfaceCount = PyObject_GetAttrString(mod, "MakeInterfaceResult");
		Py_XDECREF(mod);
	}
	if (g_obFuncMakeInterfaceCount==NULL) goto done;

	args = Py_BuildValue("OO", pyis, obIID);
	if (args==NULL) goto done;
	ret = PyEval_CallObject(g_obFuncMakeInterfaceCount, args);
done:
	if (PyErr_Occurred()) {
		NS_ABORT_IF_FALSE(ret==NULL, "Have an error, but also a return val!");
		PyXPCOM_LogError("Creating an interface object to be used as a result failed\n");
		PyErr_Clear();
	}
	Py_XDECREF(mod);
	Py_XDECREF(args);
	Py_XDECREF(obIID);
	if (ret==NULL) // eek - error - return the original with no refcount mod.
		ret = pyis;
	else
		// no error - decref the old object
		Py_DECREF(pyis);
	// return our obISupports.  If NULL, we are really hosed and nothing we can do.
	return ret;
}

// @pymethod <o Py_nsISupports>|Py_nsISupports|QueryInterface|Queries an object for a specific interface.
PyObject *
Py_nsISupports::QueryInterface(PyObject *self, PyObject *args)
{
	PyObject *obiid;
	int bWrap = 1;
	// @pyparm IID|iid||The IID requested.
	// @rdesc The result is always a <o Py_nsISupports> object.
	// Any error (including E_NOINTERFACE) will generate a <o com_error> exception.
	if (!PyArg_ParseTuple(args, "O|i:QueryInterface", &obiid, &bWrap))
		return NULL;

	nsIID	iid;
	if (!Py_nsIID::IIDFromPyObject(obiid, &iid))
		return NULL;

	nsISupports *pMyIS = GetI(self);
	if (pMyIS==NULL) return NULL;

	// Optimization, If we already wrap the IID, just return
	// ourself.
	if (!bWrap && iid.Equals(((Py_nsISupports *)self)->m_iid)) {
		Py_INCREF(self);
		return self;
	}

	nsCOMPtr<nsISupports> pis;
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pMyIS->QueryInterface(iid, getter_AddRefs(pis));
	Py_END_ALLOW_THREADS;

	/* Note that this failure may include E_NOINTERFACE */
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	/* Return a type based on the IID (with no extra ref) */
	return ((Py_nsISupports *)self)->MakeInterfaceResult(pis, iid, (PRBool)bWrap);
}


#ifdef VBOX
static PyObject *
QueryErrorObject(PyObject *self, PyObject *args)
{
	nsresult rc = 0;

	if (!PyArg_ParseTuple(args, "i", &rc))
		return NULL;

        return PyXPCOM_BuildErrorMessage(rc);
}
#endif

// @object Py_nsISupports|The base object for all PythonCOM objects.  Wraps a COM nsISupports interface.
/*static*/ struct PyMethodDef
Py_nsISupports::methods[] =
{
	{ "queryInterface", Py_nsISupports::QueryInterface, 1, "Queries the object for an interface."},
	{ "QueryInterface", Py_nsISupports::QueryInterface, 1, "An alias for queryInterface."},
#ifdef VBOX
        { "QueryErrorObject", QueryErrorObject, 1, "Query an error object for given status code."},
#endif
	{NULL}
};

/*static*/void Py_nsISupports::InitType(void)
{
	type = new PyXPCOM_TypeObject(
		"nsISupports",
		NULL,
		sizeof(Py_nsISupports),
		methods,
		Constructor);
}

PyXPCOM_TypeObject *Py_nsISupports::type = NULL;
PyObject *Py_nsISupports::mapIIDToType = NULL;
