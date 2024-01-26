/*
 * Copyright © 2021, Google Inc.
 * Copyright (C) 2021, GlobalLogic Ukraine
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef EGL_ANDROID_INCLUDED
#define EGL_ANDROID_INCLUDED

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <GL/internal/dri_interface.h>

#include "egl_dri2.h"

#if ANDROID_API_LEVEL < 26
/* Shim layer to map ANativeWindow_* onto the legacy system internal APIs */
enum ANativeWindowQuery {
   ANATIVEWINDOW_QUERY_MIN_UNDEQUEUED_BUFFERS = 3,
   ANATIVEWINDOW_QUERY_DEFAULT_WIDTH = 6,
   ANATIVEWINDOW_QUERY_DEFAULT_HEIGHT = 7,
};

static inline void
ANativeWindow_acquire(struct ANativeWindow *window)
{
   window->common.incRef(&window->common);
}

static inline void
ANativeWindow_release(struct ANativeWindow *window)
{
   window->common.decRef(&window->common);
}

static inline int32_t
ANativeWindow_getFormat(struct ANativeWindow *window)
{
   int32_t format = 0;
   int res = window->query(window, NATIVE_WINDOW_FORMAT, &format);
   return res < 0 ? res : format;
}

static inline int
ANativeWindow_dequeueBuffer(struct ANativeWindow *window,
                            struct ANativeWindowBuffer **buffer,
                            int *fenceFd)
{
   return window->dequeueBuffer(window, buffer, fenceFd);
}

static inline int
ANativeWindow_queueBuffer(struct ANativeWindow *window,
                          struct ANativeWindowBuffer *buffer,
                          int fenceFd)
{
   return window->queueBuffer(window, buffer, fenceFd);
}

static inline int
ANativeWindow_cancelBuffer(struct ANativeWindow *window,
                           struct ANativeWindowBuffer *buffer,
                           int fenceFd)
{
   return window->cancelBuffer(window, buffer, fenceFd);
}

static inline int
ANativeWindow_setUsage(struct ANativeWindow *window, uint64_t usage)
{
   return native_window_set_usage(window, usage);
}

static inline int
ANativeWindow_setSharedBufferMode(struct ANativeWindow *window,
                                  bool sharedBufferMode)
{
   return native_window_set_shared_buffer_mode(window, sharedBufferMode);
}

static inline int
ANativeWindow_setSwapInterval(struct ANativeWindow *window, int interval)
{
   return window->setSwapInterval(window, interval);
}

static inline int
ANativeWindow_query(const struct ANativeWindow *window,
                    enum ANativeWindowQuery what,
                    int *value)
{
   switch (what) {
   case ANATIVEWINDOW_QUERY_MIN_UNDEQUEUED_BUFFERS:
   case ANATIVEWINDOW_QUERY_DEFAULT_WIDTH:
   case ANATIVEWINDOW_QUERY_DEFAULT_HEIGHT:
      break;
   default:
      return -EINVAL;
   }
   return window->query(window, (int)what, value);
}
#endif // ANDROID_API_LEVEL < 26

struct buffer_info {
   int width;
   int height;
   uint32_t drm_fourcc;
   int num_planes;
   int fds[4];
   uint64_t modifier;
   int offsets[4];
   int pitches[4];
   enum __DRIYUVColorSpace yuv_color_space;
   enum __DRISampleRange sample_range;
   enum __DRIChromaSiting horizontal_siting;
   enum __DRIChromaSiting vertical_siting;
};

#ifdef USE_IMAPPER4_METADATA_API
#ifdef __cplusplus
extern "C" {
#endif
extern int
mapper_metadata_get_buffer_info(struct ANativeWindowBuffer *buf,
                                struct buffer_info *out_buf_info);
#ifdef __cplusplus
}
#endif
#else
static inline int
mapper_metadata_get_buffer_info(struct ANativeWindowBuffer *buf,
                                struct buffer_info *out_buf_info) {
   return -ENOTSUP;
}
#endif /* USE_IMAPPER4_METADATA_API */

#endif /* EGL_ANDROID_INCLUDED */
