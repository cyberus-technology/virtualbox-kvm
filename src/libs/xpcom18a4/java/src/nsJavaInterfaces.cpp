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
 * The Original Code is Java XPCOM Bindings.
 *
 * The Initial Developer of the Original Code is IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * IBM Corporation. All Rights Reserved.
 *
 * Contributor(s):
 *   Javier Pedemonte (jhpedemonte@gmail.com)
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

#include "nsJavaInterfaces.h"
#include "nsJavaWrapper.h"
#include "nsJavaXPCOMBindingUtils.h"
#include "nsJavaXPTCStub.h"
#include "nsIComponentRegistrar.h"
#include "nsString.h"
#include "nsISimpleEnumerator.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIInputStream.h"
#include "nsEnumeratorUtils.h"
#include "nsAppFileLocProviderProxy.h"
#ifndef VBOX
#include "nsXULAppAPI.h"
#endif
#include "nsILocalFile.h"

#ifdef XP_MACOSX
#include "jawt.h"
#endif


#ifdef VBOX
#if 0
#include "org_mozilla_xpcom_internal_GREImpl.h"
#include "org_mozilla_xpcom_internal_JavaXPCOMMethods.h"
#include "org_mozilla_xpcom_internal_MozillaImpl.h"
#include "org_mozilla_xpcom_internal_XPCOMImpl.h"
#include "org_mozilla_xpcom_internal_XPCOMJavaProxy.h"
#include "org_mozilla_xpcom_ProfileLock.h"
#endif
#include <VBox/com/com.h>
using namespace com;
#include <iprt/initterm.h>
#include <iprt/string.h>
#include <alloca.h>
#endif

extern "C" NS_EXPORT void JNICALL
MOZILLA_NATIVE(initialize) (JNIEnv* env, jobject)
{
  if (!InitializeJavaGlobals(env)) {
    jclass clazz =
        env->FindClass("org/mozilla/xpcom/XPCOMInitializationException");
    if (clazz) {
      env->ThrowNew(clazz, "Failed to initialize JavaXPCOM");
    }
  }
}

nsresult
InitEmbedding_Impl(JNIEnv* env, jobject aLibXULDirectory,
                   jobject aAppDirectory, jobject aAppDirProvider)
{
  nsresult rv;

  // create an nsILocalFile from given java.io.File
  nsCOMPtr<nsILocalFile> libXULDir;
  if (aLibXULDirectory) {
    rv = File_to_nsILocalFile(env, aLibXULDirectory, getter_AddRefs(libXULDir));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  nsCOMPtr<nsILocalFile> appDir;
  if (aAppDirectory) {
    rv = File_to_nsILocalFile(env, aAppDirectory, getter_AddRefs(appDir));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // create nsAppFileLocProviderProxy from given Java object
  nsCOMPtr<nsIDirectoryServiceProvider> provider;
  if (aAppDirProvider) {
    rv = NS_NewAppFileLocProviderProxy(aAppDirProvider,
                                       getter_AddRefs(provider));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // init libXUL
#ifdef VBOX
  return 0;
#else
  return XRE_InitEmbedding(libXULDir, appDir, provider, nsnull, 0);
#endif
}

extern "C" NS_EXPORT void JNICALL
GRE_NATIVE(initEmbedding) (JNIEnv* env, jobject, jobject aLibXULDirectory,
                           jobject aAppDirectory, jobject aAppDirProvider)
{
  nsresult rv = InitEmbedding_Impl(env, aLibXULDirectory, aAppDirectory,
                                   aAppDirProvider);

  if (NS_FAILED(rv)) {
    ThrowException(env, rv, "Failure in initEmbedding");
    FreeJavaGlobals(env);
  }
}

extern "C" NS_EXPORT void JNICALL
GRE_NATIVE(termEmbedding) (JNIEnv *env, jobject)
{
  // Free globals before calling XRE_TermEmbedding(), since we need some
  // XPCOM services.
  FreeJavaGlobals(env);

#ifndef VBOX
  XRE_TermEmbedding();
#endif
}
#ifdef VBOX
nsresult
InitXPCOMVBox_Impl(JNIEnv* env, jobject aVBoxBinDirectory)
{
#if defined(VBOX_PATH_APP_PRIVATE_ARCH) && defined(VBOX_PATH_SHARED_LIBS)
    rv = RTR3InitDll(RTR3INIT_FLAGS_UNOBTRUSIVE);
#else
    const char *pszHome  = nsnull;
    const char *jhome = nsnull;
    jstring path = nsnull;

    int rv;
    jclass     clazz;
    jmethodID  getPathMID;

    if (aVBoxBinDirectory  &&
        (clazz = env->FindClass("java/io/File")) &&
        (getPathMID = env->GetMethodID(clazz, "getAbsolutePath",
                                       "()Ljava/lang/String;"))
        )
    {
        path = (jstring)env->CallObjectMethod(aVBoxBinDirectory, getPathMID);
        pszHome = jhome = env->GetStringUTFChars(path, nsnull);
    }

    if (pszHome == nsnull)
        pszHome = getenv("VBOX_PROGRAM_PATH");

    if (pszHome) {
      size_t cchHome = strlen(pszHome);
      char *pszExePath = (char *)alloca(cchHome + 32);
      memcpy(pszExePath, pszHome, cchHome);
      memcpy(pszExePath + cchHome, "/javafake", sizeof("/javafake"));
      rv = RTR3InitEx(RTR3INIT_VER_CUR, RTR3INIT_FLAGS_DLL | RTR3INIT_FLAGS_UNOBTRUSIVE, 0, NULL, pszExePath);
    } else {
      rv = RTR3InitDll(RTR3INIT_FLAGS_UNOBTRUSIVE);
    }

    if (jhome)
        env->ReleaseStringUTFChars(path, jhome);
#endif

    return com::Initialize();
}
#endif

nsresult
InitXPCOM_Impl(JNIEnv* env, jobject aMozBinDirectory,
               jobject aAppFileLocProvider, jobject* aResult)
{
  nsresult rv;
  // create an nsILocalFile from given java.io.File
  nsCOMPtr<nsILocalFile> directory;
  if (aMozBinDirectory) {
    rv = File_to_nsILocalFile(env, aMozBinDirectory, getter_AddRefs(directory));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // create nsAppFileLocProviderProxy from given Java object
  nsCOMPtr<nsIDirectoryServiceProvider> provider;
  if (aAppFileLocProvider) {
    rv = NS_NewAppFileLocProviderProxy(aAppFileLocProvider,
                                       getter_AddRefs(provider));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // init XPCOM
  nsCOMPtr<nsIServiceManager> servMan;
  rv = NS_InitXPCOM2(getter_AddRefs(servMan), directory, provider);
  NS_ENSURE_SUCCESS(rv, rv);

  // create Java proxy for service manager returned by NS_InitXPCOM2
  return NativeInterfaceToJavaObject(env, servMan, NS_GET_IID(nsIServiceManager),
                                     nsnull, aResult);
}

extern "C" NS_EXPORT jobject JNICALL
XPCOM_NATIVE(initXPCOM) (JNIEnv* env, jobject, jobject aMozBinDirectory,
                         jobject aAppFileLocProvider)
{
#ifdef VBOX
  nsresult rv = InitXPCOMVBox_Impl(env, aMozBinDirectory);
  if (NS_SUCCEEDED(rv))
    return nsnull;
#else
  jobject servMan;
  nsresult rv = InitXPCOM_Impl(env, aMozBinDirectory, aAppFileLocProvider,
                               &servMan);
  if (NS_SUCCEEDED(rv))
    return servMan;
#endif

  ThrowException(env, rv, "Failure in initXPCOM");
  FreeJavaGlobals(env);
  return nsnull;
}

extern "C" NS_EXPORT void JNICALL
#ifdef VBOX
XPCOM_NATIVE2(shutdownXPCOM) (JNIEnv *env, jobject, jobject aServMgr)
#else
XPCOM_NATIVE(shutdownXPCOM) (JNIEnv *env, jobject, jobject aServMgr)
#endif
{
#ifdef VBOX
  // Free globals before calling NS_ShutdownXPCOM(), since we need some
  // XPCOM services.
  //FreeJavaGlobals(env);
  //com::Shutdown();
#else
  nsresult rv;
  nsIServiceManager* servMgr = nsnull;
  if (aServMgr) {
    // Get native XPCOM instance
    nsISupports* instancePtr = nsnull;
    rv = JavaObjectToNativeInterface(env, aServMgr,
            NS_GET_IID(nsIServiceManager), (void**) &instancePtr);
    NS_ASSERTION(NS_SUCCEEDED(rv) && instancePtr != nsnull,
                 "Failed to get XPCOM obj for ServiceMgr.");
    if (NS_SUCCEEDED(rv)) {
      rv = instancePtr->QueryInterface(NS_GET_IID(nsIServiceManager),
                                       (void**) &servMgr);
      NS_ASSERTION(NS_SUCCEEDED(rv), "QI for nsIServiceManager failed");
    }

    // Even if we failed to get the matching xpcom object, we don't abort this
    // function.  Just call NS_ShutdownXPCOM with a null service manager.
  }

  // Free globals before calling NS_ShutdownXPCOM(), since we need some
  // XPCOM services.
  FreeJavaGlobals(env);

  rv = NS_ShutdownXPCOM(servMgr);
  if (NS_FAILED(rv))
    ThrowException(env, rv, "NS_ShutdownXPCOM failed");
#endif
}

extern "C" NS_EXPORT jobject JNICALL
XPCOM_NATIVE(newLocalFile) (JNIEnv *env, jobject, jstring aPath,
                            jboolean aFollowLinks)
{
  // Create a Mozilla string from the jstring
  const PRUnichar* buf = nsnull;
  if (aPath) {
    buf = env->GetStringChars(aPath, nsnull);
    if (!buf)
      return nsnull;  // exception already thrown
  }

  nsAutoString path_str(buf);
  env->ReleaseStringChars(aPath, buf);

  // Make call to given function
  nsCOMPtr<nsILocalFile> file;
  nsresult rv = NS_NewLocalFile(path_str, aFollowLinks, getter_AddRefs(file));

  if (NS_SUCCEEDED(rv)) {
    jobject javaProxy;
    rv = NativeInterfaceToJavaObject(env, file, NS_GET_IID(nsILocalFile),
                                     nsnull, &javaProxy);
    if (NS_SUCCEEDED(rv))
      return javaProxy;
  }

  ThrowException(env, rv, "Failure in newLocalFile");
  return nsnull;
}

extern "C" NS_EXPORT jobject JNICALL
#ifdef VBOX
XPCOM_NATIVE2(getComponentManager) (JNIEnv *env, jobject)
#else
XPCOM_NATIVE(getComponentManager) (JNIEnv *env, jobject)
#endif
{
  // Call XPCOM method
  nsCOMPtr<nsIComponentManager> cm;
  nsresult rv = NS_GetComponentManager(getter_AddRefs(cm));

  if (NS_SUCCEEDED(rv)) {
    jobject javaProxy;
    rv = NativeInterfaceToJavaObject(env, cm, NS_GET_IID(nsIComponentManager),
                                     nsnull, &javaProxy);
    if (NS_SUCCEEDED(rv))
      return javaProxy;
  }

  ThrowException(env, rv, "Failure in getComponentManager");
  return nsnull;
}

extern "C" NS_EXPORT jobject JNICALL
XPCOM_NATIVE(getComponentRegistrar) (JNIEnv *env, jobject)
{
  // Call XPCOM method
  nsCOMPtr<nsIComponentRegistrar> cr;
  nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(cr));

  if (NS_SUCCEEDED(rv)) {
    jobject javaProxy;
    rv = NativeInterfaceToJavaObject(env, cr, NS_GET_IID(nsIComponentRegistrar),
                                     nsnull, &javaProxy);
    if (NS_SUCCEEDED(rv))
      return javaProxy;
  }

  ThrowException(env, rv, "Failure in getComponentRegistrar");
  return nsnull;
}

#ifdef VBOX
#  include <VBox/com/NativeEventQueue.h>
#  include <iprt/err.h>

extern "C" NS_EXPORT jint JNICALL
XPCOM_NATIVE2(waitForEvents) (JNIEnv *env, jobject, jlong aTimeout)
{
    com::NativeEventQueue* aEventQ = com::NativeEventQueue::getMainEventQueue();
    NS_WARN_IF_FALSE(aEventQ != nsnull, "Null main event queue");
    if (!aEventQ)
        return -1;

    int rc = aEventQ->processEventQueue(aTimeout < 0 ? RT_INDEFINITE_WAIT : (uint32_t)aTimeout);

    if (RT_SUCCESS(rc))
        return 0;

    if (   rc == VERR_TIMEOUT
        || rc == VERR_INTERRUPTED)
        return 1;

    return 2;
}
#endif

extern "C" NS_EXPORT jobject JNICALL
#ifdef VBOX
XPCOM_NATIVE2(getServiceManager) (JNIEnv *env, jobject)
#else
XPCOM_NATIVE(getServiceManager) (JNIEnv *env, jobject)
#endif
{
  // Call XPCOM method
  nsCOMPtr<nsIServiceManager> sm;
  nsresult rv = NS_GetServiceManager(getter_AddRefs(sm));

  if (NS_SUCCEEDED(rv)) {
    jobject javaProxy;
    rv = NativeInterfaceToJavaObject(env, sm, NS_GET_IID(nsIServiceManager),
                                     nsnull, &javaProxy);
    if (NS_SUCCEEDED(rv))
      return javaProxy;
  }

  ThrowException(env, rv, "Failure in getServiceManager");
  return nsnull;
}

extern "C" NS_EXPORT jobject JNICALL
GRE_NATIVE(lockProfileDirectory) (JNIEnv* env, jobject, jobject aDirectory)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (aDirectory) {
    nsCOMPtr<nsILocalFile> profileDir;
    rv = File_to_nsILocalFile(env, aDirectory, getter_AddRefs(profileDir));

    if (NS_SUCCEEDED(rv)) {
      nsISupports* lock;
#ifdef VBOX
      rv = 0;
      lock = 0;
#else
      rv = XRE_LockProfileDirectory(profileDir, &lock);
#endif

      if (NS_SUCCEEDED(rv)) {
        jclass clazz =
            env->FindClass("org/mozilla/xpcom/ProfileLock");
        if (clazz) {
          jmethodID mid = env->GetMethodID(clazz, "<init>", "(J)V");
          if (mid) {
            return env->NewObject(clazz, mid, reinterpret_cast<jlong>(lock));
          }
        }

        // if we get here, then something failed
        rv = NS_ERROR_FAILURE;
      }
    }
  }

  ThrowException(env, rv, "Failure in lockProfileDirectory");
  return nsnull;
}

extern "C" NS_EXPORT void JNICALL
GRE_NATIVE(notifyProfile) (JNIEnv *env, jobject)
{
#ifndef VBOX
  XRE_NotifyProfile();
#endif
}

#ifdef XP_MACOSX
extern PRUint64 GetPlatformHandle(JAWT_DrawingSurfaceInfo* dsi);
#endif

extern "C" NS_EXPORT jlong JNICALL
MOZILLA_NATIVE(getNativeHandleFromAWT) (JNIEnv* env, jobject clazz,
                                        jobject widget)
{
  PRUint64 handle = 0;

#if defined(XP_MACOSX) && !defined(VBOX)
  JAWT awt;
  awt.version = JAWT_VERSION_1_4;
  jboolean result = JAWT_GetAWT(env, &awt);
  if (result == JNI_FALSE)
    return 0;

  JAWT_DrawingSurface* ds = awt.GetDrawingSurface(env, widget);
  if (ds != nsnull) {
    jint lock = ds->Lock(ds);
    if (!(lock & JAWT_LOCK_ERROR)) {
      JAWT_DrawingSurfaceInfo* dsi = ds->GetDrawingSurfaceInfo(ds);
      if (dsi) {
        handle = GetPlatformHandle(dsi);
        ds->FreeDrawingSurfaceInfo(dsi);
      }

      ds->Unlock(ds);
    }

    awt.FreeDrawingSurface(ds);
  }
#else
  NS_WARNING("getNativeHandleFromAWT JNI method not implemented");
#endif

  return handle;
}

extern "C" NS_EXPORT jlong JNICALL
JXUTILS_NATIVE(wrapJavaObject) (JNIEnv* env, jobject, jobject aJavaObject,
                                jstring aIID)
{
  nsresult rv;
  void* xpcomObject = nsnull;

  if (!aJavaObject || !aIID) {
    rv = NS_ERROR_NULL_POINTER;
  } else {
    const char* str = env->GetStringUTFChars(aIID, nsnull);
    if (!str) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
      nsID iid;
      if (iid.Parse(str)) {
        rv = JavaObjectToNativeInterface(env, aJavaObject, iid, &xpcomObject);
        if (NS_SUCCEEDED(rv)) {
          nsISupports *xpcom_nat_obj = (nsISupports*) xpcomObject;
          rv = xpcom_nat_obj->QueryInterface(iid, &xpcomObject);
          NS_IF_RELEASE(xpcom_nat_obj);
        }
      } else {
        rv = NS_ERROR_INVALID_ARG;
      }

      env->ReleaseStringUTFChars(aIID, str);
    }
  }

  if (NS_FAILED(rv)) {
    ThrowException(env, rv, "Failed to create XPCOM proxy for Java object");
  }
  return reinterpret_cast<jlong>(xpcomObject);
}

extern "C" NS_EXPORT jobject JNICALL
JXUTILS_NATIVE(wrapXPCOMObject) (JNIEnv* env, jobject, jlong aXPCOMObject,
                                 jstring aIID)
{
  nsresult rv;
  jobject javaObject = nsnull;
  nsISupports* xpcomObject = reinterpret_cast<nsISupports*>(aXPCOMObject);

  if (!xpcomObject || !aIID) {
    rv = NS_ERROR_NULL_POINTER;
  } else {
    const char* str = env->GetStringUTFChars(aIID, nsnull);
    if (!str) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
      nsID iid;
      if (iid.Parse(str)) {
        // XXX Should we be passing something other than NULL for aObjectLoader?
        rv = NativeInterfaceToJavaObject(env, xpcomObject, iid, nsnull,
                                         &javaObject);
      } else {
        rv = NS_ERROR_INVALID_ARG;
      }

      env->ReleaseStringUTFChars(aIID, str);
    }
  }

  if (NS_FAILED(rv)) {
    ThrowException(env, rv, "Failed to create XPCOM proxy for Java object");
  }
  return javaObject;
}
