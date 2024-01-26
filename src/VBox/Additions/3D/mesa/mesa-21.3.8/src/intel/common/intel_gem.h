/*
 * Copyright © 2018 Intel Corporation
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

#ifndef INTEL_GEM_H
#define INTEL_GEM_H

#include "drm-uapi/i915_drm.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

static inline uint64_t
intel_canonical_address(uint64_t v)
{
   /* From the Broadwell PRM Vol. 2a, MI_LOAD_REGISTER_MEM::MemoryAddress:
    *
    *    "This field specifies the address of the memory location where the
    *    register value specified in the DWord above will read from. The
    *    address specifies the DWord location of the data. Range =
    *    GraphicsVirtualAddress[63:2] for a DWord register GraphicsAddress
    *    [63:48] are ignored by the HW and assumed to be in correct
    *    canonical form [63:48] == [47]."
    */
   const int shift = 63 - 47;
   return (int64_t)(v << shift) >> shift;
}

/**
 * This returns a 48-bit address with the high 16 bits zeroed.
 *
 * It's the opposite of intel_canonicalize_address.
 */
static inline uint64_t
intel_48b_address(uint64_t v)
{
   const int shift = 63 - 47;
   return (uint64_t)(v << shift) >> shift;
}

/**
 * Call ioctl, restarting if it is interupted
 */
static inline int
intel_ioctl(int fd, unsigned long request, void *arg)
{
    int ret;

    do {
        ret = ioctl(fd, request, arg);
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));
    return ret;
}

/**
 * A wrapper around DRM_IOCTL_I915_QUERY
 *
 * Unfortunately, the error semantics of this ioctl are rather annoying so
 * it's better to have a common helper.
 */
static inline int
intel_i915_query_flags(int fd, uint64_t query_id, uint32_t flags,
                       void *buffer, int32_t *buffer_len)
{
   struct drm_i915_query_item item = {
      .query_id = query_id,
      .length = *buffer_len,
      .flags = flags,
      .data_ptr = (uintptr_t)buffer,
   };

   struct drm_i915_query args = {
      .num_items = 1,
      .flags = 0,
      .items_ptr = (uintptr_t)&item,
   };

   int ret = intel_ioctl(fd, DRM_IOCTL_I915_QUERY, &args);
   if (ret != 0)
      return -errno;
   else if (item.length < 0)
      return item.length;

   *buffer_len = item.length;
   return 0;
}

static inline int
intel_i915_query(int fd, uint64_t query_id, void *buffer,
                 int32_t *buffer_len)
{
   return intel_i915_query_flags(fd, query_id, 0, buffer, buffer_len);
}

/**
 * Query for the given data, allocating as needed
 *
 * The caller is responsible for freeing the returned pointer.
 */
static inline void *
intel_i915_query_alloc(int fd, uint64_t query_id)
{
   int32_t length = 0;
   int ret = intel_i915_query(fd, query_id, NULL, &length);
   if (ret < 0)
      return NULL;

   void *data = calloc(1, length);
   assert(data != NULL); /* This shouldn't happen in practice */
   if (data == NULL)
      return NULL;

   ret = intel_i915_query(fd, query_id, data, &length);
   assert(ret == 0); /* We should have caught the error above */
   if (ret < 0) {
      free(data);
      return NULL;
   }

   return data;
}

bool intel_gem_supports_syncobj_wait(int fd);

#endif /* INTEL_GEM_H */
