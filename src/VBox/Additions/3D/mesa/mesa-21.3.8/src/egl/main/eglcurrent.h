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


#ifndef EGLCURRENT_INCLUDED
#define EGLCURRENT_INCLUDED

#include "c99_compat.h"

#include "egltypedefs.h"


#ifdef __cplusplus
extern "C" {
#endif

#define _EGL_API_ALL_BITS \
   (EGL_OPENGL_ES_BIT   | \
    EGL_OPENVG_BIT      | \
    EGL_OPENGL_ES2_BIT  | \
    EGL_OPENGL_ES3_BIT_KHR | \
    EGL_OPENGL_BIT)


/**
 * Per-thread info
 */
struct _egl_thread_info
{
   EGLint LastError;
   _EGLContext *CurrentContext;
   EGLenum CurrentAPI;
   EGLLabelKHR Label;

   /**
    * The name of the EGL function that's being called at the moment. This is
    * used to report the function name to the EGL_KHR_debug callback.
    */
   const char *CurrentFuncName;
   EGLLabelKHR CurrentObjectLabel;
};


/**
 * Return true if a client API enum is recognized.
 */
static inline EGLBoolean
_eglIsApiValid(EGLenum api)
{
#ifdef ANDROID
   /* OpenGL is not a valid/supported API on Android */
   return api == EGL_OPENGL_ES_API;
#else
   return (api == EGL_OPENGL_ES_API || api == EGL_OPENGL_API);
#endif
}


extern _EGLThreadInfo *
_eglGetCurrentThread(void);


extern void
_eglDestroyCurrentThread(void);


extern EGLBoolean
_eglIsCurrentThreadDummy(void);


extern _EGLContext *
_eglGetCurrentContext(void);


extern EGLBoolean
_eglError(EGLint errCode, const char *msg);

extern void
_eglDebugReport(EGLenum error, const char *funcName,
      EGLint type, const char *message, ...);

#define _eglReportCritical(error, funcName, ...) \
    _eglDebugReport(error, funcName, EGL_DEBUG_MSG_CRITICAL_KHR, __VA_ARGS__)

#define _eglReportError(error, funcName, ...) \
    _eglDebugReport(error, funcName, EGL_DEBUG_MSG_ERROR_KHR, __VA_ARGS__)

#define _eglReportWarn(funcName, ...) \
    _eglDebugReport(EGL_SUCCESS, funcName, EGL_DEBUG_MSG_WARN_KHR, __VA_ARGS__)

#define _eglReportInfo(funcName, ...) \
    _eglDebugReport(EGL_SUCCESS, funcName, EGL_DEBUG_MSG_INFO_KHR, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* EGLCURRENT_INCLUDED */
