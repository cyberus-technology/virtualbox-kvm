/*
 * Copyright © 2008 Intel Corporation
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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

/**
 * \file
 * \brief Support for GL_ARB_sync and EGL_KHR_fence_sync.
 *
 * GL_ARB_sync is implemented by flushing the current batchbuffer and keeping a
 * reference on it.  We can then check for completion or wait for completion
 * using the normal buffer object mechanisms.  This does mean that if an
 * application is using many sync objects, it will emit small batchbuffers
 * which may end up being a significant overhead.  In other tests of removing
 * gratuitous batchbuffer syncs in Mesa, it hasn't appeared to be a significant
 * performance bottleneck, though.
 */

#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"

struct intel_fence {
   struct intel_context *intel;
   /** The fence waits for completion of this batch. */
   drm_intel_bo *batch_bo;

   mtx_t mutex;
   bool signalled;
};

struct intel_gl_sync_object {
   struct gl_sync_object Base;
   struct intel_fence fence;
};

static void
intel_fence_finish(struct intel_fence *fence)
{
   if (fence->batch_bo)
      drm_intel_bo_unreference(fence->batch_bo);
}

static void
intel_fence_insert(struct intel_context *intel, struct intel_fence *fence)
{
   assert(!fence->batch_bo);
   assert(!fence->signalled);

   intel_batchbuffer_emit_mi_flush(intel);
   fence->batch_bo = intel->batch.bo;
   drm_intel_bo_reference(fence->batch_bo);
   intel_batchbuffer_flush(intel);
}

static bool
intel_fence_has_completed_locked(struct intel_fence *fence)
{
   if (fence->signalled)
      return true;

   if (fence->batch_bo && !drm_intel_bo_busy(fence->batch_bo)) {
      drm_intel_bo_unreference(fence->batch_bo);
      fence->batch_bo = NULL;
      fence->signalled = true;
      return true;
   }

   return false;
}

static bool
intel_fence_has_completed(struct intel_fence *fence)
{
   bool ret;

   mtx_lock(&fence->mutex);
   ret = intel_fence_has_completed_locked(fence);
   mtx_unlock(&fence->mutex);

   return ret;
}

static bool
intel_fence_client_wait_locked(struct intel_context *intel, struct intel_fence *fence,
                             uint64_t timeout)
{
   if (fence->signalled)
      return true;

   assert(fence->batch_bo);

   /* DRM_IOCTL_I915_GEM_WAIT uses a signed 64 bit timeout and returns
    * immediately for timeouts <= 0.  The best we can do is to clamp the
    * timeout to INT64_MAX.  This limits the maximum timeout from 584 years to
    * 292 years - likely not a big deal.
    */
   if (timeout > INT64_MAX)
      timeout = INT64_MAX;

   if (drm_intel_gem_bo_wait(fence->batch_bo, timeout) != 0)
      return false;

   fence->signalled = true;
   drm_intel_bo_unreference(fence->batch_bo);
   fence->batch_bo = NULL;

   return true;
}

/**
 * Return true if the function successfully signals or has already signalled.
 * (This matches the behavior expected from __DRI2fence::client_wait_sync).
 */
static bool
intel_fence_client_wait(struct intel_context *intel, struct intel_fence *fence,
                      uint64_t timeout)
{
   bool ret;

   mtx_lock(&fence->mutex);
   ret = intel_fence_client_wait_locked(intel, fence, timeout);
   mtx_unlock(&fence->mutex);

   return ret;
}

static void
intel_fence_server_wait(struct intel_context *intel, struct intel_fence *fence)
{
   /* We have nothing to do for WaitSync.  Our GL command stream is sequential,
    * so given that the sync object has already flushed the batchbuffer, any
    * batchbuffers coming after this waitsync will naturally not occur until
    * the previous one is done.
    */
}

static struct gl_sync_object *
intel_gl_new_sync_object(struct gl_context *ctx)
{
   struct intel_gl_sync_object *sync;

   sync = calloc(1, sizeof(*sync));
   if (!sync)
      return NULL;

   return &sync->Base;
}

static void
intel_gl_delete_sync_object(struct gl_context *ctx, struct gl_sync_object *s)
{
   struct intel_gl_sync_object *sync = (struct intel_gl_sync_object *)s;

   intel_fence_finish(&sync->fence);
   free(sync);
}

static void
intel_gl_fence_sync(struct gl_context *ctx, struct gl_sync_object *s,
	       GLenum condition, GLbitfield flags)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_gl_sync_object *sync = (struct intel_gl_sync_object *)s;

   intel_fence_insert(intel, &sync->fence);
}

static void
intel_gl_client_wait_sync(struct gl_context *ctx, struct gl_sync_object *s,
				 GLbitfield flags, GLuint64 timeout)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_gl_sync_object *sync = (struct intel_gl_sync_object *)s;

   if (intel_fence_client_wait(intel, &sync->fence, timeout))
      s->StatusFlag = 1;
}

static void
intel_gl_server_wait_sync(struct gl_context *ctx, struct gl_sync_object *s,
				 GLbitfield flags, GLuint64 timeout)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_gl_sync_object *sync = (struct intel_gl_sync_object *)s;

   intel_fence_server_wait(intel, &sync->fence);
}

static void
intel_gl_check_sync(struct gl_context *ctx, struct gl_sync_object *s)
{
   struct intel_gl_sync_object *sync = (struct intel_gl_sync_object *)s;

   if (intel_fence_has_completed(&sync->fence))
      s->StatusFlag = 1;
}

void
intel_init_syncobj_functions(struct dd_function_table *functions)
{
   functions->NewSyncObject = intel_gl_new_sync_object;
   functions->DeleteSyncObject = intel_gl_delete_sync_object;
   functions->FenceSync = intel_gl_fence_sync;
   functions->CheckSync = intel_gl_check_sync;
   functions->ClientWaitSync = intel_gl_client_wait_sync;
   functions->ServerWaitSync = intel_gl_server_wait_sync;
}

static void *
intel_dri_create_fence(__DRIcontext *ctx)
{
   struct intel_context *intel = ctx->driverPrivate;
   struct intel_fence *fence;

   fence = calloc(1, sizeof(*fence));
   if (!fence)
      return NULL;

   mtx_init(&fence->mutex, mtx_plain);
   fence->intel = intel;
   intel_fence_insert(intel, fence);

   return fence;
}

static void
intel_dri_destroy_fence(__DRIscreen *screen, void *driver_fence)
{
   struct intel_fence *fence = driver_fence;

   intel_fence_finish(fence);
   free(fence);
}

static GLboolean
intel_dri_client_wait_sync(__DRIcontext *ctx, void *driver_fence, unsigned flags,
                           uint64_t timeout)
{
   struct intel_fence *fence = driver_fence;

   return intel_fence_client_wait(fence->intel, fence, timeout);
}

static void
intel_dri_server_wait_sync(__DRIcontext *ctx, void *driver_fence, unsigned flags)
{
   struct intel_fence *fence = driver_fence;

   /* We might be called here with a NULL fence as a result of WaitSyncKHR
    * on a EGL_KHR_reusable_sync fence. Nothing to do here in such case.
    */
   if (!fence)
      return;

   intel_fence_server_wait(fence->intel, fence);
}

const __DRI2fenceExtension intelFenceExtension = {
   .base = { __DRI2_FENCE, 1 },

   .create_fence = intel_dri_create_fence,
   .destroy_fence = intel_dri_destroy_fence,
   .client_wait_sync = intel_dri_client_wait_sync,
   .server_wait_sync = intel_dri_server_wait_sync,
   .get_fence_from_cl_event = NULL,
};
