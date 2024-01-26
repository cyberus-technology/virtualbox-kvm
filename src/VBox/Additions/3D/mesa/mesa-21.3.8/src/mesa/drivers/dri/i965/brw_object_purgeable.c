/*
 * Copyright © 2010 Intel Corporation
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

/**
 * @file brw_object_purgeable.c
 *
 * The driver implementation of the GL_APPLE_object_purgeable extension.
 */

#include "main/mtypes.h"
#include "main/macros.h"
#include "main/bufferobj.h"

#include "brw_context.h"
#include "brw_buffer_objects.h"
#include "brw_fbo.h"
#include "brw_mipmap_tree.h"

static GLenum
brw_buffer_purgeable(struct brw_bo *buffer)
{
   int retained = 0;

   if (buffer != NULL)
      retained = brw_bo_madvise(buffer, I915_MADV_DONTNEED);

   return retained ? GL_VOLATILE_APPLE : GL_RELEASED_APPLE;
}

static GLenum
brw_buffer_object_purgeable(struct gl_context * ctx,
                            struct gl_buffer_object *obj,
                            GLenum option)
{
   struct brw_buffer_object *intel_obj = brw_buffer_object(obj);

   if (intel_obj->buffer != NULL)
      return brw_buffer_purgeable(intel_obj->buffer);

   if (option == GL_RELEASED_APPLE) {
      return GL_RELEASED_APPLE;
   } else {
      /* XXX Create the buffer and madvise(MADV_DONTNEED)? */
      return brw_buffer_purgeable(intel_obj->buffer);
   }
}

static GLenum
brw_texture_object_purgeable(struct gl_context * ctx,
                             struct gl_texture_object *obj,
                             GLenum option)
{
   struct brw_texture_object *intel;

   (void) ctx;
   (void) option;

   intel = brw_texture_object(obj);
   if (intel->mt == NULL || intel->mt->bo == NULL)
      return GL_RELEASED_APPLE;

   return brw_buffer_purgeable(intel->mt->bo);
}

static GLenum
brw_render_object_purgeable(struct gl_context * ctx,
                            struct gl_renderbuffer *obj,
                            GLenum option)
{
   struct brw_renderbuffer *intel;

   (void) ctx;
   (void) option;

   intel = brw_renderbuffer(obj);
   if (intel->mt == NULL)
      return GL_RELEASED_APPLE;

   return brw_buffer_purgeable(intel->mt->bo);
}

static int
brw_bo_unpurgeable(struct brw_bo *buffer)
{
   int retained;

   retained = 0;
   if (buffer != NULL)
      retained = brw_bo_madvise(buffer, I915_MADV_WILLNEED);

   return retained;
}

static GLenum
brw_buffer_object_unpurgeable(struct gl_context * ctx,
                              struct gl_buffer_object *obj,
                              GLenum option)
{
   struct brw_buffer_object *intel = brw_buffer_object(obj);

   (void) ctx;

   if (!intel->buffer)
      return GL_UNDEFINED_APPLE;

   if (option == GL_UNDEFINED_APPLE || !brw_bo_unpurgeable(intel->buffer)) {
      brw_bo_unreference(intel->buffer);
      intel->buffer = NULL;
      return GL_UNDEFINED_APPLE;
   }

   return GL_RETAINED_APPLE;
}

static GLenum
brw_texture_object_unpurgeable(struct gl_context * ctx,
                                 struct gl_texture_object *obj,
                                 GLenum option)
{
   struct brw_texture_object *intel;

   (void) ctx;

   intel = brw_texture_object(obj);
   if (intel->mt == NULL || intel->mt->bo == NULL)
      return GL_UNDEFINED_APPLE;

   if (option == GL_UNDEFINED_APPLE || !brw_bo_unpurgeable(intel->mt->bo)) {
      brw_miptree_release(&intel->mt);
      return GL_UNDEFINED_APPLE;
   }

   return GL_RETAINED_APPLE;
}

static GLenum
brw_render_object_unpurgeable(struct gl_context * ctx,
                              struct gl_renderbuffer *obj,
                              GLenum option)
{
   struct brw_renderbuffer *intel;

   (void) ctx;

   intel = brw_renderbuffer(obj);
   if (intel->mt == NULL)
      return GL_UNDEFINED_APPLE;

   if (option == GL_UNDEFINED_APPLE || !brw_bo_unpurgeable(intel->mt->bo)) {
      brw_miptree_release(&intel->mt);
      return GL_UNDEFINED_APPLE;
   }

   return GL_RETAINED_APPLE;
}

void
brw_init_object_purgeable_functions(struct dd_function_table *functions)
{
   functions->BufferObjectPurgeable = brw_buffer_object_purgeable;
   functions->TextureObjectPurgeable = brw_texture_object_purgeable;
   functions->RenderObjectPurgeable = brw_render_object_purgeable;

   functions->BufferObjectUnpurgeable = brw_buffer_object_unpurgeable;
   functions->TextureObjectUnpurgeable = brw_texture_object_unpurgeable;
   functions->RenderObjectUnpurgeable = brw_render_object_unpurgeable;
}
