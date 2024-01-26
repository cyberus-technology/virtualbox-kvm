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

#include "PyXPCOM.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIFile.h"
#include "nsIComponentRegistrar.h"
#include "nsIComponentManagerObsolete.h"
#include "nsIConsoleService.h"
#include "nspr.h" // PR_fprintf
#ifdef VBOX
# include "nsEventQueueUtils.h"
#endif

#ifdef XP_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "windows.h"
#endif

#include "nsIEventQueue.h"
#include "nsIProxyObjectManager.h"

#define LOADER_LINKS_WITH_PYTHON

#ifndef PYXPCOM_USE_PYGILSTATE
extern PYXPCOM_EXPORT void PyXPCOM_InterpreterState_Ensure();
#endif

#ifdef VBOX_PYXPCOM
# include <iprt/cdefs.h>
# include <VBox/com/com.h>
# ifndef MODULE_NAME_SUFFIX
#  define MANGLE_MODULE_NAME(a_szName)  a_szName
#  define MANGLE_MODULE_INIT(a_Name)    a_Name
# else
#  define MANGLE_MODULE_NAME(a_szName)  a_szName RT_XSTR(MODULE_NAME_SUFFIX)
#  define MANGLE_MODULE_INIT(a_Name)    RT_CONCAT(a_Name, MODULE_NAME_SUFFIX)
# endif
# if defined(VBOX_PYXPCOM_VERSIONED) && !defined(VBOX_PYXPCOM_MAJOR_VERSIONED)
#  if   PY_VERSION_HEX >= 0x030a0000 && PY_VERSION_HEX < 0x030b0000
#   define MODULE_NAME    MANGLE_MODULE_NAME("VBoxPython3_10")
#   define initVBoxPython MANGLE_MODULE_INIT(PyInit_VBoxPython3_10)

#  elif   PY_VERSION_HEX >= 0x03090000 && PY_VERSION_HEX < 0x030a0000
#   define MODULE_NAME    MANGLE_MODULE_NAME("VBoxPython3_9")
#   define initVBoxPython MANGLE_MODULE_INIT(PyInit_VBoxPython3_9)

#  elif PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#   define MODULE_NAME    MANGLE_MODULE_NAME("VBoxPython3_8")
#   define initVBoxPython MANGLE_MODULE_INIT(PyInit_VBoxPython3_8)

#  elif PY_VERSION_HEX >= 0x03070000 && PY_VERSION_HEX < 0x03080000
#   define MODULE_NAME    MANGLE_MODULE_NAME("VBoxPython3_7")
#   define initVBoxPython MANGLE_MODULE_INIT(PyInit_VBoxPython3_7)

#  elif PY_VERSION_HEX >= 0x03060000 && PY_VERSION_HEX < 0x03070000
#   define MODULE_NAME    MANGLE_MODULE_NAME("VBoxPython3_6")
#   define initVBoxPython MANGLE_MODULE_INIT(PyInit_VBoxPython3_6)

#  elif PY_VERSION_HEX >= 0x03050000 && PY_VERSION_HEX < 0x03060000
#   define MODULE_NAME    MANGLE_MODULE_NAME("VBoxPython3_5")
#   define initVBoxPython MANGLE_MODULE_INIT(PyInit_VBoxPython3_5)

#  elif PY_VERSION_HEX >= 0x03040000 && PY_VERSION_HEX < 0x03050000
#   define MODULE_NAME    MANGLE_MODULE_NAME("VBoxPython3_4")
#   define initVBoxPython MANGLE_MODULE_INIT(PyInit_VBoxPython3_4)

#  elif PY_VERSION_HEX >= 0x03030000 && PY_VERSION_HEX < 0x03040000
#   define MODULE_NAME    MANGLE_MODULE_NAME("VBoxPython3_3")
#   define initVBoxPython MANGLE_MODULE_INIT(PyInit_VBoxPython3_3)

#  elif PY_VERSION_HEX >= 0x03020000 && PY_VERSION_HEX < 0x03030000
#   define MODULE_NAME    MANGLE_MODULE_NAME("VBoxPython3_2")
#   define initVBoxPython MANGLE_MODULE_INIT(PyInit_VBoxPython3_2)

#  elif PY_VERSION_HEX >= 0x03010000 && PY_VERSION_HEX < 0x03020000
#   define MODULE_NAME    MANGLE_MODULE_NAME("VBoxPython3_1")
#   define initVBoxPython MANGLE_MODULE_INIT(PyInit_VBoxPython3_1)

#  elif PY_VERSION_HEX >= 0x02080000 && PY_VERSION_HEX < 0x02090000
#   define MODULE_NAME    MANGLE_MODULE_NAME("VBoxPython2_8")
#   define initVBoxPython MANGLE_MODULE_INIT(initVBoxPython2_8)

#  elif PY_VERSION_HEX >= 0x02070000 && PY_VERSION_HEX < 0x02080000
#   define MODULE_NAME    MANGLE_MODULE_NAME("VBoxPython2_7")
#   define initVBoxPython MANGLE_MODULE_INIT(initVBoxPython2_7)

#  elif PY_VERSION_HEX >= 0x02060000 && PY_VERSION_HEX < 0x02070000
#   define MODULE_NAME    MANGLE_MODULE_NAME("VBoxPython2_6")
#   define initVBoxPython MANGLE_MODULE_INIT(initVBoxPython2_6)
#  else
#   error "Fix module versioning. This Python version is not recognized."
#  endif
# else
#  if PY_MAJOR_VERSION <= 2 && defined(VBOX_PYXPCOM_MAJOR_VERSIONED)
#   define MODULE_NAME 	  MANGLE_MODULE_NAME("VBoxPython2")
#   define initVBoxPython MANGLE_MODULE_INIT(initVBoxPython2)
#  elif PY_MAJOR_VERSION <= 2
#   define MODULE_NAME 	  MANGLE_MODULE_NAME("VBoxPython")
#   define initVBoxPython MANGLE_MODULE_INIT(initVBoxPython)
#  elif defined(Py_LIMITED_API) || defined(VBOX_PYXPCOM_MAJOR_VERSIONED)
#   define MODULE_NAME 	  MANGLE_MODULE_NAME("VBoxPython3")
#   define initVBoxPython MANGLE_MODULE_INIT(PyInit_VBoxPython3)
#  else
#   define MODULE_NAME 	  MANGLE_MODULE_NAME("VBoxPython")
#   define initVBoxPython MANGLE_MODULE_INIT(PyInit_VBoxPython)
#  endif
# endif
#else
#define MODULE_NAME "_xpcom"
#endif

// "boot-strap" methods - interfaces we need to get the base
// interface support!

#ifndef VBOX
/* deprecated, included for backward compatibility */
static PyObject *
PyXPCOMMethod_NS_GetGlobalComponentManager(PyObject *self, PyObject *args)
{
	if (PyErr_Warn(PyExc_DeprecationWarning, "Use GetComponentManager instead") < 0)
	       return NULL;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	nsCOMPtr<nsIComponentManager> cm;
	nsresult rv;
	Py_BEGIN_ALLOW_THREADS;
	rv = NS_GetComponentManager(getter_AddRefs(cm));
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(rv) )
		return PyXPCOM_BuildPyException(rv);

	nsCOMPtr<nsIComponentManagerObsolete> ocm(do_QueryInterface(cm, &rv));
	if ( NS_FAILED(rv) )
		return PyXPCOM_BuildPyException(rv);

	return Py_nsISupports::PyObjectFromInterface(ocm, NS_GET_IID(nsIComponentManagerObsolete), PR_FALSE);
}
#endif

static PyObject *
PyXPCOMMethod_GetComponentManager(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	nsCOMPtr<nsIComponentManager> cm;
	nsresult rv;
	Py_BEGIN_ALLOW_THREADS;
	rv = NS_GetComponentManager(getter_AddRefs(cm));
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(rv) )
		return PyXPCOM_BuildPyException(rv);

	return Py_nsISupports::PyObjectFromInterface(cm, NS_GET_IID(nsIComponentManager), PR_FALSE);
}

// No xpcom callable way to get at the registrar, even though the interface
// is scriptable.
static PyObject *
PyXPCOMMethod_GetComponentRegistrar(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	nsCOMPtr<nsIComponentRegistrar> cm;
	nsresult rv;
	Py_BEGIN_ALLOW_THREADS;
	rv = NS_GetComponentRegistrar(getter_AddRefs(cm));
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(rv) )
		return PyXPCOM_BuildPyException(rv);

	return Py_nsISupports::PyObjectFromInterface(cm, NS_GET_IID(nsISupports), PR_FALSE);
}

static PyObject *
PyXPCOMMethod_GetServiceManager(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	nsCOMPtr<nsIServiceManager> sm;
	nsresult rv;
	Py_BEGIN_ALLOW_THREADS;
	rv = NS_GetServiceManager(getter_AddRefs(sm));
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(rv) )
		return PyXPCOM_BuildPyException(rv);

	// Return a type based on the IID.
	return Py_nsISupports::PyObjectFromInterface(sm, NS_GET_IID(nsIServiceManager));
}

#ifndef VBOX
/* deprecated, included for backward compatibility */
static PyObject *
PyXPCOMMethod_GetGlobalServiceManager(PyObject *self, PyObject *args)
{
	if (PyErr_Warn(PyExc_DeprecationWarning, "Use GetServiceManager instead") < 0)
		return NULL;

	return PyXPCOMMethod_GetComponentManager(self, args);
}
#endif

static PyObject *
PyXPCOMMethod_XPTI_GetInterfaceInfoManager(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	nsIInterfaceInfoManager* im;
	Py_BEGIN_ALLOW_THREADS;
	im = XPTI_GetInterfaceInfoManager();
	Py_END_ALLOW_THREADS;
	if ( im == nsnull )
		return PyXPCOM_BuildPyException(NS_ERROR_FAILURE);

	/* Return a type based on the IID (with no extra ref) */
	// Can not auto-wrap the interface info manager as it is critical to
	// building the support we need for autowrap.
	PyObject *ret = Py_nsISupports::PyObjectFromInterface(im, NS_GET_IID(nsIInterfaceInfoManager), PR_FALSE);
	NS_IF_RELEASE(im);
	return ret;
}

static PyObject *
PyXPCOMMethod_XPTC_InvokeByIndex(PyObject *self, PyObject *args)
{
	PyObject *obIS, *obParams;
	nsCOMPtr<nsISupports> pis;
	int index;

	// We no longer rely on PyErr_Occurred() for our error state,
	// but keeping this assertion can't hurt - it should still always be true!
	NS_WARN_IF_FALSE(!PyErr_Occurred(), "Should be no pending Python error!");

	if (!PyArg_ParseTuple(args, "OiO", &obIS, &index, &obParams))
		return NULL;

	if (!Py_nsISupports::Check(obIS)) {
		return PyErr_Format(PyExc_TypeError,
		                    "First param must be a native nsISupports wrapper (got %s)",
				    PyXPCOM_ObTypeName(obIS));
	}
	// Ack!  We must ask for the "native" interface supported by
	// the object, not specifically nsISupports, else we may not
	// back the same pointer (eg, Python, following identity rules,
	// will return the "original" gateway when QI'd for nsISupports)
	if (!Py_nsISupports::InterfaceFromPyObject(
			obIS,
			Py_nsIID_NULL,
			getter_AddRefs(pis),
			PR_FALSE))
		return NULL;

	PyXPCOM_InterfaceVariantHelper arg_helper((Py_nsISupports *)obIS, index);
	if (!arg_helper.Init(obParams))
		return NULL;

	if (!arg_helper.FillArray())
		return NULL;

	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = XPTC_InvokeByIndex(pis, index, arg_helper.m_num_array, arg_helper.m_var_array);
/** @todo bird: Maybe we could processing pending XPCOM events here to make
 *        life a bit simpler inside python? */
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	return arg_helper.MakePythonResult();
}

static PyObject *
PyXPCOMMethod_WrapObject(PyObject *self, PyObject *args)
{
	PyObject *ob, *obIID;
	int bWrapClient = 1;
	if (!PyArg_ParseTuple(args, "OO|i", &ob, &obIID, &bWrapClient))
		return NULL;

	nsIID	iid;
	if (!Py_nsIID::IIDFromPyObject(obIID, &iid))
		return NULL;

	nsCOMPtr<nsISupports> ret;
	nsresult r = PyXPCOM_XPTStub::CreateNew(ob, iid, getter_AddRefs(ret));
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	// _ALL_ wrapped objects are associated with a weak-ref
	// to their "main" instance.
	AddDefaultGateway(ob, ret); // inject a weak reference to myself into the instance.

	// Now wrap it in an interface.
	return Py_nsISupports::PyObjectFromInterface(ret, iid, bWrapClient);
}

static PyObject *
PyXPCOMMethod_UnwrapObject(PyObject *self, PyObject *args)
{
	PyObject *ob;
	if (!PyArg_ParseTuple(args, "O", &ob))
		return NULL;

	nsISupports *uob = NULL;
	nsIInternalPython *iob = NULL;
	PyObject *ret = NULL;
	if (!Py_nsISupports::InterfaceFromPyObject(ob,
				NS_GET_IID(nsISupports),
				&uob,
				PR_FALSE))
		goto done;
	if (NS_FAILED(uob->QueryInterface(NS_GET_IID(nsIInternalPython), reinterpret_cast<void **>(&iob)))) {
		PyErr_SetString(PyExc_ValueError, "This XPCOM object is not implemented by Python");
		goto done;
	}
	ret = iob->UnwrapPythonObject();
done:
	Py_BEGIN_ALLOW_THREADS;
	NS_IF_RELEASE(uob);
	NS_IF_RELEASE(iob);
	Py_END_ALLOW_THREADS;
	return ret;
}

// @pymethod int|pythoncom|_GetInterfaceCount|Retrieves the number of interface objects currently in existance
static PyObject *
PyXPCOMMethod_GetInterfaceCount(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":_GetInterfaceCount"))
		return NULL;
	return PyInt_FromLong(_PyXPCOM_GetInterfaceCount());
	// @comm If is occasionally a good idea to call this function before your Python program
	// terminates.  If this function returns non-zero, then you still have PythonCOM objects
	// alive in your program (possibly in global variables).
}

#ifdef VBOX_DEBUG_LIFETIMES
// @pymethod int|pythoncom|_DumpInterfaces|Dumps the interfaces still in existance to standard output
static PyObject *
PyXPCOMMethod_DumpInterfaces(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":_DumpInterfaces"))
		return NULL;
	return PyInt_FromLong(_PyXPCOM_DumpInterfaces());
}
#endif

// @pymethod int|pythoncom|_GetGatewayCount|Retrieves the number of gateway objects currently in existance
static PyObject *
PyXPCOMMethod_GetGatewayCount(PyObject *self, PyObject *args)
{
	// @comm This is the number of Python object that implement COM servers which
	// are still alive (ie, serving a client).  The only way to reduce this count
	// is to have the process which uses these PythonCOM servers release its references.
	if (!PyArg_ParseTuple(args, ":_GetGatewayCount"))
		return NULL;
	return PyInt_FromLong(_PyXPCOM_GetGatewayCount());
}

static PyObject *
PyXPCOMMethod_NS_ShutdownXPCOM(PyObject *self, PyObject *args)
{
	// @comm This is the number of Python object that implement COM servers which
	// are still alive (ie, serving a client).  The only way to reduce this count
	// is to have the process which uses these PythonCOM servers release its references.
	if (!PyArg_ParseTuple(args, ":NS_ShutdownXPCOM"))
		return NULL;
	nsresult nr;
	Py_BEGIN_ALLOW_THREADS;
	nr = NS_ShutdownXPCOM(nsnull);
	Py_END_ALLOW_THREADS;

#ifdef VBOX_DEBUG_LIFETIME
	Py_nsISupports::dumpList();
#endif

	// Dont raise an exception - as we are probably shutting down
	// and dont really case - just return the status
	return PyInt_FromLong(nr);
}

static NS_DEFINE_CID(kProxyObjectManagerCID, NS_PROXYEVENT_MANAGER_CID);

// A hack to work around their magic constants!
static PyObject *
PyXPCOMMethod_GetProxyForObject(PyObject *self, PyObject *args)
{
	PyObject *obQueue, *obIID, *obOb;
	int flags;
	if (!PyArg_ParseTuple(args, "OOOi", &obQueue, &obIID, &obOb, &flags))
		return NULL;
	nsIID iid;
	if (!Py_nsIID::IIDFromPyObject(obIID, &iid))
		return NULL;
	nsCOMPtr<nsISupports> pob;
	if (!Py_nsISupports::InterfaceFromPyObject(obOb, iid, getter_AddRefs(pob), PR_FALSE))
		return NULL;
	nsIEventQueue *pQueue = NULL;
	nsIEventQueue *pQueueRelease = NULL;

	if (PyInt_Check(obQueue)) {
		pQueue = (nsIEventQueue *)PyInt_AsLong(obQueue);
	} else {
		if (!Py_nsISupports::InterfaceFromPyObject(obQueue, NS_GET_IID(nsIEventQueue), (nsISupports **)&pQueue, PR_TRUE))
			return NULL;
		pQueueRelease = pQueue;
	}

	nsresult rv_proxy;
	nsCOMPtr<nsISupports> presult;
	Py_BEGIN_ALLOW_THREADS;
	nsCOMPtr<nsIProxyObjectManager> proxyMgr =
	         do_GetService(kProxyObjectManagerCID, &rv_proxy);

	if ( NS_SUCCEEDED(rv_proxy) ) {
		rv_proxy = proxyMgr->GetProxyForObject(pQueue,
				iid,
				pob,
				flags,
				getter_AddRefs(presult));
	}
	if (pQueueRelease)
		pQueueRelease->Release();
	Py_END_ALLOW_THREADS;

	PyObject *result;
	if (NS_SUCCEEDED(rv_proxy) ) {
		result = Py_nsISupports::PyObjectFromInterface(presult, iid);
	} else {
		result = PyXPCOM_BuildPyException(rv_proxy);
	}
	return result;
}

static PyObject *
PyXPCOMMethod_MakeVariant(PyObject *self, PyObject *args)
{
	PyObject *ob;
	if (!PyArg_ParseTuple(args, "O:MakeVariant", &ob))
		return NULL;
	nsCOMPtr<nsIVariant> pVar;
	nsresult nr = PyObject_AsVariant(ob, getter_AddRefs(pVar));
	if (NS_FAILED(nr))
		return PyXPCOM_BuildPyException(nr);
	if (pVar == nsnull) {
		NS_ERROR("PyObject_AsVariant worked but returned a NULL ptr!");
		return PyXPCOM_BuildPyException(NS_ERROR_UNEXPECTED);
	}
	return Py_nsISupports::PyObjectFromInterface(pVar, NS_GET_IID(nsIVariant));
}

static PyObject *
PyXPCOMMethod_GetVariantValue(PyObject *self, PyObject *args)
{
	PyObject *ob, *obParent = NULL;
	if (!PyArg_ParseTuple(args, "O|O:GetVariantValue", &ob, &obParent))
		return NULL;

	nsCOMPtr<nsIVariant> var;
	if (!Py_nsISupports::InterfaceFromPyObject(ob,
				NS_GET_IID(nsISupports),
				getter_AddRefs(var),
				PR_FALSE))
		return PyErr_Format(PyExc_ValueError,
				    "Object is not an nsIVariant (got %s)",
				    PyXPCOM_ObTypeName(ob));

	Py_nsISupports *parent = nsnull;
	if (obParent && obParent != Py_None) {
		if (!Py_nsISupports::Check(obParent)) {
			PyErr_SetString(PyExc_ValueError,
					"Object not an nsISupports wrapper");
			return NULL;
		}
		parent = (Py_nsISupports *)obParent;
	}
	return PyObject_FromVariant(parent, var);
}

PyObject *PyGetSpecialDirectory(PyObject *self, PyObject *args)
{
	char *dirname;
	if (!PyArg_ParseTuple(args, "s:GetSpecialDirectory", &dirname))
		return NULL;
	nsCOMPtr<nsIFile> file;
	nsresult r = NS_GetSpecialDirectory(dirname, getter_AddRefs(file));
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);
	// returned object swallows our reference.
	return Py_nsISupports::PyObjectFromInterface(file, NS_GET_IID(nsIFile));
}

PyObject *AllocateBuffer(PyObject *self, PyObject *args)
{
	int bufSize;
	if (!PyArg_ParseTuple(args, "i", &bufSize))
		return NULL;
#if PY_MAJOR_VERSION <= 2
	return PyBuffer_New(bufSize);
#else
    return PyBytes_FromStringAndSize(NULL, bufSize);
#endif
}

// Writes a message to the console service.  This could be done via pure
// Python code, but is useful when the logging code is actually the
// xpcom .py framework itself (ie, we don't want our logging framework to
// call back into the very code generating the log messages!
PyObject *LogConsoleMessage(PyObject *self, PyObject *args)
{
	char *msg;
	if (!PyArg_ParseTuple(args, "s", &msg))
		return NULL;

	nsCOMPtr<nsIConsoleService> consoleService = do_GetService(NS_CONSOLESERVICE_CONTRACTID);
	if (consoleService)
		consoleService->LogStringMessage(NS_ConvertASCIItoUCS2(msg).get());
	else {
	// This either means no such service, or in shutdown - hardly worth
	// the warning, and not worth reporting an error to Python about - its
	// log handler would just need to catch and ignore it.
	// And as this is only called by this logging setup, any messages should
	// still go to stderr or a logfile.
		NS_WARNING("pyxpcom can't log console message.");
	}

	Py_INCREF(Py_None);
	return Py_None;
}

#ifdef VBOX

#  include <VBox/com/NativeEventQueue.h>
#  include <iprt/err.h>

static PyObject *
PyXPCOMMethod_WaitForEvents(PyObject *self, PyObject *args)
{
    long lTimeout;
    if (!PyArg_ParseTuple(args, "l", &lTimeout))
        return NULL;

    int rc;
    com::NativeEventQueue* aEventQ = com::NativeEventQueue::getMainEventQueue();
    NS_WARN_IF_FALSE(aEventQ != nsnull, "Null main event queue");
    if (!aEventQ)
    {
        PyErr_SetString(PyExc_TypeError, "the main event queue is NULL");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS

    RTMSINTERVAL cMsTimeout = (RTMSINTERVAL)lTimeout;
    if (lTimeout < 0 || (long)cMsTimeout != lTimeout)
        cMsTimeout = RT_INDEFINITE_WAIT;
    rc = aEventQ->processEventQueue(cMsTimeout);

    Py_END_ALLOW_THREADS
    if (RT_SUCCESS(rc))
        return PyInt_FromLong(0);

    if (   rc == VERR_TIMEOUT
        || rc == VERR_INTERRUPTED)
        return PyInt_FromLong(1);

    if (rc == VERR_INVALID_CONTEXT)
    {
        PyErr_SetString(PyExc_Exception, "wrong thread, use the main thread");
        return NULL;
    }

    return PyInt_FromLong(2);
}

static PyObject*
PyXPCOMMethod_InterruptWait(PyObject *self, PyObject *args)
{
  com::NativeEventQueue* aEventQ = com::NativeEventQueue::getMainEventQueue();
  NS_WARN_IF_FALSE(aEventQ != nsnull, "Null main event queue");
  if (!aEventQ)
      return NULL;

  int rc = aEventQ->interruptEventQueueProcessing();
  return PyBool_FromLong(RT_SUCCESS(rc));
}

static nsresult deinitVBoxPython();

static PyObject*
PyXPCOMMethod_DeinitCOM(PyObject *self, PyObject *args)
{
    nsresult nr;
    Py_BEGIN_ALLOW_THREADS;
    nr = deinitVBoxPython();
    Py_END_ALLOW_THREADS;
    return PyInt_FromLong(nr);
}

static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

static PyObject*
PyXPCOMMethod_AttachThread(PyObject *self, PyObject *args)
{
    nsresult rv;
    PRInt32  result = 0;
    nsCOMPtr<nsIEventQueueService> eqs;

    // Create the Event Queue for this thread...
    Py_BEGIN_ALLOW_THREADS;
    eqs =
      do_GetService(kEventQueueServiceCID, &rv);
    Py_END_ALLOW_THREADS;
    if (NS_FAILED(rv))
    {
      result = 1;
      goto done;
    }

    Py_BEGIN_ALLOW_THREADS;
    rv = eqs->CreateThreadEventQueue();
    Py_END_ALLOW_THREADS;
    if (NS_FAILED(rv))
    {
      result = 2;
      goto done;
    }

 done:
    /** @todo: better throw an exception on error */
    return PyInt_FromLong(result);
}

static PyObject*
PyXPCOMMethod_DetachThread(PyObject *self, PyObject *args)
{
    nsresult rv;
    PRInt32  result = 0;
    nsCOMPtr<nsIEventQueueService> eqs;

    // Destroy the Event Queue for this thread...
    Py_BEGIN_ALLOW_THREADS;
    eqs =
      do_GetService(kEventQueueServiceCID, &rv);
    Py_END_ALLOW_THREADS;
    if (NS_FAILED(rv))
    {
      result = 1;
      goto done;
    }

    Py_BEGIN_ALLOW_THREADS;
    rv = eqs->DestroyThreadEventQueue();
    Py_END_ALLOW_THREADS;
    if (NS_FAILED(rv))
    {
      result = 2;
      goto done;
    }

 done:
    /** @todo: better throw an exception on error */
    return PyInt_FromLong(result);
}

#endif /* VBOX */

extern PYXPCOM_EXPORT PyObject *PyXPCOMMethod_IID(PyObject *self, PyObject *args);

static struct PyMethodDef xpcom_methods[]=
{
	{"GetComponentManager", PyXPCOMMethod_GetComponentManager, 1},
	{"GetComponentRegistrar", PyXPCOMMethod_GetComponentRegistrar, 1},
#ifndef VBOX
	{"NS_GetGlobalComponentManager", PyXPCOMMethod_NS_GetGlobalComponentManager, 1}, // deprecated
#endif
	{"XPTI_GetInterfaceInfoManager", PyXPCOMMethod_XPTI_GetInterfaceInfoManager, 1},
	{"XPTC_InvokeByIndex", PyXPCOMMethod_XPTC_InvokeByIndex, 1},
	{"GetServiceManager", PyXPCOMMethod_GetServiceManager, 1},
#ifndef VBOX
	{"GetGlobalServiceManager", PyXPCOMMethod_GetGlobalServiceManager, 1}, // deprecated
	{"IID", PyXPCOMMethod_IID, 1}, // IID is wrong - deprecated - not just IID, but CID, etc.
#endif
	{"ID", PyXPCOMMethod_IID, 1}, // This is the official name.
	{"NS_ShutdownXPCOM", PyXPCOMMethod_NS_ShutdownXPCOM, 1},
	{"WrapObject", PyXPCOMMethod_WrapObject, 1},
	{"UnwrapObject", PyXPCOMMethod_UnwrapObject, 1},
	{"_GetInterfaceCount", PyXPCOMMethod_GetInterfaceCount, 1},
	{"_GetGatewayCount", PyXPCOMMethod_GetGatewayCount, 1},
	{"getProxyForObject", PyXPCOMMethod_GetProxyForObject, 1},
	{"GetProxyForObject", PyXPCOMMethod_GetProxyForObject, 1},
	{"GetSpecialDirectory", PyGetSpecialDirectory, 1},
	{"AllocateBuffer", AllocateBuffer, 1},
	{"LogConsoleMessage", LogConsoleMessage, 1, "Write a message to the xpcom console service"},
	{"MakeVariant", PyXPCOMMethod_MakeVariant, 1},
	{"GetVariantValue", PyXPCOMMethod_GetVariantValue, 1},
#ifdef VBOX
	{"WaitForEvents", PyXPCOMMethod_WaitForEvents, 1},
	{"InterruptWait", PyXPCOMMethod_InterruptWait, 1},
	{"DeinitCOM",     PyXPCOMMethod_DeinitCOM, 1},
	{"AttachThread",  PyXPCOMMethod_AttachThread, 1},
	{"DetachThread",  PyXPCOMMethod_DetachThread, 1},
#endif
#ifdef VBOX_DEBUG_LIFETIMES
	{"_DumpInterfaces", PyXPCOMMethod_DumpInterfaces, 1},
#endif
	// These should no longer be used - just use the logging.getLogger('pyxpcom')...
	/* bird: The above comment refers to LogWarning and LogError. Both now removed. */
	{ NULL }
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef xpcom_module =
{
    PyModuleDef_HEAD_INIT,
    MODULE_NAME,    /* name of module */
    NULL,           /* module documentation */
    -1,             /* size of per-interpreter state or -1 if using globals */
    xpcom_methods
};
#endif


#define REGISTER_IID(t) { \
	PyObject *iid_ob = Py_nsIID::PyObjectFromIID(NS_GET_IID(t)); \
	PyDict_SetItemString(dict, "IID_"#t, iid_ob); \
	Py_DECREF(iid_ob); \
	}

#define REGISTER_INT(val) { \
	PyObject *ob = PyInt_FromLong(val); \
	PyDict_SetItemString(dict, #val, ob); \
	Py_DECREF(ob); \
	}


////////////////////////////////////////////////////////////
// The module init code.
//
#if PY_MAJOR_VERSION <= 2
extern "C" NS_EXPORT
void
#else
PyObject *
#endif
init_xpcom() {
	PyObject *oModule;

	// ensure the framework has valid state to work with.
	if (!PyXPCOM_Globals_Ensure())
#if PY_MAJOR_VERSION <= 2
		return;
#else
		return NULL;
#endif

	// Must force Python to start using thread locks
	PyEval_InitThreads();

	// Create the module and add the functions
#if PY_MAJOR_VERSION <= 2
	oModule = Py_InitModule(MODULE_NAME, xpcom_methods);
#else
	oModule = PyModule_Create(&xpcom_module);
#endif

	PyObject *dict = PyModule_GetDict(oModule);
	PyObject *pycom_Error = PyXPCOM_Error;
	if (pycom_Error == NULL || PyDict_SetItemString(dict, "error", pycom_Error) != 0)
	{
		PyErr_SetString(PyExc_MemoryError, "can't define error");
#if PY_MAJOR_VERSION <= 2
		return;
#else
		return NULL;
#endif
	}
#ifndef Py_LIMITED_API
	PyDict_SetItemString(dict, "IIDType", (PyObject *)&Py_nsIID::type);
#else
	PyDict_SetItemString(dict, "IIDType", (PyObject *)Py_nsIID::GetTypeObject());
#endif

	REGISTER_IID(nsISupports);
	REGISTER_IID(nsISupportsCString);
	REGISTER_IID(nsISupportsString);
	REGISTER_IID(nsIModule);
	REGISTER_IID(nsIFactory);
	REGISTER_IID(nsIWeakReference);
	REGISTER_IID(nsISupportsWeakReference);
	REGISTER_IID(nsIClassInfo);
	REGISTER_IID(nsIServiceManager);
	REGISTER_IID(nsIComponentRegistrar);

	// Register our custom interfaces.
	REGISTER_IID(nsIComponentManager);
	REGISTER_IID(nsIInterfaceInfoManager);
	REGISTER_IID(nsIEnumerator);
	REGISTER_IID(nsISimpleEnumerator);
	REGISTER_IID(nsIInterfaceInfo);
	REGISTER_IID(nsIInputStream);
	REGISTER_IID(nsIClassInfo);
	REGISTER_IID(nsIVariant);
	// for backward compatibility:
	REGISTER_IID(nsIComponentManagerObsolete);

	// No good reason not to expose this impl detail, and tests can use it
	REGISTER_IID(nsIInternalPython);
    // We have special support for proxies - may as well add their constants!
    REGISTER_INT(PROXY_SYNC);
    REGISTER_INT(PROXY_ASYNC);
    REGISTER_INT(PROXY_ALWAYS);
    // Build flags that may be useful.
    PyObject *ob = PyBool_FromLong(
#ifdef NS_DEBUG
                                   1
#else
                                   0
#endif
                                   );
    PyDict_SetItemString(dict, "NS_DEBUG", ob);
    Py_DECREF(ob);
#if PY_MAJOR_VERSION >= 3
    return oModule;
#endif
}

#ifdef VBOX_PYXPCOM
# include <VBox/com/com.h>
using namespace com;

# include <iprt/initterm.h>
# include <iprt/string.h>
# include <iprt/alloca.h>
# include <iprt/stream.h>

/** Set if NS_ShutdownXPCOM has been called successfully already and we don't
 * need to do it again during module termination.  This avoids assertion in the
 * VBoxCOM glue code. */
static bool g_fComShutdownAlready = true;

# if PY_MAJOR_VERSION <= 2
extern "C" NS_EXPORT
void
# else
/** @todo r=klaus this is hacky, but as Python3 doesn't deal with ELF
 * visibility, assuming that all globals are visible (which is ugly and not
 * true in our case). */
#  undef PyMODINIT_FUNC
#  define PyMODINIT_FUNC extern "C" NS_EXPORT PyObject*
PyMODINIT_FUNC
# endif
initVBoxPython() { /* NOTE! This name is redefined at the top of the file! */
  static bool s_vboxInited = false;
  if (!s_vboxInited) {
    int rc = 0; /* Error handling in this code is NON-EXISTING. Sigh. */

# if defined(VBOX_PATH_APP_PRIVATE_ARCH) && defined(VBOX_PATH_SHARED_LIBS)
    rc = RTR3InitDll(RTR3INIT_FLAGS_UNOBTRUSIVE);
# else
    const char *home = getenv("VBOX_PROGRAM_PATH");
    if (home) {
      size_t len = strlen(home);
      char *exepath = (char *)alloca(len + 32);
      memcpy(exepath, home, len);
      memcpy(exepath + len, "/pythonfake", sizeof("/pythonfake"));
      rc = RTR3InitEx(RTR3INIT_VER_CUR, RTR3INIT_FLAGS_DLL | RTR3INIT_FLAGS_UNOBTRUSIVE, 0, NULL, exepath);
    } else {
      rc = RTR3InitDll(RTR3INIT_FLAGS_UNOBTRUSIVE);
    }
# endif

    rc = com::Initialize();
    g_fComShutdownAlready = false;

# if PY_MAJOR_VERSION <= 2
    init_xpcom();
# else
    return init_xpcom();
# endif
  }
# if PY_MAJOR_VERSION >= 3
  return NULL;
# endif
}

static
nsresult deinitVBoxPython()
{
  nsresult nr;
  if (!g_fComShutdownAlready)
  {
    nr = com::Shutdown();
    if (!NS_FAILED(nr))
      g_fComShutdownAlready = true;
  }
  else
    nr = NS_ERROR_NOT_INITIALIZED;
  return nr;
}

#endif /* VBOX_PYXPCOM */
