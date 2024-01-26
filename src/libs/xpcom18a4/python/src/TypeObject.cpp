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
#include <nsIInterfaceInfoManager.h>
#include <nsXPCOM.h>
#include <nsISupportsPrimitives.h>

#if defined(Py_LIMITED_API) && defined(RT_OS_LINUX)
# include <features.h>
# ifdef __GLIBC_PREREQ
#  if __GLIBC_PREREQ(2,9)
#   define PYXPCOM_HAVE_PIPE2
#   include <fcntl.h>
#  endif
# endif
#endif


#ifndef Py_LIMITED_API
static PyTypeObject PyInterfaceType_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"interface-type",			/* Name of this type */
	sizeof(PyTypeObject),	/* Basic object size */
	0,			/* Item size for varobject */
	0,			/*tp_dealloc*/
	0,			/*tp_print*/
	PyType_Type.tp_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	PyType_Type.tp_repr,	/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,			/*tp_call*/
	0,			/*tp_str*/
	0,			/* tp_getattro */
	0,			/*tp_setattro */
	0,			/* tp_as_buffer */
	0,			/* tp_flags */
	"Define the behavior of a PythonCOM Interface type.",
};

#else  /* Py_LIMITED_API */

/** The offset of PyTypeObject::ob_name. */
static size_t g_offObTypeNameMember = sizeof(PyVarObject);

/**
 * Base type object for XPCOM interfaces.  Created dynamicially.
 */
static PyTypeObject *g_pPyInterfaceTypeObj = NULL;

/**
 * Gets the base XPCOM interface type object, creating it if needed.
 */
static PyTypeObject *PyXPCOM_CreateInterfaceType(void)
{
	static char g_szTypeDoc[] = "Define the behavior of a PythonCOM Interface type."; /* need non-const */
	PyType_Slot aTypeSlots[] = {
		{ Py_tp_doc,    	g_szTypeDoc },
		{ 0, NULL } /* terminator */
	};
	static const char g_szClassNm[] = "interface-type";
	PyType_Spec TypeSpec = {
		/* .name: */ 		g_szClassNm,
		/* .basicsize: */       0,
		/* .itemsize: */	0,
		/* .flags: */   	Py_TPFLAGS_BASETYPE,
		/* .slots: */		aTypeSlots,
	};

        PyObject *exc_typ = NULL, *exc_val = NULL, *exc_tb = NULL;
        PyErr_Fetch(&exc_typ, &exc_val, &exc_tb); /* goes south in PyType_Ready if we don't clear exceptions first. */

	PyTypeObject *pTypeObj = (PyTypeObject *)PyType_FromSpec(&TypeSpec);
	assert(pTypeObj);

        PyErr_Restore(exc_typ, exc_val, exc_tb);
	g_pPyInterfaceTypeObj = pTypeObj;

	/*
	 * Verify/correct g_offObTypeNameMember.
	 *
	 * Using pipe+write to probe the memory content, banking on the kernel
	 * to return EFAULT when we pass it an invalid address.
	 */
	for (size_t off = sizeof(PyVarObject); off < sizeof(PyVarObject) + 64; off += sizeof(char *)) {
		const char * const pszProbe = *(const char **)((uintptr_t)(pTypeObj) + off);
		if (RT_VALID_PTR(pszProbe)) {
			int fds[2] = { -1, -1 };
# ifdef PYXPCOM_HAVE_PIPE2
			int rc = pipe2(fds, O_CLOEXEC);
# else
			int rc = pipe(fds);
# endif
			if (rc)
				break;

			ssize_t cbWritten = write(fds[1], pszProbe, sizeof(g_szClassNm));
			if (cbWritten == (ssize_t)sizeof(g_szClassNm)) {
				char szReadBack[sizeof(g_szClassNm)];
				ssize_t offRead = 0;
				while (offRead < cbWritten) {
					ssize_t cbRead = read(fds[0], &szReadBack[offRead], cbWritten - offRead);
					if (cbRead >= 0) {
						offRead += cbRead;
					} else if (errno != EINTR)
						break;
				}
				if (   cbWritten == offRead
				    && memcmp(szReadBack, g_szClassNm, sizeof(szReadBack)) == 0) {
					g_offObTypeNameMember = off;
					close(fds[0]);
					close(fds[1]);
					return pTypeObj;
				}
			}
			close(fds[0]);
			close(fds[1]);
		}
	}
	assert(0);

	return pTypeObj;
}

/**
 * Gets the base XPCOM interface type object, creating it if needed.
 */
static PyTypeObject *PyXPCOM_GetInterfaceType(void)
{
	PyTypeObject *pTypeObj = g_pPyInterfaceTypeObj;
	if (pTypeObj)
		return pTypeObj;
	return PyXPCOM_CreateInterfaceType();
}

/**
 * Get the PyTypeObject::ob_name value.
 *
 * @todo This is _horrible_, but there appears to be no simple tp_name getter
 *       till https://bugs.python.org/issue31497 (2017 / 3.7.0).  But even then
 *       it is not part of the limited API.
 */
const char *PyXPCOMGetObTypeName(PyTypeObject *pTypeObj)
{
	return *(const char **)((uintptr_t)(pTypeObj) + g_offObTypeNameMember);
}

#endif /* Py_LIMITED_API */

/*static*/ PRBool
PyXPCOM_TypeObject::IsType(PyTypeObject *t)
{
#if PY_MAJOR_VERSION <= 2
	return t->ob_type == &PyInterfaceType_Type;
#elif !defined(Py_LIMITED_API)
	return Py_TYPE(t) == &PyInterfaceType_Type;
#else
	return Py_TYPE(t) == g_pPyInterfaceTypeObj         /* Typically not the case as t->ob_type is &PyType_Type */
	    || PyType_IsSubtype(t, g_pPyInterfaceTypeObj); /* rather than g_pPyInterfaceTypeObj because of PyType_FromSpec(). */
#endif
}

////////////////////////////////////////////////////////////////////
//
// The type methods
//
/*static*/PyObject *
PyXPCOM_TypeObject::Py_getattr(PyObject *self, char *name)
{
	return ((Py_nsISupports *)self)->getattr(name);
}

/*static*/int
PyXPCOM_TypeObject::Py_setattr(PyObject *op, char *name, PyObject *v)
{
	return ((Py_nsISupports *)op)->setattr(name, v);
}

// @pymethod int|Py_nsISupports|__cmp__|Implements XPCOM rules for object identity.
/*static*/int
PyXPCOM_TypeObject::Py_cmp(PyObject *self, PyObject *other)
{
	// @comm NOTE: Copied from COM - have not confirmed these rules are true for XPCOM
	// @comm As per the XPCOM rules for object identity, both objects are queried for nsISupports, and these values compared.
	// The only meaningful test is for equality - the result of other comparisons is undefined
	// (ie, determined by the object's relative addresses in memory.
	nsISupports *pUnkOther;
	nsISupports *pUnkThis;
	if (!Py_nsISupports::InterfaceFromPyObject(self, NS_GET_IID(nsISupports), &pUnkThis, PR_FALSE))
		return -1;
	if (!Py_nsISupports::InterfaceFromPyObject(other, NS_GET_IID(nsISupports), &pUnkOther, PR_FALSE)) {
		pUnkThis->Release();
		return -1;
	}
	int rc = pUnkThis==pUnkOther ? 0 :
		(pUnkThis < pUnkOther ? -1 : 1);
	pUnkThis->Release();
	pUnkOther->Release();
	return rc;
}

/*static*/PyObject *
PyXPCOM_TypeObject::Py_richcmp(PyObject *self, PyObject *other, int op)
{
    PyObject *result = NULL;
    int rc = Py_cmp(self, other);
    switch (op)
    {
        case Py_LT:
            result = rc < 0 ? Py_True : Py_False;
            break;
        case Py_LE:
            result = rc <= 0 ? Py_True : Py_False;
            break;
        case Py_EQ:
            result = rc == 0 ? Py_True : Py_False;
            break;
        case Py_NE:
            result = rc != 0 ? Py_True : Py_False;
            break;
        case Py_GT:
            result = rc > 0 ? Py_True : Py_False;
            break;
        case Py_GE:
            result = rc >= 0 ? Py_True : Py_False;
            break;
    }
    Py_XINCREF(result);
    return result;
}

// @pymethod int|Py_nsISupports|__hash__|Implement a hash-code for the XPCOM object using XPCOM identity rules.
#if PY_VERSION_HEX >= 0x03020000
/*static*/Py_hash_t PyXPCOM_TypeObject::Py_hash(PyObject *self)
#else
/*static*/long PyXPCOM_TypeObject::Py_hash(PyObject *self)
#endif
{
	// We always return the value of the nsISupports *.
	nsISupports *pUnkThis;
	if (!Py_nsISupports::InterfaceFromPyObject(self, NS_GET_IID(nsISupports), &pUnkThis, PR_FALSE))
		return -1;
#if PY_VERSION_HEX >= 0x03020000
	Py_hash_t ret = _Py_HashPointer(pUnkThis);
#else
	long ret = _Py_HashPointer(pUnkThis);
#endif
	pUnkThis->Release();
	return ret;
}

// @method string|Py_nsISupports|__repr__|Called to create a representation of a Py_nsISupports object
/*static */PyObject *
PyXPCOM_TypeObject::Py_repr(PyObject *self)
{
	// @comm The repr of this object displays both the object's address, and its attached nsISupports's address
	Py_nsISupports *pis = (Py_nsISupports *)self;
	// Try and get the IID name.
	char *iid_repr = nsnull;
	nsCOMPtr<nsIInterfaceInfoManager> iim(do_GetService(
	                NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
	if (iim!=nsnull)
		iim->GetNameForIID(&pis->m_iid, &iid_repr);
	if (iid_repr==nsnull)
		// no IIM available, or it doesnt know the name.
		iid_repr = pis->m_iid.ToString();
	// XXX - need some sort of buffer overflow.
	char buf[512];
#ifdef VBOX
	snprintf(buf, sizeof(buf), "<XPCOM object (%s) at %p/%p>",
	        iid_repr, (void *)self, (void *)pis->m_obj.get());
#else
	sprintf(buf, "<XPCOM object (%s) at 0x%p/0x%p>",
	        iid_repr, (void *)self, (void *)pis->m_obj.get());
#endif
	nsMemory::Free(iid_repr);
#if PY_MAJOR_VERSION <= 2
	return PyString_FromString(buf);
#else
	return PyUnicode_FromString(buf);
#endif
}

/*static */PyObject *
PyXPCOM_TypeObject::Py_str(PyObject *self)
{
	Py_nsISupports *pis = (Py_nsISupports *)self;
	nsresult rv;
	char *val = NULL;
	Py_BEGIN_ALLOW_THREADS;
	{ // scope to kill pointer while thread-lock released.
	nsCOMPtr<nsISupportsCString> ss( do_QueryInterface(pis->m_obj, &rv ));
	if (NS_SUCCEEDED(rv))
		rv = ss->ToString(&val);
	} // end-scope
	Py_END_ALLOW_THREADS;
	PyObject *ret;
	if (NS_FAILED(rv))
		ret = Py_repr(self);
	else
#if PY_MAJOR_VERSION <= 2
		ret = PyString_FromString(val);
#else
		ret = PyUnicode_FromString(val);
#endif
	if (val) nsMemory::Free(val);
	return ret;
}

/* static */void
PyXPCOM_TypeObject::Py_dealloc(PyObject *self)
{
	delete (Py_nsISupports *)self;
}

PyXPCOM_TypeObject::PyXPCOM_TypeObject( const char *name, PyXPCOM_TypeObject *pBase, int typeSize, struct PyMethodDef* methodList, PyXPCOM_I_CTOR thector)
{
#ifndef Py_LIMITED_API
	static const PyTypeObject type_template = {
		PyVarObject_HEAD_INIT(&PyInterfaceType_Type, 0)
		"XPCOMTypeTemplate",                         /*tp_name*/
		sizeof(Py_nsISupports),                      /*tp_basicsize*/
		0,                                           /*tp_itemsize*/
		Py_dealloc,                                  /* tp_dealloc */
		0,                                           /* tp_print */
		Py_getattr,                                  /* tp_getattr */
		Py_setattr,                                  /* tp_setattr */
#if PY_MAJOR_VERSION <= 2
		Py_cmp,                                      /* tp_compare */
#else
		0,                                           /* reserved */
#endif
		Py_repr,                                     /* tp_repr */
		0,                                           /* tp_as_number*/
		0,                                           /* tp_as_sequence */
		0,                                           /* tp_as_mapping */
		Py_hash,                                     /* tp_hash */
		0,                                           /* tp_call */
		Py_str,                                      /* tp_str */
		0,                                           /* tp_getattro */
		0,                                           /* tp_setattro */
		0,                                           /* tp_as_buffer */
		0,                                           /* tp_flags */
		0,                                           /* tp_doc */
		0,                                           /* tp_traverse */
		0,                                           /* tp_clear */
		Py_richcmp,                                  /* tp_richcompare */
		0,                                           /* tp_weaklistoffset */
		0,                                           /* tp_iter */
		0,                                           /* tp_iternext */
		0,                                           /* tp_methods */
		0,                                           /* tp_members */
		0,                                           /* tp_getset */
		0,                                           /* tp_base */
	};

	*((PyTypeObject *)this) = type_template;
#else  /* Py_LIMITED_API */
	/* Create the type specs: */
	PyType_Slot aTypeSlots[] = {
		{ Py_tp_base, 		PyXPCOM_GetInterfaceType() },
		{ Py_tp_dealloc, 	(void *)(uintptr_t)&PyXPCOM_TypeObject::Py_dealloc },
		{ Py_tp_getattr, 	(void *)(uintptr_t)&PyXPCOM_TypeObject::Py_getattr },
		{ Py_tp_setattr, 	(void *)(uintptr_t)&PyXPCOM_TypeObject::Py_setattr },
		{ Py_tp_repr, 		(void *)(uintptr_t)&PyXPCOM_TypeObject::Py_repr },
		{ Py_tp_hash, 		(void *)(uintptr_t)&PyXPCOM_TypeObject::Py_hash },
		{ Py_tp_str, 		(void *)(uintptr_t)&PyXPCOM_TypeObject::Py_str },
		{ Py_tp_richcompare, 	(void *)(uintptr_t)&PyXPCOM_TypeObject::Py_richcmp },
		{ 0, NULL } /* terminator */
	};
	PyType_Spec TypeSpec = {
		/* .name: */ 		name,
		/* .basicsize: */       typeSize,
		/* .itemsize: */	0,
		/* .flags: */   	Py_TPFLAGS_BASETYPE /*?*/,
		/* .slots: */		aTypeSlots,
	};

	PyObject *exc_typ = NULL, *exc_val = NULL, *exc_tb = NULL;
	PyErr_Fetch(&exc_typ, &exc_val, &exc_tb); /* goes south in PyType_Ready if we don't clear exceptions first. */

	m_pTypeObj = (PyTypeObject *)PyType_FromSpec(&TypeSpec);
	assert(m_pTypeObj);

        PyErr_Restore(exc_typ, exc_val, exc_tb);

	/* Initialize the PyObject part - needed so we can keep instance in a PyDict: */
	ob_type = PyXPCOM_GetInterfaceType();
	PyObject_Init(this, ob_type); /* VBox: Needed for 3.9 and up (also works on Python 2.7), includes _Py_NewReferences. @bugref{10079} */

#endif /* Py_LIMITED_API */

	chain.methods = methodList;
	chain.link = pBase ? &pBase->chain : NULL;

	baseType = pBase;
	ctor = thector;

#ifndef Py_LIMITED_API
	// cast away const, as Python doesnt use it.
	tp_name = (char *)name;
	tp_basicsize = typeSize;
#endif
}

PyXPCOM_TypeObject::~PyXPCOM_TypeObject()
{
}

