/*
 * Copyright © 2012 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "main/context.h"

#include <xf86drm.h>
#include "brw_context.h"

/**
 * Query information about GPU resets observed by this context
 *
 * Called via \c dd_function_table::GetGraphicsResetStatus.
 */
GLenum
brw_get_graphics_reset_status(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);
   struct drm_i915_reset_stats stats = { .ctx_id = brw->hw_ctx };

   /* If hardware contexts are not being used (or
    * DRM_IOCTL_I915_GET_RESET_STATS is not supported), this function should
    * not be accessible.
    */
   assert(brw->hw_ctx != 0);

   /* A reset status other than NO_ERROR was returned last time. I915 returns
    * nonzero active/pending only if reset has been encountered and completed.
    * Return NO_ERROR from now on.
    */
   if (brw->reset_count != 0)
      return GL_NO_ERROR;

   if (drmIoctl(brw->screen->fd, DRM_IOCTL_I915_GET_RESET_STATS, &stats) != 0)
      return GL_NO_ERROR;

   /* A reset was observed while a batch from this context was executing.
    * Assume that this context was at fault.
    */
   if (stats.batch_active != 0) {
      brw->reset_count = stats.reset_count;
      return GL_GUILTY_CONTEXT_RESET_ARB;
   }

   /* A reset was observed while a batch from this context was in progress,
    * but the batch was not executing.  In this case, assume that the context
    * was not at fault.
    */
   if (stats.batch_pending != 0) {
      brw->reset_count = stats.reset_count;
      return GL_INNOCENT_CONTEXT_RESET_ARB;
   }

   return GL_NO_ERROR;
}

void
brw_check_for_reset(struct brw_context *brw)
{
   struct drm_i915_reset_stats stats = { .ctx_id = brw->hw_ctx };

   if (drmIoctl(brw->screen->fd, DRM_IOCTL_I915_GET_RESET_STATS, &stats) != 0)
      return;

   if (stats.batch_active > 0 || stats.batch_pending > 0)
      _mesa_set_context_lost_dispatch(&brw->ctx);
}
