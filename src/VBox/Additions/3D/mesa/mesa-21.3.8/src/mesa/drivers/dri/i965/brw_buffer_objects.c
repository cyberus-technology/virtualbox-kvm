/*
 * Copyright 2003 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file brw_buffer_objects.c
 *
 * This provides core GL buffer object functionality.
 */

#include "main/mtypes.h"
#include "main/macros.h"
#include "main/streaming-load-memcpy.h"
#include "main/bufferobj.h"
#include "x86/common_x86_asm.h"
#include "util/u_memory.h"

#include "brw_context.h"
#include "brw_blorp.h"
#include "brw_buffer_objects.h"
#include "brw_batch.h"

static void
mark_buffer_gpu_usage(struct brw_buffer_object *intel_obj,
                               uint32_t offset, uint32_t size)
{
   intel_obj->gpu_active_start = MIN2(intel_obj->gpu_active_start, offset);
   intel_obj->gpu_active_end = MAX2(intel_obj->gpu_active_end, offset + size);
}

static void
mark_buffer_inactive(struct brw_buffer_object *intel_obj)
{
   intel_obj->gpu_active_start = ~0;
   intel_obj->gpu_active_end = 0;
}

static void
mark_buffer_valid_data(struct brw_buffer_object *intel_obj,
                       uint32_t offset, uint32_t size)
{
   intel_obj->valid_data_start = MIN2(intel_obj->valid_data_start, offset);
   intel_obj->valid_data_end = MAX2(intel_obj->valid_data_end, offset + size);
}

static void
mark_buffer_invalid(struct brw_buffer_object *intel_obj)
{
   intel_obj->valid_data_start = ~0;
   intel_obj->valid_data_end = 0;
}

/** Allocates a new brw_bo to store the data for the buffer object. */
static void
alloc_buffer_object(struct brw_context *brw,
                    struct brw_buffer_object *intel_obj)
{
   const struct gl_context *ctx = &brw->ctx;

   uint64_t size = intel_obj->Base.Size;
   if (ctx->Const.RobustAccess) {
      /* Pad out buffer objects with an extra 2kB (half a page).
       *
       * When pushing UBOs, we need to safeguard against 3DSTATE_CONSTANT_*
       * reading out of bounds memory.  The application might bind a UBO that's
       * smaller than what the program expects.  Ideally, we'd bind an extra
       * push buffer containing zeros, but we have a limited number of those,
       * so it's not always viable.  Our only safe option is to pad all buffer
       * objects by the maximum push data length, so that it will never read
       * past the end of a BO.
       *
       * This is unfortunate, but it should result in at most 1 extra page,
       * which probably isn't too terrible.
       */
      size += 64 * 32; /* max read length of 64 256-bit units */
   }
   intel_obj->buffer =
      brw_bo_alloc(brw->bufmgr, "bufferobj", size, BRW_MEMZONE_OTHER);

   /* the buffer might be bound as a uniform buffer, need to update it
    */
   if (intel_obj->Base.UsageHistory & USAGE_UNIFORM_BUFFER)
      brw->ctx.NewDriverState |= BRW_NEW_UNIFORM_BUFFER;
   if (intel_obj->Base.UsageHistory & USAGE_SHADER_STORAGE_BUFFER)
      brw->ctx.NewDriverState |= BRW_NEW_UNIFORM_BUFFER;
   if (intel_obj->Base.UsageHistory & USAGE_TEXTURE_BUFFER)
      brw->ctx.NewDriverState |= BRW_NEW_TEXTURE_BUFFER;
   if (intel_obj->Base.UsageHistory & USAGE_ATOMIC_COUNTER_BUFFER)
      brw->ctx.NewDriverState |= BRW_NEW_UNIFORM_BUFFER;

   mark_buffer_inactive(intel_obj);
   mark_buffer_invalid(intel_obj);
}

static void
release_buffer(struct brw_buffer_object *intel_obj)
{
   brw_bo_unreference(intel_obj->buffer);
   intel_obj->buffer = NULL;
}

/**
 * The NewBufferObject() driver hook.
 *
 * Allocates a new brw_buffer_object structure and initializes it.
 *
 * There is some duplication between mesa's bufferobjects and our
 * bufmgr buffers.  Both have an integer handle and a hashtable to
 * lookup an opaque structure.  It would be nice if the handles and
 * internal structure where somehow shared.
 */
static struct gl_buffer_object *
brw_new_buffer_object(struct gl_context * ctx, GLuint name)
{
   struct brw_buffer_object *obj = CALLOC_STRUCT(brw_buffer_object);
   if (!obj) {
      _mesa_error_no_memory(__func__);
      return NULL;
   }

   _mesa_initialize_buffer_object(ctx, &obj->Base, name);

   obj->buffer = NULL;

   return &obj->Base;
}

/**
 * The DeleteBuffer() driver hook.
 *
 * Deletes a single OpenGL buffer object.  Used by glDeleteBuffers().
 */
static void
brw_delete_buffer(struct gl_context * ctx, struct gl_buffer_object *obj)
{
   struct brw_buffer_object *intel_obj = brw_buffer_object(obj);

   assert(intel_obj);

   /* Buffer objects are automatically unmapped when deleting according
    * to the spec, but Mesa doesn't do UnmapBuffer for us at context destroy
    * (though it does if you call glDeleteBuffers)
    */
   _mesa_buffer_unmap_all_mappings(ctx, obj);

   brw_bo_unreference(intel_obj->buffer);
   _mesa_delete_buffer_object(ctx, obj);
}


/**
 * The BufferData() driver hook.
 *
 * Implements glBufferData(), which recreates a buffer object's data store
 * and populates it with the given data, if present.
 *
 * Any data that was previously stored in the buffer object is lost.
 *
 * \return true for success, false if out of memory
 */
static GLboolean
brw_buffer_data(struct gl_context *ctx,
                GLenum target,
                GLsizeiptrARB size,
                const GLvoid *data,
                GLenum usage,
                GLbitfield storageFlags,
                struct gl_buffer_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_buffer_object *intel_obj = brw_buffer_object(obj);

   /* Part of the ABI, but this function doesn't use it.
    */
   (void) target;

   intel_obj->Base.Size = size;
   intel_obj->Base.Usage = usage;
   intel_obj->Base.StorageFlags = storageFlags;

   assert(!obj->Mappings[MAP_USER].Pointer); /* Mesa should have unmapped it */
   assert(!obj->Mappings[MAP_INTERNAL].Pointer);

   if (intel_obj->buffer != NULL)
      release_buffer(intel_obj);

   if (size != 0) {
      alloc_buffer_object(brw, intel_obj);
      if (!intel_obj->buffer)
         return false;

      if (data != NULL) {
         brw_bo_subdata(intel_obj->buffer, 0, size, data);
         mark_buffer_valid_data(intel_obj, 0, size);
      }
   }

   return true;
}

static GLboolean
brw_buffer_data_mem(struct gl_context *ctx,
                    GLenum target,
                    GLsizeiptrARB size,
                    struct gl_memory_object *memObj,
                    GLuint64 offset,
                    GLenum usage,
                    struct gl_buffer_object *bufObj)
{
   struct brw_buffer_object *intel_obj = brw_buffer_object(bufObj);
   struct brw_memory_object *intel_memObj = brw_memory_object(memObj);

   /* Part of the ABI, but this function doesn't use it.
    */
   (void) target;

   intel_obj->Base.Size = size;
   intel_obj->Base.Usage = usage;
   intel_obj->Base.StorageFlags = 0;

   assert(!bufObj->Mappings[MAP_USER].Pointer); /* Mesa should have unmapped it */
   assert(!bufObj->Mappings[MAP_INTERNAL].Pointer);

   if (intel_obj->buffer != NULL)
      release_buffer(intel_obj);

   if (size != 0) {
      intel_obj->buffer = intel_memObj->bo;
      mark_buffer_valid_data(intel_obj, offset, size);
   }

   return true;
}

/**
 * The BufferSubData() driver hook.
 *
 * Implements glBufferSubData(), which replaces a portion of the data in a
 * buffer object.
 *
 * If the data range specified by (size + offset) extends beyond the end of
 * the buffer or if data is NULL, no copy is performed.
 */
static void
brw_buffer_subdata(struct gl_context *ctx,
                   GLintptrARB offset,
                   GLsizeiptrARB size,
                   const GLvoid *data,
                   struct gl_buffer_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_buffer_object *intel_obj = brw_buffer_object(obj);
   bool busy;

   if (size == 0)
      return;

   assert(intel_obj);

   /* See if we can unsynchronized write the data into the user's BO. This
    * avoids GPU stalls in unfortunately common user patterns (uploading
    * sequentially into a BO, with draw calls in between each upload).
    *
    * Once we've hit this path, we mark this GL BO as preferring stalling to
    * blits, so that we can hopefully hit this path again in the future
    * (otherwise, an app that might occasionally stall but mostly not will end
    * up with blitting all the time, at the cost of bandwidth)
    */
   if (offset + size <= intel_obj->gpu_active_start ||
       intel_obj->gpu_active_end <= offset ||
       offset + size <= intel_obj->valid_data_start ||
       intel_obj->valid_data_end <= offset) {
      void *map = brw_bo_map(brw, intel_obj->buffer, MAP_WRITE | MAP_ASYNC);
      memcpy(map + offset, data, size);
      brw_bo_unmap(intel_obj->buffer);

      if (intel_obj->gpu_active_end > intel_obj->gpu_active_start)
         intel_obj->prefer_stall_to_blit = true;

      mark_buffer_valid_data(intel_obj, offset, size);
      return;
   }

   busy =
      brw_bo_busy(intel_obj->buffer) ||
      brw_batch_references(&brw->batch, intel_obj->buffer);

   if (busy) {
      if (size == intel_obj->Base.Size ||
          (intel_obj->valid_data_start >= offset &&
           intel_obj->valid_data_end <= offset + size)) {
         /* Replace the current busy bo so the subdata doesn't stall. */
         brw_bo_unreference(intel_obj->buffer);
         alloc_buffer_object(brw, intel_obj);
      } else if (!intel_obj->prefer_stall_to_blit) {
         perf_debug("Using a blit copy to avoid stalling on "
                    "glBufferSubData(%ld, %ld) (%ldkb) to a busy "
                    "(%d-%d) / valid (%d-%d) buffer object.\n",
                    (long)offset, (long)offset + size, (long)(size/1024),
                    intel_obj->gpu_active_start,
                    intel_obj->gpu_active_end,
                    intel_obj->valid_data_start,
                    intel_obj->valid_data_end);
         struct brw_bo *temp_bo =
            brw_bo_alloc(brw->bufmgr, "subdata temp", size, BRW_MEMZONE_OTHER);

         brw_bo_subdata(temp_bo, 0, size, data);

         brw_blorp_copy_buffers(brw,
                                temp_bo, 0,
                                intel_obj->buffer, offset,
                                size);
         brw_emit_mi_flush(brw);

         brw_bo_unreference(temp_bo);
         mark_buffer_valid_data(intel_obj, offset, size);
         return;
      } else {
         perf_debug("Stalling on glBufferSubData(%ld, %ld) (%ldkb) to a busy "
                    "(%d-%d) buffer object.  Use glMapBufferRange() to "
                    "avoid this.\n",
                    (long)offset, (long)offset + size, (long)(size/1024),
                    intel_obj->gpu_active_start,
                    intel_obj->gpu_active_end);
         brw_batch_flush(brw);
      }
   }

   brw_bo_subdata(intel_obj->buffer, offset, size, data);
   mark_buffer_inactive(intel_obj);
   mark_buffer_valid_data(intel_obj, offset, size);
}

/* Typedef for memcpy function (used in brw_get_buffer_subdata below). */
typedef void *(*mem_copy_fn)(void *dest, const void *src, size_t n);

/**
 * The GetBufferSubData() driver hook.
 *
 * Implements glGetBufferSubData(), which copies a subrange of a buffer
 * object into user memory.
 */
static void
brw_get_buffer_subdata(struct gl_context *ctx,
                       GLintptrARB offset,
                       GLsizeiptrARB size,
                       GLvoid *data,
                       struct gl_buffer_object *obj)
{
   struct brw_buffer_object *intel_obj = brw_buffer_object(obj);
   struct brw_context *brw = brw_context(ctx);

   assert(intel_obj);
   if (brw_batch_references(&brw->batch, intel_obj->buffer)) {
      brw_batch_flush(brw);
   }

   unsigned int map_flags = MAP_READ;
   mem_copy_fn memcpy_fn = memcpy;
#ifdef USE_SSE41
   if (!intel_obj->buffer->cache_coherent && cpu_has_sse4_1) {
      /* Rather than acquire a new WB mmaping of the buffer object and pull
       * it into the CPU cache, keep using the WC mmap that we have for writes,
       * and use the magic movntd instructions instead.
       */
      map_flags |= MAP_COHERENT;
      memcpy_fn = (mem_copy_fn) _mesa_streaming_load_memcpy;
   }
#endif

   void *map = brw_bo_map(brw, intel_obj->buffer, map_flags);
   if (unlikely(!map)) {
      _mesa_error_no_memory(__func__);
      return;
   }
   memcpy_fn(data, map + offset, size);
   brw_bo_unmap(intel_obj->buffer);

   mark_buffer_inactive(intel_obj);
}


/**
 * The MapBufferRange() driver hook.
 *
 * This implements both glMapBufferRange() and glMapBuffer().
 *
 * The goal of this extension is to allow apps to accumulate their rendering
 * at the same time as they accumulate their buffer object.  Without it,
 * you'd end up blocking on execution of rendering every time you mapped
 * the buffer to put new data in.
 *
 * We support it in 3 ways: If unsynchronized, then don't bother
 * flushing the batchbuffer before mapping the buffer, which can save blocking
 * in many cases.  If we would still block, and they allow the whole buffer
 * to be invalidated, then just allocate a new buffer to replace the old one.
 * If not, and we'd block, and they allow the subrange of the buffer to be
 * invalidated, then we can make a new little BO, let them write into that,
 * and blit it into the real BO at unmap time.
 */
static void *
brw_map_buffer_range(struct gl_context *ctx,
                     GLintptr offset, GLsizeiptr length,
                     GLbitfield access, struct gl_buffer_object *obj,
                     gl_map_buffer_index index)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_buffer_object *intel_obj = brw_buffer_object(obj);

   assert(intel_obj);

   STATIC_ASSERT(GL_MAP_UNSYNCHRONIZED_BIT == MAP_ASYNC);
   STATIC_ASSERT(GL_MAP_WRITE_BIT == MAP_WRITE);
   STATIC_ASSERT(GL_MAP_READ_BIT == MAP_READ);
   STATIC_ASSERT(GL_MAP_PERSISTENT_BIT == MAP_PERSISTENT);
   STATIC_ASSERT(GL_MAP_COHERENT_BIT == MAP_COHERENT);
   assert((access & MAP_INTERNAL_MASK) == 0);

   /* _mesa_MapBufferRange (GL entrypoint) sets these, but the vbo module also
    * internally uses our functions directly.
    */
   obj->Mappings[index].Offset = offset;
   obj->Mappings[index].Length = length;
   obj->Mappings[index].AccessFlags = access;

   if (intel_obj->buffer == NULL) {
      obj->Mappings[index].Pointer = NULL;
      return NULL;
   }

   /* If the access is synchronized (like a normal buffer mapping), then get
    * things flushed out so the later mapping syncs appropriately through GEM.
    * If the user doesn't care about existing buffer contents and mapping would
    * cause us to block, then throw out the old buffer.
    *
    * If they set INVALIDATE_BUFFER, we can pitch the current contents to
    * achieve the required synchronization.
    */
   if (!(access & GL_MAP_UNSYNCHRONIZED_BIT)) {
      if (brw_batch_references(&brw->batch, intel_obj->buffer)) {
         if (access & GL_MAP_INVALIDATE_BUFFER_BIT) {
            brw_bo_unreference(intel_obj->buffer);
            alloc_buffer_object(brw, intel_obj);
         } else {
            perf_debug("Stalling on the GPU for mapping a busy buffer "
                       "object\n");
            brw_batch_flush(brw);
         }
      } else if (brw_bo_busy(intel_obj->buffer) &&
                 (access & GL_MAP_INVALIDATE_BUFFER_BIT)) {
         brw_bo_unreference(intel_obj->buffer);
         alloc_buffer_object(brw, intel_obj);
      }
   }

   if (access & MAP_WRITE)
      mark_buffer_valid_data(intel_obj, offset, length);

   /* If the user is mapping a range of an active buffer object but
    * doesn't require the current contents of that range, make a new
    * BO, and we'll copy what they put in there out at unmap or
    * FlushRange time.
    *
    * That is, unless they're looking for a persistent mapping -- we would
    * need to do blits in the MemoryBarrier call, and it's easier to just do a
    * GPU stall and do a mapping.
    */
   if (!(access & (GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_PERSISTENT_BIT)) &&
       (access & GL_MAP_INVALIDATE_RANGE_BIT) &&
       brw_bo_busy(intel_obj->buffer)) {
      /* Ensure that the base alignment of the allocation meets the alignment
       * guarantees the driver has advertised to the application.
       */
      const unsigned alignment = ctx->Const.MinMapBufferAlignment;

      intel_obj->map_extra[index] = (uintptr_t) offset % alignment;
      intel_obj->range_map_bo[index] =
         brw_bo_alloc(brw->bufmgr, "BO blit temp",
                      length + intel_obj->map_extra[index],
                      BRW_MEMZONE_OTHER);
      void *map = brw_bo_map(brw, intel_obj->range_map_bo[index], access);
      obj->Mappings[index].Pointer = map + intel_obj->map_extra[index];
      return obj->Mappings[index].Pointer;
   }

   void *map = brw_bo_map(brw, intel_obj->buffer, access);
   if (!(access & GL_MAP_UNSYNCHRONIZED_BIT)) {
      mark_buffer_inactive(intel_obj);
   }

   obj->Mappings[index].Pointer = map + offset;
   return obj->Mappings[index].Pointer;
}

/**
 * The FlushMappedBufferRange() driver hook.
 *
 * Implements glFlushMappedBufferRange(), which signifies that modifications
 * have been made to a range of a mapped buffer, and it should be flushed.
 *
 * This is only used for buffers mapped with GL_MAP_FLUSH_EXPLICIT_BIT.
 *
 * Ideally we'd use a BO to avoid taking up cache space for the temporary
 * data, but FlushMappedBufferRange may be followed by further writes to
 * the pointer, so we would have to re-map after emitting our blit, which
 * would defeat the point.
 */
static void
brw_flush_mapped_buffer_range(struct gl_context *ctx,
                              GLintptr offset, GLsizeiptr length,
                              struct gl_buffer_object *obj,
                              gl_map_buffer_index index)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_buffer_object *intel_obj = brw_buffer_object(obj);

   assert(obj->Mappings[index].AccessFlags & GL_MAP_FLUSH_EXPLICIT_BIT);

   /* If we gave a direct mapping of the buffer instead of using a temporary,
    * then there's nothing to do.
    */
   if (intel_obj->range_map_bo[index] == NULL)
      return;

   if (length == 0)
      return;

   /* Note that we're not unmapping our buffer while executing the blit.  We
    * need to have a mapping still at the end of this call, since the user
    * gets to make further modifications and glFlushMappedBufferRange() calls.
    * This is safe, because:
    *
    * - On LLC platforms, we're using a CPU mapping that's coherent with the
    *   GPU (except for the render caches), so the kernel doesn't need to do
    *   any flushing work for us except for what happens at batch exec time
    *   anyway.
    *
    * - On non-LLC platforms, we're using a GTT mapping that writes directly
    *   to system memory (except for the chipset cache that gets flushed at
    *   batch exec time).
    *
    * In both cases we don't need to stall for the previous blit to complete
    * so we can re-map (and we definitely don't want to, since that would be
    * slow): If the user edits a part of their buffer that's previously been
    * blitted, then our lack of synchoronization is fine, because either
    * they'll get some too-new data in the first blit and not do another blit
    * of that area (but in that case the results are undefined), or they'll do
    * another blit of that area and the complete newer data will land the
    * second time.
    */
   brw_blorp_copy_buffers(brw,
                          intel_obj->range_map_bo[index],
                          intel_obj->map_extra[index] + offset,
                          intel_obj->buffer,
                          obj->Mappings[index].Offset + offset,
                          length);
   mark_buffer_gpu_usage(intel_obj,
                         obj->Mappings[index].Offset + offset,
                         length);
   brw_emit_mi_flush(brw);
}


/**
 * The UnmapBuffer() driver hook.
 *
 * Implements glUnmapBuffer().
 */
static GLboolean
brw_unmap_buffer(struct gl_context *ctx,
                 struct gl_buffer_object *obj,
                 gl_map_buffer_index index)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_buffer_object *intel_obj = brw_buffer_object(obj);

   assert(intel_obj);
   assert(obj->Mappings[index].Pointer);
   if (intel_obj->range_map_bo[index] != NULL) {
      brw_bo_unmap(intel_obj->range_map_bo[index]);

      if (!(obj->Mappings[index].AccessFlags & GL_MAP_FLUSH_EXPLICIT_BIT)) {
         brw_blorp_copy_buffers(brw,
                                intel_obj->range_map_bo[index],
                                intel_obj->map_extra[index],
                                intel_obj->buffer, obj->Mappings[index].Offset,
                                obj->Mappings[index].Length);
         mark_buffer_gpu_usage(intel_obj, obj->Mappings[index].Offset,
                               obj->Mappings[index].Length);
         brw_emit_mi_flush(brw);
      }

      /* Since we've emitted some blits to buffers that will (likely) be used
       * in rendering operations in other cache domains in this batch, emit a
       * flush.  Once again, we wish for a domain tracker in libdrm to cover
       * usage inside of a batchbuffer.
       */

      brw_bo_unreference(intel_obj->range_map_bo[index]);
      intel_obj->range_map_bo[index] = NULL;
   } else if (intel_obj->buffer != NULL) {
      brw_bo_unmap(intel_obj->buffer);
   }
   obj->Mappings[index].Pointer = NULL;
   obj->Mappings[index].Offset = 0;
   obj->Mappings[index].Length = 0;

   return true;
}

/**
 * Gets a pointer to the object's BO, and marks the given range as being used
 * on the GPU.
 *
 * Anywhere that uses buffer objects in the pipeline should be using this to
 * mark the range of the buffer that is being accessed by the pipeline.
 */
struct brw_bo *
brw_bufferobj_buffer(struct brw_context *brw,
                     struct brw_buffer_object *intel_obj,
                     uint32_t offset, uint32_t size, bool write)
{
   /* This is needed so that things like transform feedback and texture buffer
    * objects that need a BO but don't want to check that they exist for
    * draw-time validation can just always get a BO from a GL buffer object.
    */
   if (intel_obj->buffer == NULL)
      alloc_buffer_object(brw, intel_obj);

   mark_buffer_gpu_usage(intel_obj, offset, size);

   /* If writing, (conservatively) mark this section as having valid data. */
   if (write)
      mark_buffer_valid_data(intel_obj, offset, size);

   return intel_obj->buffer;
}

/**
 * The CopyBufferSubData() driver hook.
 *
 * Implements glCopyBufferSubData(), which copies a portion of one buffer
 * object's data to another.  Independent source and destination offsets
 * are allowed.
 */
static void
brw_copy_buffer_subdata(struct gl_context *ctx,
                        struct gl_buffer_object *src,
                        struct gl_buffer_object *dst,
                        GLintptr read_offset, GLintptr write_offset,
                        GLsizeiptr size)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_buffer_object *intel_src = brw_buffer_object(src);
   struct brw_buffer_object *intel_dst = brw_buffer_object(dst);
   struct brw_bo *src_bo, *dst_bo;

   if (size == 0)
      return;

   dst_bo = brw_bufferobj_buffer(brw, intel_dst, write_offset, size, true);
   src_bo = brw_bufferobj_buffer(brw, intel_src, read_offset, size, false);

   brw_blorp_copy_buffers(brw,
                          src_bo, read_offset,
                          dst_bo, write_offset, size);

   /* Since we've emitted some blits to buffers that will (likely) be used
    * in rendering operations in other cache domains in this batch, emit a
    * flush.  Once again, we wish for a domain tracker in libdrm to cover
    * usage inside of a batchbuffer.
    */
   brw_emit_mi_flush(brw);
}

void
brw_init_buffer_object_functions(struct dd_function_table *functions)
{
   functions->NewBufferObject = brw_new_buffer_object;
   functions->DeleteBuffer = brw_delete_buffer;
   functions->BufferData = brw_buffer_data;
   functions->BufferDataMem = brw_buffer_data_mem;
   functions->BufferSubData = brw_buffer_subdata;
   functions->GetBufferSubData = brw_get_buffer_subdata;
   functions->MapBufferRange = brw_map_buffer_range;
   functions->FlushMappedBufferRange = brw_flush_mapped_buffer_range;
   functions->UnmapBuffer = brw_unmap_buffer;
   functions->CopyBufferSubData = brw_copy_buffer_subdata;
}
