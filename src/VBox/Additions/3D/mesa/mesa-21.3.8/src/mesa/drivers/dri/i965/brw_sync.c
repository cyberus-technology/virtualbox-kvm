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

#include <libsync.h> /* Requires Android or libdrm-2.4.72 */

#include "util/os_file.h"
#include "util/u_memory.h"
#include <xf86drm.h>

#include "brw_context.h"
#include "brw_batch.h"
#include "mesa/main/externalobjects.h"

struct brw_fence {
   struct brw_context *brw;

   enum brw_fence_type {
      /** The fence waits for completion of brw_fence::batch_bo. */
      BRW_FENCE_TYPE_BO_WAIT,

      /** The fence waits for brw_fence::sync_fd to signal. */
      BRW_FENCE_TYPE_SYNC_FD,
   } type;

   union {
      struct brw_bo *batch_bo;

      /* This struct owns the fd. */
      int sync_fd;
   };

   mtx_t mutex;
   bool signalled;
};

struct brw_gl_sync {
   struct gl_sync_object gl;
   struct brw_fence fence;
};

struct intel_semaphore_object {
   struct gl_semaphore_object Base;
   struct drm_syncobj_handle *syncobj;
};

static inline struct intel_semaphore_object *
intel_semaphore_object(struct gl_semaphore_object *sem_obj) {
   return (struct intel_semaphore_object*) sem_obj;
}

static struct gl_semaphore_object *
intel_semaphoreobj_alloc(struct gl_context *ctx, GLuint name)
{
   struct intel_semaphore_object *is_obj = CALLOC_STRUCT(intel_semaphore_object);
   if (!is_obj)
      return NULL;

   _mesa_initialize_semaphore_object(ctx, &is_obj->Base, name);
   return &is_obj->Base;
}

static void
intel_semaphoreobj_free(struct gl_context *ctx,
                     struct gl_semaphore_object *semObj)
{
   _mesa_delete_semaphore_object(ctx, semObj);
}

static void
intel_semaphoreobj_import(struct gl_context *ctx,
                                struct gl_semaphore_object *semObj,
                                int fd)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_screen *screen = brw->screen;
   struct intel_semaphore_object *iSemObj = intel_semaphore_object(semObj);
   iSemObj->syncobj = CALLOC_STRUCT(drm_syncobj_handle);
   iSemObj->syncobj->fd = fd;

   if (drmIoctl(screen->fd, DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE, iSemObj->syncobj) < 0) {
      fprintf(stderr, "DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE failed: %s\n",
              strerror(errno));
      free(iSemObj->syncobj);
   }
}

static void
intel_semaphoreobj_signal(struct gl_context *ctx,
                                       struct gl_semaphore_object *semObj,
                                       GLuint numBufferBarriers,
                                       struct gl_buffer_object **bufObjs,
                                       GLuint numTextureBarriers,
                                       struct gl_texture_object **texObjs,
                                       const GLenum *dstLayouts)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_semaphore_object *iSemObj = intel_semaphore_object(semObj);
   struct drm_i915_gem_exec_fence *fence =
      util_dynarray_grow(&brw->batch.exec_fences, struct drm_i915_gem_exec_fence *, 1);
   fence->flags = I915_EXEC_FENCE_SIGNAL;
   fence->handle = iSemObj->syncobj->handle;
   brw->batch.contains_fence_signal = true;
}

static void
intel_semaphoreobj_wait(struct gl_context *ctx,
                                     struct gl_semaphore_object *semObj,
                                     GLuint numBufferBarriers,
                                     struct gl_buffer_object **bufObjs,
                                     GLuint numTextureBarriers,
                                     struct gl_texture_object **texObjs,
                                     const GLenum *srcLayouts)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_screen *screen = brw->screen;
   struct intel_semaphore_object *iSemObj = intel_semaphore_object(semObj);
   struct drm_syncobj_wait args = {
      .handles = (uintptr_t)&iSemObj->syncobj->handle,
      .count_handles = 1,
   };

   drmIoctl(screen->fd, DRM_IOCTL_SYNCOBJ_WAIT, &args);
}

static void
brw_fence_init(struct brw_context *brw, struct brw_fence *fence,
               enum brw_fence_type type)
{
   fence->brw = brw;
   fence->type = type;
   mtx_init(&fence->mutex, mtx_plain);

   switch (type) {
   case BRW_FENCE_TYPE_BO_WAIT:
      fence->batch_bo = NULL;
      break;
    case BRW_FENCE_TYPE_SYNC_FD:
      fence->sync_fd = -1;
      break;
   }
}

static void
brw_fence_finish(struct brw_fence *fence)
{
   switch (fence->type) {
   case BRW_FENCE_TYPE_BO_WAIT:
      if (fence->batch_bo)
         brw_bo_unreference(fence->batch_bo);
      break;
   case BRW_FENCE_TYPE_SYNC_FD:
      if (fence->sync_fd != -1)
         close(fence->sync_fd);
      break;
   }

   mtx_destroy(&fence->mutex);
}

static bool MUST_CHECK
brw_fence_insert_locked(struct brw_context *brw, struct brw_fence *fence)
{
   __DRIcontext *driContext = brw->driContext;
   __DRIdrawable *driDrawable = driContext->driDrawablePriv;

   /*
    * From KHR_fence_sync:
    *
    *   When the condition of the sync object is satisfied by the fence
    *   command, the sync is signaled by the associated client API context,
    *   causing any eglClientWaitSyncKHR commands (see below) blocking on
    *   <sync> to unblock. The only condition currently supported is
    *   EGL_SYNC_PRIOR_COMMANDS_COMPLETE_KHR, which is satisfied by
    *   completion of the fence command corresponding to the sync object,
    *   and all preceding commands in the associated client API context's
    *   command stream. The sync object will not be signaled until all
    *   effects from these commands on the client API's internal and
    *   framebuffer state are fully realized. No other state is affected by
    *   execution of the fence command.
    *
    * Note the emphasis there on ensuring that the framebuffer is fully
    * realised before the fence is signaled. We cannot just flush the batch,
    * but must also resolve the drawable first. The importance of this is,
    * for example, in creating a fence for a frame to be passed to a
    * remote compositor. Without us flushing the drawable explicitly, the
    * resolve will be in a following batch (when the client finally calls
    * SwapBuffers, or triggers a resolve via some other path) and so the
    * compositor may read the incomplete framebuffer instead.
    */
   if (driDrawable)
      brw_resolve_for_dri2_flush(brw, driDrawable);
   brw_emit_mi_flush(brw);

   switch (fence->type) {
   case BRW_FENCE_TYPE_BO_WAIT:
      assert(!fence->batch_bo);
      assert(!fence->signalled);

      fence->batch_bo = brw->batch.batch.bo;
      brw_bo_reference(fence->batch_bo);

      if (brw_batch_flush(brw) < 0) {
         brw_bo_unreference(fence->batch_bo);
         fence->batch_bo = NULL;
         return false;
      }
      break;
   case BRW_FENCE_TYPE_SYNC_FD:
      assert(!fence->signalled);

      if (fence->sync_fd == -1) {
         /* Create an out-fence that signals after all pending commands
          * complete.
          */
         if (brw_batch_flush_fence(brw, -1, &fence->sync_fd) < 0)
            return false;
         assert(fence->sync_fd != -1);
      } else {
         /* Wait on the in-fence before executing any subsequently submitted
          * commands.
          */
         if (brw_batch_flush(brw) < 0)
            return false;

         /* Emit a dummy batch just for the fence. */
         brw_emit_mi_flush(brw);
         if (brw_batch_flush_fence(brw, fence->sync_fd, NULL) < 0)
            return false;
      }
      break;
   }

   return true;
}

static bool MUST_CHECK
brw_fence_insert(struct brw_context *brw, struct brw_fence *fence)
{
   bool ret;

   mtx_lock(&fence->mutex);
   ret = brw_fence_insert_locked(brw, fence);
   mtx_unlock(&fence->mutex);

   return ret;
}

static bool
brw_fence_has_completed_locked(struct brw_fence *fence)
{
   if (fence->signalled)
      return true;

   switch (fence->type) {
   case BRW_FENCE_TYPE_BO_WAIT:
      if (!fence->batch_bo) {
         /* There may be no batch if brw_batch_flush() failed. */
         return false;
      }

      if (brw_bo_busy(fence->batch_bo))
         return false;

      brw_bo_unreference(fence->batch_bo);
      fence->batch_bo = NULL;
      fence->signalled = true;

      return true;

   case BRW_FENCE_TYPE_SYNC_FD:
      assert(fence->sync_fd != -1);

      if (sync_wait(fence->sync_fd, 0) == -1)
         return false;

      fence->signalled = true;

      return true;
   }

   return false;
}

static bool
brw_fence_has_completed(struct brw_fence *fence)
{
   bool ret;

   mtx_lock(&fence->mutex);
   ret = brw_fence_has_completed_locked(fence);
   mtx_unlock(&fence->mutex);

   return ret;
}

static bool
brw_fence_client_wait_locked(struct brw_context *brw, struct brw_fence *fence,
                             uint64_t timeout)
{
   int32_t timeout_i32;

   if (fence->signalled)
      return true;

   switch (fence->type) {
   case BRW_FENCE_TYPE_BO_WAIT:
      if (!fence->batch_bo) {
         /* There may be no batch if brw_batch_flush() failed. */
         return false;
      }

      /* DRM_IOCTL_I915_GEM_WAIT uses a signed 64 bit timeout and returns
       * immediately for timeouts <= 0.  The best we can do is to clamp the
       * timeout to INT64_MAX.  This limits the maximum timeout from 584 years to
       * 292 years - likely not a big deal.
       */
      if (timeout > INT64_MAX)
         timeout = INT64_MAX;

      if (brw_bo_wait(fence->batch_bo, timeout) != 0)
         return false;

      fence->signalled = true;
      brw_bo_unreference(fence->batch_bo);
      fence->batch_bo = NULL;

      return true;
   case BRW_FENCE_TYPE_SYNC_FD:
      if (fence->sync_fd == -1)
         return false;

      if (timeout > INT32_MAX)
         timeout_i32 = -1;
      else
         timeout_i32 = timeout;

      if (sync_wait(fence->sync_fd, timeout_i32) == -1)
         return false;

      fence->signalled = true;
      return true;
   }

   assert(!"bad enum brw_fence_type");
   return false;
}

/**
 * Return true if the function successfully signals or has already signalled.
 * (This matches the behavior expected from __DRI2fence::client_wait_sync).
 */
static bool
brw_fence_client_wait(struct brw_context *brw, struct brw_fence *fence,
                      uint64_t timeout)
{
   bool ret;

   mtx_lock(&fence->mutex);
   ret = brw_fence_client_wait_locked(brw, fence, timeout);
   mtx_unlock(&fence->mutex);

   return ret;
}

static void
brw_fence_server_wait(struct brw_context *brw, struct brw_fence *fence)
{
   switch (fence->type) {
   case BRW_FENCE_TYPE_BO_WAIT:
      /* We have nothing to do for WaitSync.  Our GL command stream is sequential,
       * so given that the sync object has already flushed the batchbuffer, any
       * batchbuffers coming after this waitsync will naturally not occur until
       * the previous one is done.
       */
      break;
   case BRW_FENCE_TYPE_SYNC_FD:
      assert(fence->sync_fd != -1);

      /* The user wants explicit synchronization, so give them what they want. */
      if (!brw_fence_insert(brw, fence)) {
         /* FIXME: There exists no way yet to report an error here. If an error
          * occurs, continue silently and hope for the best.
          */
      }
      break;
   }
}

static struct gl_sync_object *
brw_gl_new_sync(struct gl_context *ctx)
{
   struct brw_gl_sync *sync;

   sync = calloc(1, sizeof(*sync));
   if (!sync)
      return NULL;

   return &sync->gl;
}

static void
brw_gl_delete_sync(struct gl_context *ctx, struct gl_sync_object *_sync)
{
   struct brw_gl_sync *sync = (struct brw_gl_sync *) _sync;

   brw_fence_finish(&sync->fence);
   free(sync->gl.Label);
   free(sync);
}

static void
brw_gl_fence_sync(struct gl_context *ctx, struct gl_sync_object *_sync,
                  GLenum condition, GLbitfield flags)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_gl_sync *sync = (struct brw_gl_sync *) _sync;

   /* brw_fence_insert_locked() assumes it must do a complete flush */
   assert(condition == GL_SYNC_GPU_COMMANDS_COMPLETE);

   brw_fence_init(brw, &sync->fence, BRW_FENCE_TYPE_BO_WAIT);

   if (!brw_fence_insert_locked(brw, &sync->fence)) {
      /* FIXME: There exists no way to report a GL error here. If an error
       * occurs, continue silently and hope for the best.
       */
   }
}

static void
brw_gl_client_wait_sync(struct gl_context *ctx, struct gl_sync_object *_sync,
                        GLbitfield flags, GLuint64 timeout)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_gl_sync *sync = (struct brw_gl_sync *) _sync;

   if (brw_fence_client_wait(brw, &sync->fence, timeout))
      sync->gl.StatusFlag = 1;
}

static void
brw_gl_server_wait_sync(struct gl_context *ctx, struct gl_sync_object *_sync,
                          GLbitfield flags, GLuint64 timeout)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_gl_sync *sync = (struct brw_gl_sync *) _sync;

   brw_fence_server_wait(brw, &sync->fence);
}

static void
brw_gl_check_sync(struct gl_context *ctx, struct gl_sync_object *_sync)
{
   struct brw_gl_sync *sync = (struct brw_gl_sync *) _sync;

   if (brw_fence_has_completed(&sync->fence))
      sync->gl.StatusFlag = 1;
}

void
brw_init_syncobj_functions(struct dd_function_table *functions)
{
   functions->NewSyncObject = brw_gl_new_sync;
   functions->DeleteSyncObject = brw_gl_delete_sync;
   functions->FenceSync = brw_gl_fence_sync;
   functions->CheckSync = brw_gl_check_sync;
   functions->ClientWaitSync = brw_gl_client_wait_sync;
   functions->ServerWaitSync = brw_gl_server_wait_sync;
   functions->NewSemaphoreObject = intel_semaphoreobj_alloc;
   functions->DeleteSemaphoreObject = intel_semaphoreobj_free;
   functions->ImportSemaphoreFd = intel_semaphoreobj_import;
   functions->ServerSignalSemaphoreObject = intel_semaphoreobj_signal;
   functions->ServerWaitSemaphoreObject = intel_semaphoreobj_wait;
}

static void *
brw_dri_create_fence(__DRIcontext *ctx)
{
   struct brw_context *brw = ctx->driverPrivate;
   struct brw_fence *fence;

   fence = calloc(1, sizeof(*fence));
   if (!fence)
      return NULL;

   brw_fence_init(brw, fence, BRW_FENCE_TYPE_BO_WAIT);

   if (!brw_fence_insert_locked(brw, fence)) {
      brw_fence_finish(fence);
      free(fence);
      return NULL;
   }

   return fence;
}

static void
brw_dri_destroy_fence(__DRIscreen *dri_screen, void *_fence)
{
   struct brw_fence *fence = _fence;

   brw_fence_finish(fence);
   free(fence);
}

static GLboolean
brw_dri_client_wait_sync(__DRIcontext *ctx, void *_fence, unsigned flags,
                         uint64_t timeout)
{
   struct brw_fence *fence = _fence;

   return brw_fence_client_wait(fence->brw, fence, timeout);
}

static void
brw_dri_server_wait_sync(__DRIcontext *ctx, void *_fence, unsigned flags)
{
   struct brw_fence *fence = _fence;

   /* We might be called here with a NULL fence as a result of WaitSyncKHR
    * on a EGL_KHR_reusable_sync fence. Nothing to do here in such case.
    */
   if (!fence)
      return;

   brw_fence_server_wait(fence->brw, fence);
}

static unsigned
brw_dri_get_capabilities(__DRIscreen *dri_screen)
{
   struct brw_screen *screen = dri_screen->driverPrivate;
   unsigned caps = 0;

   if (screen->has_exec_fence)
      caps |=  __DRI_FENCE_CAP_NATIVE_FD;

   return caps;
}

static void *
brw_dri_create_fence_fd(__DRIcontext *dri_ctx, int fd)
{
   struct brw_context *brw = dri_ctx->driverPrivate;
   struct brw_fence *fence;

   assert(brw->screen->has_exec_fence);

   fence = calloc(1, sizeof(*fence));
   if (!fence)
      return NULL;

   brw_fence_init(brw, fence, BRW_FENCE_TYPE_SYNC_FD);

   if (fd == -1) {
      /* Create an out-fence fd */
      if (!brw_fence_insert_locked(brw, fence))
         goto fail;
   } else {
      /* Import the sync fd as an in-fence. */
      fence->sync_fd = os_dupfd_cloexec(fd);
   }

   assert(fence->sync_fd != -1);

   return fence;

fail:
   brw_fence_finish(fence);
   free(fence);
   return NULL;
}

static int
brw_dri_get_fence_fd_locked(struct brw_fence *fence)
{
   assert(fence->type == BRW_FENCE_TYPE_SYNC_FD);
   return os_dupfd_cloexec(fence->sync_fd);
}

static int
brw_dri_get_fence_fd(__DRIscreen *dri_screen, void *_fence)
{
   struct brw_fence *fence = _fence;
   int fd;

   mtx_lock(&fence->mutex);
   fd = brw_dri_get_fence_fd_locked(fence);
   mtx_unlock(&fence->mutex);

   return fd;
}

const __DRI2fenceExtension brwFenceExtension = {
   .base = { __DRI2_FENCE, 2 },

   .create_fence = brw_dri_create_fence,
   .destroy_fence = brw_dri_destroy_fence,
   .client_wait_sync = brw_dri_client_wait_sync,
   .server_wait_sync = brw_dri_server_wait_sync,
   .get_fence_from_cl_event = NULL,
   .get_capabilities = brw_dri_get_capabilities,
   .create_fence_fd = brw_dri_create_fence_fd,
   .get_fence_fd = brw_dri_get_fence_fd,
};
