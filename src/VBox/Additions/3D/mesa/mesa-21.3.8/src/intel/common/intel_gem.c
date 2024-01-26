/*
 * Copyright © 2020 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "intel_gem.h"
#include "drm-uapi/i915_drm.h"

bool
intel_gem_supports_syncobj_wait(int fd)
{
   int ret;

   struct drm_syncobj_create create = {
      .flags = 0,
   };
   ret = intel_ioctl(fd, DRM_IOCTL_SYNCOBJ_CREATE, &create);
   if (ret)
      return false;

   uint32_t syncobj = create.handle;

   struct drm_syncobj_wait wait = {
      .handles = (uint64_t)(uintptr_t)&create,
      .count_handles = 1,
      .timeout_nsec = 0,
      .flags = DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT,
   };
   ret = intel_ioctl(fd, DRM_IOCTL_SYNCOBJ_WAIT, &wait);

   struct drm_syncobj_destroy destroy = {
      .handle = syncobj,
   };
   intel_ioctl(fd, DRM_IOCTL_SYNCOBJ_DESTROY, &destroy);

   /* If it timed out, then we have the ioctl and it supports the
    * DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT flag.
    */
   return ret == -1 && errno == ETIME;
}
