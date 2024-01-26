/**************************************************************************
 *
 * Copyright 2009-2010 Chia-I Wu <olvaffe@gmail.com>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "c99_compat.h"
#include "c11/threads.h"
#include "util/u_thread.h"
#include "util/u_string.h"

#include "egllog.h"
#include "eglcurrent.h"
#include "eglglobals.h"

/* a fallback thread info to guarantee that every thread always has one */
static _EGLThreadInfo dummy_thread;
static mtx_t _egl_TSDMutex = _MTX_INITIALIZER_NP;
static EGLBoolean _egl_TSDInitialized;
static tss_t _egl_TSD;
static void _eglDestroyThreadInfo(_EGLThreadInfo *t);

#ifdef USE_ELF_TLS
static __THREAD_INITIAL_EXEC const _EGLThreadInfo *_egl_TLS;
#endif

static inline void _eglSetTSD(const _EGLThreadInfo *t)
{
   tss_set(_egl_TSD, (void *) t);
#ifdef USE_ELF_TLS
   _egl_TLS = t;
#endif
}

static inline _EGLThreadInfo *_eglGetTSD(void)
{
#ifdef USE_ELF_TLS
   return (_EGLThreadInfo *) _egl_TLS;
#else
   return (_EGLThreadInfo *) tss_get(_egl_TSD);
#endif
}

static inline void _eglFiniTSD(void)
{
   mtx_lock(&_egl_TSDMutex);
   if (_egl_TSDInitialized) {
      _EGLThreadInfo *t = _eglGetTSD();

      _egl_TSDInitialized = EGL_FALSE;
      _eglDestroyThreadInfo(t);
      tss_delete(_egl_TSD);
   }
   mtx_unlock(&_egl_TSDMutex);
}

static inline EGLBoolean _eglInitTSD()
{
   if (!_egl_TSDInitialized) {
      mtx_lock(&_egl_TSDMutex);

      /* check again after acquiring lock */
      if (!_egl_TSDInitialized) {
         if (tss_create(&_egl_TSD, (void (*)(void *)) _eglDestroyThreadInfo) != thrd_success) {
            mtx_unlock(&_egl_TSDMutex);
            return EGL_FALSE;
         }
         _eglAddAtExitCall(_eglFiniTSD);
         _egl_TSDInitialized = EGL_TRUE;
      }

      mtx_unlock(&_egl_TSDMutex);
   }

   return EGL_TRUE;
}

static void
_eglInitThreadInfo(_EGLThreadInfo *t)
{
   t->LastError = EGL_SUCCESS;
   /* default, per EGL spec */
   t->CurrentAPI = EGL_OPENGL_ES_API;
}


/**
 * Allocate and init a new _EGLThreadInfo object.
 */
static _EGLThreadInfo *
_eglCreateThreadInfo(void)
{
   _EGLThreadInfo *t = calloc(1, sizeof(_EGLThreadInfo));
   if (!t)
      t = &dummy_thread;

   _eglInitThreadInfo(t);
   return t;
}


/**
 * Delete/free a _EGLThreadInfo object.
 */
static void
_eglDestroyThreadInfo(_EGLThreadInfo *t)
{
   if (t != &dummy_thread) {
      free(t);
#ifdef USE_ELF_TLS
      /* Reset the TLS also here, otherwise
       * it will be having a dangling pointer */
      _egl_TLS = NULL;
#endif
   }
}


/**
 * Make sure TSD is initialized and return current value.
 */
static inline _EGLThreadInfo *
_eglCheckedGetTSD(void)
{
   if (_eglInitTSD() != EGL_TRUE) {
      _eglLog(_EGL_FATAL, "failed to initialize \"current\" system");
      return NULL;
   }

   return _eglGetTSD();
}


/**
 * Return the calling thread's thread info.
 * If the calling thread nevers calls this function before, or if its thread
 * info was destroyed, a new one is created.  This function never returns NULL.
 * In the case allocation fails, a dummy one is returned.  See also
 * _eglIsCurrentThreadDummy.
 */
_EGLThreadInfo *
_eglGetCurrentThread(void)
{
   _EGLThreadInfo *t = _eglCheckedGetTSD();
   if (!t) {
      t = _eglCreateThreadInfo();
      _eglSetTSD(t);
   }

   return t;
}


/**
 * Destroy the calling thread's thread info.
 */
void
_eglDestroyCurrentThread(void)
{
   _EGLThreadInfo *t = _eglCheckedGetTSD();
   if (t) {
      _eglDestroyThreadInfo(t);
      _eglSetTSD(NULL);
   }
}


/**
 * Return true if the calling thread's thread info is dummy.
 * A dummy thread info is shared by all threads and should not be modified.
 * Functions like eglBindAPI or eglMakeCurrent should check for dummy-ness
 * before updating the thread info.
 */
EGLBoolean
_eglIsCurrentThreadDummy(void)
{
   _EGLThreadInfo *t = _eglCheckedGetTSD();
   return (!t || t == &dummy_thread);
}


/**
 * Return the currently bound context of the current API, or NULL.
 */
_EGLContext *
_eglGetCurrentContext(void)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   return t->CurrentContext;
}


/**
 * Record EGL error code and return EGL_FALSE.
 */
static EGLBoolean
_eglInternalError(EGLint errCode, const char *msg)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();

   if (t == &dummy_thread)
      return EGL_FALSE;

   t->LastError = errCode;

   if (errCode != EGL_SUCCESS) {
      const char *s;

      switch (errCode) {
      case EGL_BAD_ACCESS:
         s = "EGL_BAD_ACCESS";
         break;
      case EGL_BAD_ALLOC:
         s = "EGL_BAD_ALLOC";
         break;
      case EGL_BAD_ATTRIBUTE:
         s = "EGL_BAD_ATTRIBUTE";
         break;
      case EGL_BAD_CONFIG:
         s = "EGL_BAD_CONFIG";
         break;
      case EGL_BAD_CONTEXT:
         s = "EGL_BAD_CONTEXT";
         break;
      case EGL_BAD_CURRENT_SURFACE:
         s = "EGL_BAD_CURRENT_SURFACE";
         break;
      case EGL_BAD_DISPLAY:
         s = "EGL_BAD_DISPLAY";
         break;
      case EGL_BAD_MATCH:
         s = "EGL_BAD_MATCH";
         break;
      case EGL_BAD_NATIVE_PIXMAP:
         s = "EGL_BAD_NATIVE_PIXMAP";
         break;
      case EGL_BAD_NATIVE_WINDOW:
         s = "EGL_BAD_NATIVE_WINDOW";
         break;
      case EGL_BAD_PARAMETER:
         s = "EGL_BAD_PARAMETER";
         break;
      case EGL_BAD_SURFACE:
         s = "EGL_BAD_SURFACE";
         break;
      case EGL_NOT_INITIALIZED:
         s = "EGL_NOT_INITIALIZED";
         break;
      default:
         s = "other EGL error";
      }
      _eglLog(_EGL_DEBUG, "EGL user error 0x%x (%s) in %s\n", errCode, s, msg);
   }

   return EGL_FALSE;
}

EGLBoolean
_eglError(EGLint errCode, const char *msg)
{
   if (errCode != EGL_SUCCESS) {
      EGLint type;
      if (errCode == EGL_BAD_ALLOC)
         type = EGL_DEBUG_MSG_CRITICAL_KHR;
      else
         type = EGL_DEBUG_MSG_ERROR_KHR;

      _eglDebugReport(errCode, NULL, type, msg);
   } else
      _eglInternalError(errCode, msg);

   return EGL_FALSE;
}

void
_eglDebugReport(EGLenum error, const char *funcName,
      EGLint type, const char *message, ...)
{
   _EGLThreadInfo *thr = _eglGetCurrentThread();
   EGLDEBUGPROCKHR callback = NULL;
   va_list args;

   if (funcName == NULL)
      funcName = thr->CurrentFuncName;

   mtx_lock(_eglGlobal.Mutex);
   if (_eglGlobal.debugTypesEnabled & DebugBitFromType(type))
      callback = _eglGlobal.debugCallback;

   mtx_unlock(_eglGlobal.Mutex);

   char *message_buf = NULL;
   if (message != NULL) {
      va_start(args, message);
      if (vasprintf(&message_buf, message, args) < 0)
         message_buf = NULL;
      va_end(args);
   }

   if (callback != NULL) {
      callback(error, funcName, type, thr->Label, thr->CurrentObjectLabel,
               message_buf);
   }

   if (type == EGL_DEBUG_MSG_CRITICAL_KHR || type == EGL_DEBUG_MSG_ERROR_KHR) {
      char *func_message_buf = NULL;
      /* Note: _eglError() is often called with msg == thr->currentFuncName */
      if (message_buf && funcName && strcmp(message_buf, funcName) != 0) {
         if (asprintf(&func_message_buf, "%s: %s", funcName, message_buf) < 0)
            func_message_buf = NULL;
      }
      _eglInternalError(error, func_message_buf ? func_message_buf : funcName);
      free(func_message_buf);
   }
   free(message_buf);
}
