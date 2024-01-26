/*
 Copyright 2003 VMware, Inc.
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */


#include "compiler/nir/nir.h"
#include "main/api_exec.h"
#include "main/context.h"
#include "main/fbobject.h"
#include "main/extensions.h"
#include "main/glthread.h"
#include "main/macros.h"
#include "main/points.h"
#include "main/version.h"
#include "main/vtxfmt.h"
#include "main/texobj.h"
#include "main/framebuffer.h"
#include "main/stencil.h"
#include "main/state.h"
#include "main/spirv_extensions.h"
#include "main/externalobjects.h"

#include "vbo/vbo.h"

#include "drivers/common/driverfuncs.h"
#include "drivers/common/meta.h"
#include "utils.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_blorp.h"
#include "brw_draw.h"
#include "brw_state.h"

#include "brw_batch.h"
#include "brw_buffer_objects.h"
#include "brw_buffers.h"
#include "brw_fbo.h"
#include "brw_mipmap_tree.h"
#include "brw_pixel.h"
#include "brw_image.h"
#include "brw_tex.h"
#include "brw_tex_obj.h"

#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "util/ralloc.h"
#include "util/debug.h"
#include "util/disk_cache.h"
#include "util/u_memory.h"
#include "isl/isl.h"

#include "common/intel_defines.h"
#include "common/intel_uuid.h"

#include "compiler/spirv/nir_spirv.h"
/***************************************
 * Mesa's Driver Functions
 ***************************************/

const char *const brw_vendor_string = "Intel Open Source Technology Center";

const char *
brw_get_renderer_string(const struct brw_screen *screen)
{
   static char buf[128];
   const char *name = screen->devinfo.name;

   if (!name)
      name = "Intel Unknown";

   snprintf(buf, sizeof(buf), "Mesa DRI %s", name);

   return buf;
}

static const GLubyte *
brw_get_string(struct gl_context * ctx, GLenum name)
{
   const struct brw_context *const brw = brw_context(ctx);

   switch (name) {
   case GL_VENDOR:
      return (GLubyte *) brw_vendor_string;

   case GL_RENDERER:
      return
         (GLubyte *) brw_get_renderer_string(brw->screen);

   default:
      return NULL;
   }
}

static void
brw_set_background_context(struct gl_context *ctx,
                           UNUSED struct util_queue_monitoring *queue_info)
{
   struct brw_context *brw = brw_context(ctx);
   __DRIcontext *driContext = brw->driContext;
   __DRIscreen *driScreen = driContext->driScreenPriv;
   const __DRIbackgroundCallableExtension *backgroundCallable =
      driScreen->dri2.backgroundCallable;

   /* Note: Mesa will only call this function if we've called
    * _mesa_enable_multithreading().  We only do that if the loader exposed
    * the __DRI_BACKGROUND_CALLABLE extension.  So we know that
    * backgroundCallable is not NULL.
    */
   backgroundCallable->setBackgroundContext(driContext->loaderPrivate);
}

static struct gl_memory_object *
brw_new_memoryobj(struct gl_context *ctx, GLuint name)
{
   struct brw_memory_object *memory_object = CALLOC_STRUCT(brw_memory_object);
   if (!memory_object)
      return NULL;

   _mesa_initialize_memory_object(ctx, &memory_object->Base, name);
   return &memory_object->Base;
}

static void
brw_delete_memoryobj(struct gl_context *ctx, struct gl_memory_object *memObj)
{
   struct brw_memory_object *memory_object = brw_memory_object(memObj);
   brw_bo_unreference(memory_object->bo);
   _mesa_delete_memory_object(ctx, memObj);
}

static void
brw_import_memoryobj_fd(struct gl_context *ctx,
                       struct gl_memory_object *obj,
                       GLuint64 size,
                       int fd)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_memory_object *memory_object = brw_memory_object(obj);

   memory_object->bo = brw_bo_gem_create_from_prime(brw->bufmgr, fd);
   brw_bo_reference(memory_object->bo);
   assert(memory_object->bo->size >= size);
   close(fd);
}

static void
brw_viewport(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);
   __DRIcontext *driContext = brw->driContext;

   if (_mesa_is_winsys_fbo(ctx->DrawBuffer)) {
      if (driContext->driDrawablePriv)
         dri2InvalidateDrawable(driContext->driDrawablePriv);
      if (driContext->driReadablePriv)
         dri2InvalidateDrawable(driContext->driReadablePriv);
   }
}

static void
brw_update_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb)
{
   struct brw_context *brw = brw_context(ctx);

   /* Quantize the derived default number of samples
    */
   fb->DefaultGeometry._NumSamples =
      brw_quantize_num_samples(brw->screen, fb->DefaultGeometry.NumSamples);
}

static void
brw_update_state(struct gl_context * ctx)
{
   GLuint new_state = ctx->NewState;
   struct brw_context *brw = brw_context(ctx);

   if (ctx->swrast_context)
      _swrast_InvalidateState(ctx, new_state);

   brw->NewGLState |= new_state;

   if (new_state & (_NEW_SCISSOR | _NEW_BUFFERS | _NEW_VIEWPORT))
      _mesa_update_draw_buffer_bounds(ctx, ctx->DrawBuffer);

   if (new_state & (_NEW_STENCIL | _NEW_BUFFERS)) {
      brw->stencil_enabled = _mesa_stencil_is_enabled(ctx);
      brw->stencil_two_sided = _mesa_stencil_is_two_sided(ctx);
      brw->stencil_write_enabled =
         _mesa_stencil_is_write_enabled(ctx, brw->stencil_two_sided);
   }

   if (new_state & _NEW_POLYGON)
      brw->polygon_front_bit = _mesa_polygon_get_front_bit(ctx);

   if (new_state & _NEW_BUFFERS) {
      brw_update_framebuffer(ctx, ctx->DrawBuffer);
      if (ctx->DrawBuffer != ctx->ReadBuffer)
         brw_update_framebuffer(ctx, ctx->ReadBuffer);
   }
}

#define flushFront(screen)      ((screen)->image.loader ? (screen)->image.loader->flushFrontBuffer : (screen)->dri2.loader->flushFrontBuffer)

static void
brw_flush_front(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);
   __DRIcontext *driContext = brw->driContext;
   __DRIdrawable *driDrawable = driContext->driDrawablePriv;
   __DRIscreen *const dri_screen = brw->screen->driScrnPriv;

   if (brw->front_buffer_dirty && ctx->DrawBuffer &&
       _mesa_is_winsys_fbo(ctx->DrawBuffer)) {
      if (flushFront(dri_screen) && driDrawable &&
          driDrawable->loaderPrivate) {

         /* Resolve before flushing FAKE_FRONT_LEFT to FRONT_LEFT.
          *
          * This potentially resolves both front and back buffer. It
          * is unnecessary to resolve the back, but harms nothing except
          * performance. And no one cares about front-buffer render
          * performance.
          */
         brw_resolve_for_dri2_flush(brw, driDrawable);
         brw_batch_flush(brw);

         flushFront(dri_screen)(driDrawable, driDrawable->loaderPrivate);

         /* We set the dirty bit in brw_prepare_render() if we're
          * front buffer rendering once we get there.
          */
         brw->front_buffer_dirty = false;
      }
   }
}

static void
brw_display_shared_buffer(struct brw_context *brw)
{
   __DRIcontext *dri_context = brw->driContext;
   __DRIdrawable *dri_drawable = dri_context->driDrawablePriv;
   __DRIscreen *dri_screen = brw->screen->driScrnPriv;
   int fence_fd = -1;

   if (!brw->is_shared_buffer_bound)
      return;

   if (!brw->is_shared_buffer_dirty)
      return;

   if (brw->screen->has_exec_fence) {
      /* This function is always called during a flush operation, so there is
       * no need to flush again here. But we want to provide a fence_fd to the
       * loader, and a redundant flush is the easiest way to acquire one.
       */
      if (brw_batch_flush_fence(brw, -1, &fence_fd))
         return;
   }

   dri_screen->mutableRenderBuffer.loader
      ->displaySharedBuffer(dri_drawable, fence_fd,
                            dri_drawable->loaderPrivate);
   brw->is_shared_buffer_dirty = false;
}

static void
brw_glFlush(struct gl_context *ctx, unsigned gallium_flush_flags)
{
   struct brw_context *brw = brw_context(ctx);

   brw_batch_flush(brw);
   brw_flush_front(ctx);
   brw_display_shared_buffer(brw);
   brw->need_flush_throttle = true;
}

static void
brw_glEnable(struct gl_context *ctx, GLenum cap, GLboolean state)
{
   struct brw_context *brw = brw_context(ctx);

   switch (cap) {
   case GL_BLACKHOLE_RENDER_INTEL:
      brw->frontend_noop = state;
      brw_batch_flush(brw);
      brw_batch_maybe_noop(brw);
      /* Because we started previous batches with a potential
       * MI_BATCH_BUFFER_END if NOOP was enabled, that means that anything
       * that was ever emitted after that never made it to the HW. So when the
       * blackhole state changes from NOOP->!NOOP reupload the entire state.
       */
      if (!brw->frontend_noop) {
         brw->NewGLState = ~0u;
         brw->ctx.NewDriverState = ~0ull;
      }
      break;
   default:
      break;
   }
}

static void
brw_finish(struct gl_context * ctx)
{
   struct brw_context *brw = brw_context(ctx);

   brw_glFlush(ctx, 0);

   if (brw->batch.last_bo)
      brw_bo_wait_rendering(brw->batch.last_bo);
}

static void
brw_get_device_uuid(struct gl_context *ctx, char *uuid)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_screen *screen = brw->screen;

   assert(GL_UUID_SIZE_EXT >= PIPE_UUID_SIZE);
   memset(uuid, 0, GL_UUID_SIZE_EXT);
   intel_uuid_compute_device_id((uint8_t *)uuid, &screen->isl_dev, PIPE_UUID_SIZE);
}


static void
brw_get_driver_uuid(struct gl_context *ctx, char *uuid)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_screen *screen = brw->screen;

   assert(GL_UUID_SIZE_EXT >= PIPE_UUID_SIZE);
   memset(uuid, 0, GL_UUID_SIZE_EXT);
   intel_uuid_compute_driver_id((uint8_t *)uuid, &screen->devinfo, PIPE_UUID_SIZE);
}

static void
brw_init_driver_functions(struct brw_context *brw,
                          struct dd_function_table *functions)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   _mesa_init_driver_functions(functions);

   /* GLX uses DRI2 invalidate events to handle window resizing.
    * Unfortunately, EGL does not - libEGL is written in XCB (not Xlib),
    * which doesn't provide a mechanism for snooping the event queues.
    *
    * So EGL still relies on viewport hacks to handle window resizing.
    * This should go away with DRI3000.
    */
   if (!brw->driContext->driScreenPriv->dri2.useInvalidate)
      functions->Viewport = brw_viewport;

   functions->Enable = brw_glEnable;
   functions->Flush = brw_glFlush;
   functions->Finish = brw_finish;
   functions->GetString = brw_get_string;
   functions->UpdateState = brw_update_state;

   brw_init_draw_functions(functions);
   brw_init_texture_functions(functions);
   brw_init_texture_image_functions(functions);
   brw_init_texture_copy_image_functions(functions);
   brw_init_copy_image_functions(functions);
   brw_init_clear_functions(functions);
   brw_init_buffer_functions(functions);
   brw_init_pixel_functions(functions);
   brw_init_buffer_object_functions(functions);
   brw_init_syncobj_functions(functions);
   brw_init_object_purgeable_functions(functions);

   brw_init_frag_prog_functions(functions);
   brw_init_common_queryobj_functions(functions);
   if (devinfo->verx10 >= 75)
      hsw_init_queryobj_functions(functions);
   else if (devinfo->ver >= 6)
      gfx6_init_queryobj_functions(functions);
   else
      gfx4_init_queryobj_functions(functions);
   brw_init_compute_functions(functions);
   brw_init_conditional_render_functions(functions);

   functions->GenerateMipmap = brw_generate_mipmap;

   functions->QueryInternalFormat = brw_query_internal_format;

   functions->NewTransformFeedback = brw_new_transform_feedback;
   functions->DeleteTransformFeedback = brw_delete_transform_feedback;
   if (can_do_mi_math_and_lrr(brw->screen)) {
      functions->BeginTransformFeedback = hsw_begin_transform_feedback;
      functions->EndTransformFeedback = hsw_end_transform_feedback;
      functions->PauseTransformFeedback = hsw_pause_transform_feedback;
      functions->ResumeTransformFeedback = hsw_resume_transform_feedback;
   } else if (devinfo->ver >= 7) {
      functions->BeginTransformFeedback = gfx7_begin_transform_feedback;
      functions->EndTransformFeedback = gfx7_end_transform_feedback;
      functions->PauseTransformFeedback = gfx7_pause_transform_feedback;
      functions->ResumeTransformFeedback = gfx7_resume_transform_feedback;
      functions->GetTransformFeedbackVertexCount =
         brw_get_transform_feedback_vertex_count;
   } else {
      functions->BeginTransformFeedback = brw_begin_transform_feedback;
      functions->EndTransformFeedback = brw_end_transform_feedback;
      functions->PauseTransformFeedback = brw_pause_transform_feedback;
      functions->ResumeTransformFeedback = brw_resume_transform_feedback;
      functions->GetTransformFeedbackVertexCount =
         brw_get_transform_feedback_vertex_count;
   }

   if (devinfo->ver >= 6)
      functions->GetSamplePosition = gfx6_get_sample_position;

   /* GL_ARB_get_program_binary */
   brw_program_binary_init(brw->screen->deviceID);
   functions->GetProgramBinaryDriverSHA1 = brw_get_program_binary_driver_sha1;
   functions->ProgramBinarySerializeDriverBlob = brw_serialize_program_binary;
   functions->ProgramBinaryDeserializeDriverBlob =
      brw_deserialize_program_binary;

   if (brw->screen->disk_cache) {
      functions->ShaderCacheSerializeDriverBlob = brw_program_serialize_nir;
   }

   functions->SetBackgroundContext = brw_set_background_context;

   functions->NewMemoryObject = brw_new_memoryobj;
   functions->DeleteMemoryObject = brw_delete_memoryobj;
   functions->ImportMemoryObjectFd = brw_import_memoryobj_fd;
   functions->GetDeviceUuid = brw_get_device_uuid;
   functions->GetDriverUuid = brw_get_driver_uuid;
}

static void
brw_initialize_spirv_supported_capabilities(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;

   /* The following SPIR-V capabilities are only supported on gfx7+. In theory
    * you should enable the extension only on gfx7+, but just in case let's
    * assert it.
    */
   assert(devinfo->ver >= 7);

   ctx->Const.SpirVCapabilities.atomic_storage = devinfo->ver >= 7;
   ctx->Const.SpirVCapabilities.draw_parameters = true;
   ctx->Const.SpirVCapabilities.float64 = devinfo->ver >= 8;
   ctx->Const.SpirVCapabilities.geometry_streams = devinfo->ver >= 7;
   ctx->Const.SpirVCapabilities.image_write_without_format = true;
   ctx->Const.SpirVCapabilities.int64 = devinfo->ver >= 8;
   ctx->Const.SpirVCapabilities.tessellation = true;
   ctx->Const.SpirVCapabilities.transform_feedback = devinfo->ver >= 7;
   ctx->Const.SpirVCapabilities.variable_pointers = true;
   ctx->Const.SpirVCapabilities.integer_functions2 = devinfo->ver >= 8;
}

static void
brw_initialize_context_constants(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;
   const struct brw_compiler *compiler = brw->screen->compiler;

   const bool stage_exists[MESA_SHADER_STAGES] = {
      [MESA_SHADER_VERTEX] = true,
      [MESA_SHADER_TESS_CTRL] = devinfo->ver >= 7,
      [MESA_SHADER_TESS_EVAL] = devinfo->ver >= 7,
      [MESA_SHADER_GEOMETRY] = devinfo->ver >= 6,
      [MESA_SHADER_FRAGMENT] = true,
      [MESA_SHADER_COMPUTE] =
         (_mesa_is_desktop_gl(ctx) &&
          ctx->Const.MaxComputeWorkGroupSize[0] >= 1024) ||
         (ctx->API == API_OPENGLES2 &&
          ctx->Const.MaxComputeWorkGroupSize[0] >= 128),
   };

   unsigned num_stages = 0;
   for (int i = 0; i < MESA_SHADER_STAGES; i++) {
      if (stage_exists[i])
         num_stages++;
   }

   unsigned max_samplers =
      devinfo->verx10 >= 75 ? BRW_MAX_TEX_UNIT : 16;

   ctx->Const.MaxDualSourceDrawBuffers = 1;
   ctx->Const.MaxDrawBuffers = BRW_MAX_DRAW_BUFFERS;
   ctx->Const.MaxCombinedShaderOutputResources =
      MAX_IMAGE_UNITS + BRW_MAX_DRAW_BUFFERS;

   /* The timestamp register we can read for glGetTimestamp() is
    * sometimes only 32 bits, before scaling to nanoseconds (depending
    * on kernel).
    *
    * Once scaled to nanoseconds the timestamp would roll over at a
    * non-power-of-two, so an application couldn't use
    * GL_QUERY_COUNTER_BITS to handle rollover correctly.  Instead, we
    * report 36 bits and truncate at that (rolling over 5 times as
    * often as the HW counter), and when the 32-bit counter rolls
    * over, it happens to also be at a rollover in the reported value
    * from near (1<<36) to 0.
    *
    * The low 32 bits rolls over in ~343 seconds.  Our 36-bit result
    * rolls over every ~69 seconds.
    */
   ctx->Const.QueryCounterBits.Timestamp = 36;

   ctx->Const.MaxTextureCoordUnits = 8; /* Mesa limit */
   ctx->Const.MaxImageUnits = MAX_IMAGE_UNITS;
   if (devinfo->ver >= 7) {
      ctx->Const.MaxRenderbufferSize = 16384;
      ctx->Const.MaxTextureSize = 16384;
      ctx->Const.MaxCubeTextureLevels = 15; /* 16384 */
   } else {
      ctx->Const.MaxRenderbufferSize = 8192;
      ctx->Const.MaxTextureSize = 8192;
      ctx->Const.MaxCubeTextureLevels = 14; /* 8192 */
   }
   ctx->Const.Max3DTextureLevels = 12; /* 2048 */
   ctx->Const.MaxArrayTextureLayers = devinfo->ver >= 7 ? 2048 : 512;
   ctx->Const.MaxTextureMbytes = 1536;
   ctx->Const.MaxTextureRectSize = devinfo->ver >= 7 ? 16384 : 8192;
   ctx->Const.MaxTextureMaxAnisotropy = 16.0;
   ctx->Const.MaxTextureLodBias = 15.0;
   ctx->Const.StripTextureBorder = true;
   if (devinfo->ver >= 7) {
      ctx->Const.MaxProgramTextureGatherComponents = 4;
      ctx->Const.MinProgramTextureGatherOffset = -32;
      ctx->Const.MaxProgramTextureGatherOffset = 31;
   } else if (devinfo->ver == 6) {
      ctx->Const.MaxProgramTextureGatherComponents = 1;
      ctx->Const.MinProgramTextureGatherOffset = -8;
      ctx->Const.MaxProgramTextureGatherOffset = 7;
   }

   ctx->Const.MaxUniformBlockSize = 65536;

   for (int i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_program_constants *prog = &ctx->Const.Program[i];

      if (!stage_exists[i])
         continue;

      prog->MaxTextureImageUnits = max_samplers;

      prog->MaxUniformBlocks = BRW_MAX_UBO;
      prog->MaxCombinedUniformComponents =
         prog->MaxUniformComponents +
         ctx->Const.MaxUniformBlockSize / 4 * prog->MaxUniformBlocks;

      prog->MaxAtomicCounters = MAX_ATOMIC_COUNTERS;
      prog->MaxAtomicBuffers = BRW_MAX_ABO;
      prog->MaxImageUniforms = compiler->scalar_stage[i] ? BRW_MAX_IMAGES : 0;
      prog->MaxShaderStorageBlocks = BRW_MAX_SSBO;
   }

   ctx->Const.MaxTextureUnits =
      MIN2(ctx->Const.MaxTextureCoordUnits,
           ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits);

   ctx->Const.MaxUniformBufferBindings = num_stages * BRW_MAX_UBO;
   ctx->Const.MaxCombinedUniformBlocks = num_stages * BRW_MAX_UBO;
   ctx->Const.MaxCombinedAtomicBuffers = num_stages * BRW_MAX_ABO;
   ctx->Const.MaxCombinedShaderStorageBlocks = num_stages * BRW_MAX_SSBO;
   ctx->Const.MaxShaderStorageBufferBindings = num_stages * BRW_MAX_SSBO;
   ctx->Const.MaxCombinedTextureImageUnits = num_stages * max_samplers;
   ctx->Const.MaxCombinedImageUniforms = num_stages * BRW_MAX_IMAGES;


   /* Hardware only supports a limited number of transform feedback buffers.
    * So we need to override the Mesa default (which is based only on software
    * limits).
    */
   ctx->Const.MaxTransformFeedbackBuffers = BRW_MAX_SOL_BUFFERS;

   /* On Gfx6, in the worst case, we use up one binding table entry per
    * transform feedback component (see comments above the definition of
    * BRW_MAX_SOL_BINDINGS, in brw_context.h), so we need to advertise a value
    * for MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS equal to
    * BRW_MAX_SOL_BINDINGS.
    *
    * In "separate components" mode, we need to divide this value by
    * BRW_MAX_SOL_BUFFERS, so that the total number of binding table entries
    * used up by all buffers will not exceed BRW_MAX_SOL_BINDINGS.
    */
   ctx->Const.MaxTransformFeedbackInterleavedComponents = BRW_MAX_SOL_BINDINGS;
   ctx->Const.MaxTransformFeedbackSeparateComponents =
      BRW_MAX_SOL_BINDINGS / BRW_MAX_SOL_BUFFERS;

   ctx->Const.AlwaysUseGetTransformFeedbackVertexCount =
      !can_do_mi_math_and_lrr(brw->screen);

   int max_samples;
   const int *msaa_modes = brw_supported_msaa_modes(brw->screen);
   const int clamp_max_samples =
      driQueryOptioni(&brw->screen->optionCache, "clamp_max_samples");

   if (clamp_max_samples < 0) {
      max_samples = msaa_modes[0];
   } else {
      /* Select the largest supported MSAA mode that does not exceed
       * clamp_max_samples.
       */
      max_samples = 0;
      for (int i = 0; msaa_modes[i] != 0; ++i) {
         if (msaa_modes[i] <= clamp_max_samples) {
            max_samples = msaa_modes[i];
            break;
         }
      }
   }

   ctx->Const.MaxSamples = max_samples;
   ctx->Const.MaxColorTextureSamples = max_samples;
   ctx->Const.MaxDepthTextureSamples = max_samples;
   ctx->Const.MaxIntegerSamples = max_samples;
   ctx->Const.MaxImageSamples = 0;

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   if (devinfo->ver >= 6) {
      ctx->Const.MaxLineWidth = 7.375;
      ctx->Const.MaxLineWidthAA = 7.375;
      ctx->Const.LineWidthGranularity = 0.125;
   } else {
      ctx->Const.MaxLineWidth = 7.0;
      ctx->Const.MaxLineWidthAA = 7.0;
      ctx->Const.LineWidthGranularity = 0.5;
   }

   /* For non-antialiased lines, we have to round the line width to the
    * nearest whole number. Make sure that we don't advertise a line
    * width that, when rounded, will be beyond the actual hardware
    * maximum.
    */
   assert(roundf(ctx->Const.MaxLineWidth) <= ctx->Const.MaxLineWidth);

   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 255.0;
   ctx->Const.MaxPointSizeAA = 255.0;
   ctx->Const.PointSizeGranularity = 1.0;

   if (devinfo->ver >= 5 || devinfo->is_g4x)
      ctx->Const.MaxClipPlanes = 8;

   ctx->Const.GLSLFragCoordIsSysVal = true;
   ctx->Const.GLSLFrontFacingIsSysVal = true;
   ctx->Const.GLSLTessLevelsAsInputs = true;
   ctx->Const.PrimitiveRestartForPatches = true;

   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeInstructions = 16 * 1024;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxAluInstructions = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTexInstructions = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTexIndirections = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeAluInstructions = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeTexInstructions = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeTexIndirections = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeAttribs = 16;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeTemps = 256;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeAddressRegs = 1;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeParameters = 1024;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxEnvParams =
      MIN2(ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeParameters,
           ctx->Const.Program[MESA_SHADER_VERTEX].MaxEnvParams);

   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeInstructions = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeAluInstructions = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeTexInstructions = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeTexIndirections = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeAttribs = 12;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeTemps = 256;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeAddressRegs = 0;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeParameters = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxEnvParams =
      MIN2(ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeParameters,
           ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxEnvParams);

   /* Fragment shaders use real, 32-bit twos-complement integers for all
    * integer types.
    */
   ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt.RangeMin = 31;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt.RangeMax = 30;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt.Precision = 0;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].HighInt = ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MediumInt = ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt;

   ctx->Const.Program[MESA_SHADER_VERTEX].LowInt.RangeMin = 31;
   ctx->Const.Program[MESA_SHADER_VERTEX].LowInt.RangeMax = 30;
   ctx->Const.Program[MESA_SHADER_VERTEX].LowInt.Precision = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].HighInt = ctx->Const.Program[MESA_SHADER_VERTEX].LowInt;
   ctx->Const.Program[MESA_SHADER_VERTEX].MediumInt = ctx->Const.Program[MESA_SHADER_VERTEX].LowInt;

   /* Gfx6 converts quads to polygon in beginning of 3D pipeline,
    * but we're not sure how it's actually done for vertex order,
    * that affect provoking vertex decision. Always use last vertex
    * convention for quad primitive which works as expected for now.
    */
   if (devinfo->ver >= 6)
      ctx->Const.QuadsFollowProvokingVertexConvention = false;

   ctx->Const.NativeIntegers = true;

   /* Regarding the CMP instruction, the Ivybridge PRM says:
    *
    *   "For each enabled channel 0b or 1b is assigned to the appropriate flag
    *    bit and 0/all zeros or all ones (e.g, byte 0xFF, word 0xFFFF, DWord
    *    0xFFFFFFFF) is assigned to dst."
    *
    * but PRMs for earlier generations say
    *
    *   "In dword format, one GRF may store up to 8 results. When the register
    *    is used later as a vector of Booleans, as only LSB at each channel
    *    contains meaning [sic] data, software should make sure all higher bits
    *    are masked out (e.g. by 'and-ing' an [sic] 0x01 constant)."
    *
    * We select the representation of a true boolean uniform to be ~0, and fix
    * the results of Gen <= 5 CMP instruction's with -(result & 1).
    */
   ctx->Const.UniformBooleanTrue = ~0;

   /* From the gfx4 PRM, volume 4 page 127:
    *
    *     "For SURFTYPE_BUFFER non-rendertarget surfaces, this field specifies
    *      the base address of the first element of the surface, computed in
    *      software by adding the surface base address to the byte offset of
    *      the element in the buffer."
    *
    * However, unaligned accesses are slower, so enforce buffer alignment.
    *
    * In order to push UBO data, 3DSTATE_CONSTANT_XS imposes an additional
    * restriction: the start of the buffer needs to be 32B aligned.
    */
   ctx->Const.UniformBufferOffsetAlignment = 32;

   /* ShaderStorageBufferOffsetAlignment should be a cacheline (64 bytes) so
    * that we can safely have the CPU and GPU writing the same SSBO on
    * non-cachecoherent systems (our Atom CPUs). With UBOs, the GPU never
    * writes, so there's no problem. For an SSBO, the GPU and the CPU can
    * be updating disjoint regions of the buffer simultaneously and that will
    * break if the regions overlap the same cacheline.
    */
   ctx->Const.ShaderStorageBufferOffsetAlignment = 64;
   ctx->Const.TextureBufferOffsetAlignment = 16;
   ctx->Const.MaxTextureBufferSize = 128 * 1024 * 1024;

   if (devinfo->ver >= 6) {
      ctx->Const.MaxVarying = 32;
      ctx->Const.Program[MESA_SHADER_VERTEX].MaxOutputComponents = 128;
      ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxInputComponents =
         compiler->scalar_stage[MESA_SHADER_GEOMETRY] ? 128 : 64;
      ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxOutputComponents = 128;
      ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxInputComponents = 128;
      ctx->Const.Program[MESA_SHADER_TESS_CTRL].MaxInputComponents = 128;
      ctx->Const.Program[MESA_SHADER_TESS_CTRL].MaxOutputComponents = 128;
      ctx->Const.Program[MESA_SHADER_TESS_EVAL].MaxInputComponents = 128;
      ctx->Const.Program[MESA_SHADER_TESS_EVAL].MaxOutputComponents = 128;
   }

   /* We want the GLSL compiler to emit code that uses condition codes */
   for (int i = 0; i < MESA_SHADER_STAGES; i++) {
      ctx->Const.ShaderCompilerOptions[i] =
         brw->screen->compiler->glsl_compiler_options[i];
   }

   if (devinfo->ver >= 7) {
      ctx->Const.MaxViewportWidth = 32768;
      ctx->Const.MaxViewportHeight = 32768;
   }

   /* ARB_viewport_array, OES_viewport_array */
   if (devinfo->ver >= 6) {
      ctx->Const.MaxViewports = GFX6_NUM_VIEWPORTS;
      ctx->Const.ViewportSubpixelBits = 8;

      /* Cast to float before negating because MaxViewportWidth is unsigned.
       */
      ctx->Const.ViewportBounds.Min = -(float)ctx->Const.MaxViewportWidth;
      ctx->Const.ViewportBounds.Max = ctx->Const.MaxViewportWidth;
   }

   /* ARB_gpu_shader5 */
   if (devinfo->ver >= 7)
      ctx->Const.MaxVertexStreams = MIN2(4, MAX_VERTEX_STREAMS);

   /* ARB_framebuffer_no_attachments */
   ctx->Const.MaxFramebufferWidth = 16384;
   ctx->Const.MaxFramebufferHeight = 16384;
   ctx->Const.MaxFramebufferLayers = ctx->Const.MaxArrayTextureLayers;
   ctx->Const.MaxFramebufferSamples = max_samples;

   /* OES_primitive_bounding_box */
   ctx->Const.NoPrimitiveBoundingBoxOutput = true;

   /* TODO: We should be able to use STD430 packing by default on all hardware
    * but some piglit tests [1] currently fail on SNB when this is enabled.
    * The problem is the messages we're using for doing uniform pulls
    * in the vec4 back-end on SNB is the OWORD block load instruction, which
    * takes its offset in units of OWORDS (16 bytes).  On IVB+, we use the
    * sampler which doesn't have these restrictions.
    *
    * In the scalar back-end, we use the sampler for dynamic uniform loads and
    * pull an entire cache line at a time for constant offset loads both of
    * which support almost any alignment.
    *
    * [1] glsl-1.40/uniform_buffer/vs-float-array-variable-index.shader_test
    */
   if (devinfo->ver >= 7)
      ctx->Const.UseSTD430AsDefaultPacking = true;

   if (!(ctx->Const.ContextFlags & GL_CONTEXT_FLAG_DEBUG_BIT))
      ctx->Const.AllowMappedBuffersDuringExecution = true;

   /* GL_ARB_get_program_binary */
   ctx->Const.NumProgramBinaryFormats = 1;
}

static void
brw_initialize_cs_context_constants(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   struct intel_device_info *devinfo = &brw->screen->devinfo;

   /* Maximum number of scalar compute shader invocations that can be run in
    * parallel in the same subslice assuming SIMD32 dispatch.
    */
   const unsigned max_threads = devinfo->max_cs_workgroup_threads;
   const uint32_t max_invocations = 32 * max_threads;
   ctx->Const.MaxComputeWorkGroupSize[0] = max_invocations;
   ctx->Const.MaxComputeWorkGroupSize[1] = max_invocations;
   ctx->Const.MaxComputeWorkGroupSize[2] = max_invocations;
   ctx->Const.MaxComputeWorkGroupInvocations = max_invocations;
   ctx->Const.MaxComputeSharedMemorySize = 64 * 1024;

   /* Constants used for ARB_compute_variable_group_size. */
   if (devinfo->ver >= 7) {
      assert(max_invocations >= 512);
      ctx->Const.MaxComputeVariableGroupSize[0] = max_invocations;
      ctx->Const.MaxComputeVariableGroupSize[1] = max_invocations;
      ctx->Const.MaxComputeVariableGroupSize[2] = max_invocations;
      ctx->Const.MaxComputeVariableGroupInvocations = max_invocations;
   }
}

/**
 * Process driconf (drirc) options, setting appropriate context flags.
 *
 * brw_init_extensions still pokes at optionCache directly, in order to
 * avoid advertising various extensions.  No flags are set, so it makes
 * sense to continue doing that there.
 */
static void
brw_process_driconf_options(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;
   const driOptionCache *const options = &brw->screen->optionCache;

   if (INTEL_DEBUG(DEBUG_NO_HIZ)) {
       brw->has_hiz = false;
       /* On gfx6, you can only do separate stencil with HIZ. */
       if (devinfo->ver == 6)
          brw->has_separate_stencil = false;
   }

   if (driQueryOptionb(options, "mesa_no_error"))
      ctx->Const.ContextFlags |= GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR;

   if (driQueryOptionb(options, "always_flush_batch")) {
      fprintf(stderr, "flushing batchbuffer before/after each draw call\n");
      brw->always_flush_batch = true;
   }

   if (driQueryOptionb(options, "always_flush_cache")) {
      fprintf(stderr, "flushing GPU caches before/after each draw call\n");
      brw->always_flush_cache = true;
   }

   if (driQueryOptionb(options, "disable_throttling")) {
      fprintf(stderr, "disabling flush throttling\n");
      brw->disable_throttling = true;
   }

   brw->precompile = driQueryOptionb(&brw->screen->optionCache, "shader_precompile");

   if (driQueryOptionb(&brw->screen->optionCache, "precise_trig"))
      brw->screen->compiler->precise_trig = true;

   ctx->Const.ForceGLSLExtensionsWarn =
      driQueryOptionb(options, "force_glsl_extensions_warn");

   ctx->Const.ForceGLSLVersion =
      driQueryOptioni(options, "force_glsl_version");

   ctx->Const.DisableGLSLLineContinuations =
      driQueryOptionb(options, "disable_glsl_line_continuations");

   ctx->Const.AllowGLSLExtensionDirectiveMidShader =
      driQueryOptionb(options, "allow_glsl_extension_directive_midshader");

   ctx->Const.AllowGLSLBuiltinVariableRedeclaration =
      driQueryOptionb(options, "allow_glsl_builtin_variable_redeclaration");

   ctx->Const.AllowHigherCompatVersion =
      driQueryOptionb(options, "allow_higher_compat_version");

   ctx->Const.ForceGLSLAbsSqrt =
      driQueryOptionb(options, "force_glsl_abs_sqrt");

   ctx->Const.GLSLZeroInit = driQueryOptionb(options, "glsl_zero_init") ? 1 : 0;

   brw->dual_color_blend_by_location =
      driQueryOptionb(options, "dual_color_blend_by_location");

   ctx->Const.AllowGLSLCrossStageInterpolationMismatch =
      driQueryOptionb(options, "allow_glsl_cross_stage_interpolation_mismatch");

   char *vendor_str = driQueryOptionstr(options, "force_gl_vendor");
   /* not an empty string */
   if (*vendor_str)
      ctx->Const.VendorOverride = vendor_str;

   ctx->Const.dri_config_options_sha1 =
      ralloc_array(brw->mem_ctx, unsigned char, 20);
   driComputeOptionsSha1(&brw->screen->optionCache,
                         ctx->Const.dri_config_options_sha1);
}

GLboolean
brw_create_context(gl_api api,
                   const struct gl_config *mesaVis,
                   __DRIcontext *driContextPriv,
                   const struct __DriverContextConfig *ctx_config,
                   unsigned *dri_ctx_error,
                   void *sharedContextPrivate)
{
   struct gl_context *shareCtx = (struct gl_context *) sharedContextPrivate;
   struct brw_screen *screen = driContextPriv->driScreenPriv->driverPrivate;
   const struct intel_device_info *devinfo = &screen->devinfo;
   struct dd_function_table functions;

   /* Only allow the __DRI_CTX_FLAG_ROBUST_BUFFER_ACCESS flag if the kernel
    * provides us with context reset notifications.
    */
   uint32_t allowed_flags = __DRI_CTX_FLAG_DEBUG |
                            __DRI_CTX_FLAG_FORWARD_COMPATIBLE |
                            __DRI_CTX_FLAG_NO_ERROR;

   if (screen->has_context_reset_notification)
      allowed_flags |= __DRI_CTX_FLAG_ROBUST_BUFFER_ACCESS;

   if (ctx_config->flags & ~allowed_flags) {
      *dri_ctx_error = __DRI_CTX_ERROR_UNKNOWN_FLAG;
      return false;
   }

   if (ctx_config->attribute_mask &
       ~(__DRIVER_CONTEXT_ATTRIB_RESET_STRATEGY |
         __DRIVER_CONTEXT_ATTRIB_PRIORITY)) {
      *dri_ctx_error = __DRI_CTX_ERROR_UNKNOWN_ATTRIBUTE;
      return false;
   }

   bool notify_reset =
      ((ctx_config->attribute_mask & __DRIVER_CONTEXT_ATTRIB_RESET_STRATEGY) &&
       ctx_config->reset_strategy != __DRI_CTX_RESET_NO_NOTIFICATION);

   struct brw_context *brw = align_calloc(sizeof(struct brw_context), 16);
   if (!brw) {
      fprintf(stderr, "%s: failed to alloc context\n", __func__);
      *dri_ctx_error = __DRI_CTX_ERROR_NO_MEMORY;
      return false;
   }
   brw->mem_ctx = ralloc_context(NULL);
   brw->perf_ctx = intel_perf_new_context(brw->mem_ctx);

   driContextPriv->driverPrivate = brw;
   brw->driContext = driContextPriv;
   brw->screen = screen;
   brw->bufmgr = screen->bufmgr;

   brw->has_hiz = devinfo->has_hiz_and_separate_stencil;
   brw->has_separate_stencil = devinfo->has_hiz_and_separate_stencil;

   brw->has_swizzling = screen->hw_has_swizzling;

   /* We don't push UBOs on IVB and earlier because the restrictions on
    * 3DSTATE_CONSTANT_* make it really annoying to use push constants
    * without dynamic state base address.
    */
   brw->can_push_ubos = devinfo->verx10 >= 75;

   brw->isl_dev = screen->isl_dev;

   brw->vs.base.stage = MESA_SHADER_VERTEX;
   brw->tcs.base.stage = MESA_SHADER_TESS_CTRL;
   brw->tes.base.stage = MESA_SHADER_TESS_EVAL;
   brw->gs.base.stage = MESA_SHADER_GEOMETRY;
   brw->wm.base.stage = MESA_SHADER_FRAGMENT;
   brw->cs.base.stage = MESA_SHADER_COMPUTE;

   brw_init_driver_functions(brw, &functions);

   if (notify_reset)
      functions.GetGraphicsResetStatus = brw_get_graphics_reset_status;

   brw_process_driconf_options(brw);

   if (api == API_OPENGL_CORE &&
       driQueryOptionb(&screen->optionCache, "force_compat_profile")) {
      api = API_OPENGL_COMPAT;
   }

   struct gl_context *ctx = &brw->ctx;

   if (!_mesa_initialize_context(ctx, api, mesaVis, shareCtx, &functions)) {
      *dri_ctx_error = __DRI_CTX_ERROR_NO_MEMORY;
      fprintf(stderr, "%s: failed to init mesa context\n", __func__);
      brw_destroy_context(driContextPriv);
      return false;
   }

   driContextSetFlags(ctx, ctx_config->flags);

   /* Initialize the software rasterizer and helper modules.
    *
    * As of GL 3.1 core, the gfx4+ driver doesn't need the swrast context for
    * software fallbacks (which we have to support on legacy GL to do weird
    * glDrawPixels(), glBitmap(), and other functions).
    */
   if (api != API_OPENGL_CORE && api != API_OPENGLES2) {
      _swrast_CreateContext(ctx);
   }

   _vbo_CreateContext(ctx, true);
   if (ctx->swrast_context) {
      _tnl_CreateContext(ctx);
      TNL_CONTEXT(ctx)->Driver.RunPipeline = _tnl_run_pipeline;
      _swsetup_CreateContext(ctx);

      /* Configure swrast to match hardware characteristics: */
      _swrast_allow_pixel_fog(ctx, false);
      _swrast_allow_vertex_fog(ctx, true);
   }

   _mesa_meta_init(ctx);

   if (INTEL_DEBUG(DEBUG_PERF))
      brw->perf_debug = true;

   brw_initialize_cs_context_constants(brw);
   brw_initialize_context_constants(brw);

   ctx->Const.ResetStrategy = notify_reset
      ? GL_LOSE_CONTEXT_ON_RESET_ARB : GL_NO_RESET_NOTIFICATION_ARB;

   /* Reinitialize the context point state.  It depends on ctx->Const values. */
   _mesa_init_point(ctx);

   brw_fbo_init(brw);

   brw_batch_init(brw);

   /* Create a new hardware context.  Using a hardware context means that
    * our GPU state will be saved/restored on context switch, allowing us
    * to assume that the GPU is in the same state we left it in.
    *
    * This is required for transform feedback buffer offsets, query objects,
    * and also allows us to reduce how much state we have to emit.
    */
   brw->hw_ctx = brw_create_hw_context(brw->bufmgr);
   if (!brw->hw_ctx && devinfo->ver >= 6) {
      fprintf(stderr, "Failed to create hardware context.\n");
      brw_destroy_context(driContextPriv);
      return false;
   }

   if (brw->hw_ctx) {
      int hw_priority = INTEL_CONTEXT_MEDIUM_PRIORITY;
      if (ctx_config->attribute_mask & __DRIVER_CONTEXT_ATTRIB_PRIORITY) {
         switch (ctx_config->priority) {
         case __DRI_CTX_PRIORITY_LOW:
            hw_priority = INTEL_CONTEXT_LOW_PRIORITY;
            break;
         case __DRI_CTX_PRIORITY_HIGH:
            hw_priority = INTEL_CONTEXT_HIGH_PRIORITY;
            break;
         }
      }
      if (hw_priority != I915_CONTEXT_DEFAULT_PRIORITY &&
          brw_hw_context_set_priority(brw->bufmgr, brw->hw_ctx, hw_priority)) {
         fprintf(stderr,
                 "Failed to set priority [%d:%d] for hardware context.\n",
                 ctx_config->priority, hw_priority);
         brw_destroy_context(driContextPriv);
         return false;
      }
   }

   if (brw_init_pipe_control(brw, devinfo)) {
      *dri_ctx_error = __DRI_CTX_ERROR_NO_MEMORY;
      brw_destroy_context(driContextPriv);
      return false;
   }

   brw_upload_init(&brw->upload, brw->bufmgr, 65536);

   brw_init_state(brw);

   brw_init_extensions(ctx);

   brw_init_surface_formats(brw);

   brw_blorp_init(brw);

   brw->urb.size = devinfo->urb.size;

   if (devinfo->ver == 6)
      brw->urb.gs_present = false;

   brw->prim_restart.in_progress = false;
   brw->prim_restart.enable_cut_index = false;
   brw->gs.enabled = false;
   brw->clip.viewport_count = 1;

   brw->predicate.state = BRW_PREDICATE_STATE_RENDER;

   brw->max_gtt_map_object_size = screen->max_gtt_map_object_size;

   ctx->VertexProgram._MaintainTnlProgram = true;
   ctx->FragmentProgram._MaintainTexEnvProgram = true;
   _mesa_reset_vertex_processing_mode(ctx);

   brw_draw_init( brw );

   if ((ctx_config->flags & __DRI_CTX_FLAG_DEBUG) != 0) {
      /* Turn on some extra GL_ARB_debug_output generation. */
      brw->perf_debug = true;
   }

   if ((ctx_config->flags & __DRI_CTX_FLAG_ROBUST_BUFFER_ACCESS) != 0) {
      ctx->Const.ContextFlags |= GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB;
      ctx->Const.RobustAccess = GL_TRUE;
   }

   if (INTEL_DEBUG(DEBUG_SHADER_TIME))
      brw_init_shader_time(brw);

   _mesa_override_extensions(ctx);
   _mesa_compute_version(ctx);

#ifndef NDEBUG
   /* Enforce that the version of the context that was created is at least as
    * high as the version that was advertised via GLX / EGL / whatever window
    * system.
    */
   const __DRIscreen *const dri_screen = brw->screen->driScrnPriv;

   switch (api) {
   case API_OPENGL_COMPAT:
      assert(ctx->Version >= dri_screen->max_gl_compat_version);
      break;
   case API_OPENGLES:
      assert(ctx->Version >= dri_screen->max_gl_es1_version);
      break;
   case API_OPENGLES2:
      assert(ctx->Version >= dri_screen->max_gl_es2_version);
      break;
   case API_OPENGL_CORE:
      assert(ctx->Version >= dri_screen->max_gl_core_version);
      break;
   }
#endif

   /* GL_ARB_gl_spirv */
   if (ctx->Extensions.ARB_gl_spirv) {
      brw_initialize_spirv_supported_capabilities(brw);

      if (ctx->Extensions.ARB_spirv_extensions) {
         /* GL_ARB_spirv_extensions */
         ctx->Const.SpirVExtensions = MALLOC_STRUCT(spirv_supported_extensions);
         _mesa_fill_supported_spirv_extensions(ctx->Const.SpirVExtensions,
                                               &ctx->Const.SpirVCapabilities);
      }
   }

   _mesa_initialize_dispatch_tables(ctx);
   _mesa_initialize_vbo_vtxfmt(ctx);

   if (ctx->Extensions.INTEL_performance_query)
      brw_init_performance_queries(brw);

   brw->ctx.Cache = brw->screen->disk_cache;

   if (driContextPriv->driScreenPriv->dri2.backgroundCallable &&
       driQueryOptionb(&screen->optionCache, "mesa_glthread")) {
      /* Loader supports multithreading, and so do we. */
      _mesa_glthread_init(ctx);
   }

   return true;
}

void
brw_destroy_context(__DRIcontext *driContextPriv)
{
   struct brw_context *brw =
      (struct brw_context *) driContextPriv->driverPrivate;
   struct gl_context *ctx = &brw->ctx;

   GET_CURRENT_CONTEXT(curctx);

   if (curctx == NULL) {
      /* No current context, but we need one to release
       * renderbuffer surface when we release framebuffer.
       * So temporarily bind the context.
       */
      _mesa_make_current(ctx, NULL, NULL);
   }

   _mesa_glthread_destroy(&brw->ctx);

   _mesa_meta_free(&brw->ctx);

   if (INTEL_DEBUG(DEBUG_SHADER_TIME)) {
      /* Force a report. */
      brw->shader_time.report_time = 0;

      brw_collect_and_report_shader_time(brw);
      brw_destroy_shader_time(brw);
   }

   blorp_finish(&brw->blorp);

   brw_destroy_state(brw);
   brw_draw_destroy(brw);

   brw_bo_unreference(brw->curbe.curbe_bo);

   brw_bo_unreference(brw->vs.base.scratch_bo);
   brw_bo_unreference(brw->tcs.base.scratch_bo);
   brw_bo_unreference(brw->tes.base.scratch_bo);
   brw_bo_unreference(brw->gs.base.scratch_bo);
   brw_bo_unreference(brw->wm.base.scratch_bo);

   brw_bo_unreference(brw->vs.base.push_const_bo);
   brw_bo_unreference(brw->tcs.base.push_const_bo);
   brw_bo_unreference(brw->tes.base.push_const_bo);
   brw_bo_unreference(brw->gs.base.push_const_bo);
   brw_bo_unreference(brw->wm.base.push_const_bo);

   brw_destroy_hw_context(brw->bufmgr, brw->hw_ctx);

   if (ctx->swrast_context) {
      _swsetup_DestroyContext(&brw->ctx);
      _tnl_DestroyContext(&brw->ctx);
   }
   _vbo_DestroyContext(&brw->ctx);

   if (ctx->swrast_context)
      _swrast_DestroyContext(&brw->ctx);

   brw_fini_pipe_control(brw);
   brw_batch_free(&brw->batch);

   brw_bo_unreference(brw->throttle_batch[1]);
   brw_bo_unreference(brw->throttle_batch[0]);
   brw->throttle_batch[1] = NULL;
   brw->throttle_batch[0] = NULL;

   /* free the Mesa context */
   _mesa_free_context_data(&brw->ctx, true);

   ralloc_free(brw->mem_ctx);
   align_free(brw);
   driContextPriv->driverPrivate = NULL;
}

GLboolean
brw_unbind_context(__DRIcontext *driContextPriv)
{
   struct gl_context *ctx = driContextPriv->driverPrivate;
   _mesa_glthread_finish(ctx);

   /* Unset current context and dispath table */
   _mesa_make_current(NULL, NULL, NULL);

   return true;
}

/**
 * Fixes up the context for GLES23 with our default-to-sRGB-capable behavior
 * on window system framebuffers.
 *
 * Desktop GL is fairly reasonable in its handling of sRGB: You can ask if
 * your renderbuffer can do sRGB encode, and you can flip a switch that does
 * sRGB encode if the renderbuffer can handle it.  You can ask specifically
 * for a visual where you're guaranteed to be capable, but it turns out that
 * everyone just makes all their ARGB8888 visuals capable and doesn't offer
 * incapable ones, because there's no difference between the two in resources
 * used.  Applications thus get built that accidentally rely on the default
 * visual choice being sRGB, so we make ours sRGB capable.  Everything sounds
 * great...
 *
 * But for GLES2/3, they decided that it was silly to not turn on sRGB encode
 * for sRGB renderbuffers you made with the GL_EXT_texture_sRGB equivalent.
 * So they removed the enable knob and made it "if the renderbuffer is sRGB
 * capable, do sRGB encode".  Then, for your window system renderbuffers, you
 * can ask for sRGB visuals and get sRGB encode, or not ask for sRGB visuals
 * and get no sRGB encode (assuming that both kinds of visual are available).
 * Thus our choice to support sRGB by default on our visuals for desktop would
 * result in broken rendering of GLES apps that aren't expecting sRGB encode.
 *
 * Unfortunately, renderbuffer setup happens before a context is created.  So
 * in brw_screen.c we always set up sRGB, and here, if you're a GLES2/3
 * context (without an sRGB visual), we go turn that back off before anyone
 * finds out.
 */
static void
brw_gles3_srgb_workaround(struct brw_context *brw, struct gl_framebuffer *fb)
{
   struct gl_context *ctx = &brw->ctx;

   if (_mesa_is_desktop_gl(ctx) || !fb->Visual.sRGBCapable)
      return;

   for (int i = 0; i < BUFFER_COUNT; i++) {
      struct gl_renderbuffer *rb = fb->Attachment[i].Renderbuffer;

      /* Check if sRGB was specifically asked for. */
      struct brw_renderbuffer *irb = brw_get_renderbuffer(fb, i);
      if (irb && irb->need_srgb)
         return;

      if (rb)
         rb->Format = _mesa_get_srgb_format_linear(rb->Format);
   }
   /* Disable sRGB from framebuffers that are not compatible. */
   fb->Visual.sRGBCapable = false;
}

GLboolean
brw_make_current(__DRIcontext *driContextPriv,
                 __DRIdrawable *driDrawPriv,
                 __DRIdrawable *driReadPriv)
{
   struct brw_context *brw;

   if (driContextPriv)
      brw = (struct brw_context *) driContextPriv->driverPrivate;
   else
      brw = NULL;

   if (driContextPriv) {
      struct gl_context *ctx = &brw->ctx;
      struct gl_framebuffer *fb, *readFb;

      if (driDrawPriv == NULL) {
         fb = _mesa_get_incomplete_framebuffer();
      } else {
         fb = driDrawPriv->driverPrivate;
         driContextPriv->dri2.draw_stamp = driDrawPriv->dri2.stamp - 1;
      }

      if (driReadPriv == NULL) {
         readFb = _mesa_get_incomplete_framebuffer();
      } else {
         readFb = driReadPriv->driverPrivate;
         driContextPriv->dri2.read_stamp = driReadPriv->dri2.stamp - 1;
      }

      /* The sRGB workaround changes the renderbuffer's format. We must change
       * the format before the renderbuffer's miptree get's allocated, otherwise
       * the formats of the renderbuffer and its miptree will differ.
       */
      brw_gles3_srgb_workaround(brw, fb);
      brw_gles3_srgb_workaround(brw, readFb);

      /* If the context viewport hasn't been initialized, force a call out to
       * the loader to get buffers so we have a drawable size for the initial
       * viewport. */
      if (!brw->ctx.ViewportInitialized)
         brw_prepare_render(brw);

      _mesa_make_current(ctx, fb, readFb);
   } else {
      GET_CURRENT_CONTEXT(ctx);
      _mesa_glthread_finish(ctx);
      _mesa_make_current(NULL, NULL, NULL);
   }

   return true;
}

void
brw_resolve_for_dri2_flush(struct brw_context *brw,
                           __DRIdrawable *drawable)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (devinfo->ver < 6) {
      /* MSAA and fast color clear are not supported, so don't waste time
       * checking whether a resolve is needed.
       */
      return;
   }

   struct gl_framebuffer *fb = drawable->driverPrivate;
   struct brw_renderbuffer *rb;

   /* Usually, only the back buffer will need to be downsampled. However,
    * the front buffer will also need it if the user has rendered into it.
    */
   static const gl_buffer_index buffers[2] = {
         BUFFER_BACK_LEFT,
         BUFFER_FRONT_LEFT,
   };

   for (int i = 0; i < 2; ++i) {
      rb = brw_get_renderbuffer(fb, buffers[i]);
      if (rb == NULL || rb->mt == NULL)
         continue;
      if (rb->mt->surf.samples == 1) {
         assert(rb->mt_layer == 0 && rb->mt_level == 0 &&
                rb->layer_count == 1);
         brw_miptree_prepare_external(brw, rb->mt);
      } else {
         brw_renderbuffer_downsample(brw, rb);

         /* Call prepare_external on the single-sample miptree to do any
          * needed resolves prior to handing it off to the window system.
          * This is needed in the case that rb->singlesample_mt is Y-tiled
          * with CCS_E enabled but without I915_FORMAT_MOD_Y_TILED_CCS_E.  In
          * this case, the MSAA resolve above will write compressed data into
          * rb->singlesample_mt.
          *
          * TODO: Some day, if we decide to care about the tiny performance
          * hit we're taking by doing the MSAA resolve and then a CCS resolve,
          * we could detect this case and just allocate the single-sampled
          * miptree without aux.  However, that would be a lot of plumbing and
          * this is a rather exotic case so it's not really worth it.
          */
         brw_miptree_prepare_external(brw, rb->singlesample_mt);
      }
   }
}

static unsigned
brw_bits_per_pixel(const struct brw_renderbuffer *rb)
{
   return _mesa_get_format_bytes(brw_rb_format(rb)) * 8;
}

static void
brw_query_dri2_buffers(struct brw_context *brw,
                       __DRIdrawable *drawable,
                       __DRIbuffer **buffers,
                       int *count);

static void
brw_process_dri2_buffer(struct brw_context *brw,
                        __DRIdrawable *drawable,
                        __DRIbuffer *buffer,
                        struct brw_renderbuffer *rb,
                        const char *buffer_name);

static void
brw_update_image_buffers(struct brw_context *brw, __DRIdrawable *drawable);

static void
brw_update_dri2_buffers(struct brw_context *brw, __DRIdrawable *drawable)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;
   struct brw_renderbuffer *rb;
   __DRIbuffer *buffers = NULL;
   int count;
   const char *region_name;

   /* Set this up front, so that in case our buffers get invalidated
    * while we're getting new buffers, we don't clobber the stamp and
    * thus ignore the invalidate. */
   drawable->lastStamp = drawable->dri2.stamp;

   if (INTEL_DEBUG(DEBUG_DRI))
      fprintf(stderr, "enter %s, drawable %p\n", __func__, drawable);

   brw_query_dri2_buffers(brw, drawable, &buffers, &count);

   if (buffers == NULL)
      return;

   for (int i = 0; i < count; i++) {
       switch (buffers[i].attachment) {
       case __DRI_BUFFER_FRONT_LEFT:
           rb = brw_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
           region_name = "dri2 front buffer";
           break;

       case __DRI_BUFFER_FAKE_FRONT_LEFT:
           rb = brw_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
           region_name = "dri2 fake front buffer";
           break;

       case __DRI_BUFFER_BACK_LEFT:
           rb = brw_get_renderbuffer(fb, BUFFER_BACK_LEFT);
           region_name = "dri2 back buffer";
           break;

       case __DRI_BUFFER_DEPTH:
       case __DRI_BUFFER_HIZ:
       case __DRI_BUFFER_DEPTH_STENCIL:
       case __DRI_BUFFER_STENCIL:
       case __DRI_BUFFER_ACCUM:
       default:
           fprintf(stderr,
                   "unhandled buffer attach event, attachment type %d\n",
                   buffers[i].attachment);
           return;
       }

       brw_process_dri2_buffer(brw, drawable, &buffers[i], rb, region_name);
   }

}

void
brw_update_renderbuffers(__DRIcontext *context, __DRIdrawable *drawable)
{
   struct brw_context *brw = context->driverPrivate;
   __DRIscreen *dri_screen = brw->screen->driScrnPriv;

   /* Set this up front, so that in case our buffers get invalidated
    * while we're getting new buffers, we don't clobber the stamp and
    * thus ignore the invalidate. */
   drawable->lastStamp = drawable->dri2.stamp;

   if (INTEL_DEBUG(DEBUG_DRI))
      fprintf(stderr, "enter %s, drawable %p\n", __func__, drawable);

   if (dri_screen->image.loader)
      brw_update_image_buffers(brw, drawable);
   else
      brw_update_dri2_buffers(brw, drawable);

   driUpdateFramebufferSize(&brw->ctx, drawable);
}

/**
 * intel_prepare_render should be called anywhere that curent read/drawbuffer
 * state is required.
 */
void
brw_prepare_render(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   __DRIcontext *driContext = brw->driContext;
   __DRIdrawable *drawable;

   drawable = driContext->driDrawablePriv;
   if (drawable && drawable->dri2.stamp != driContext->dri2.draw_stamp) {
      if (drawable->lastStamp != drawable->dri2.stamp)
         brw_update_renderbuffers(driContext, drawable);
      driContext->dri2.draw_stamp = drawable->dri2.stamp;
   }

   drawable = driContext->driReadablePriv;
   if (drawable && drawable->dri2.stamp != driContext->dri2.read_stamp) {
      if (drawable->lastStamp != drawable->dri2.stamp)
         brw_update_renderbuffers(driContext, drawable);
      driContext->dri2.read_stamp = drawable->dri2.stamp;
   }

   /* If we're currently rendering to the front buffer, the rendering
    * that will happen next will probably dirty the front buffer.  So
    * mark it as dirty here.
    */
   if (_mesa_is_front_buffer_drawing(ctx->DrawBuffer) &&
       ctx->DrawBuffer != _mesa_get_incomplete_framebuffer()) {
      brw->front_buffer_dirty = true;
   }

   if (brw->is_shared_buffer_bound) {
      /* Subsequent rendering will probably dirty the shared buffer. */
      brw->is_shared_buffer_dirty = true;
   }
}

/**
 * \brief Query DRI2 to obtain a DRIdrawable's buffers.
 *
 * To determine which DRI buffers to request, examine the renderbuffers
 * attached to the drawable's framebuffer. Then request the buffers with
 * DRI2GetBuffers() or DRI2GetBuffersWithFormat().
 *
 * This is called from brw_update_renderbuffers().
 *
 * \param drawable      Drawable whose buffers are queried.
 * \param buffers       [out] List of buffers returned by DRI2 query.
 * \param buffer_count  [out] Number of buffers returned.
 *
 * \see brw_update_renderbuffers()
 * \see DRI2GetBuffers()
 * \see DRI2GetBuffersWithFormat()
 */
static void
brw_query_dri2_buffers(struct brw_context *brw,
                       __DRIdrawable *drawable,
                       __DRIbuffer **buffers,
                       int *buffer_count)
{
   __DRIscreen *dri_screen = brw->screen->driScrnPriv;
   struct gl_framebuffer *fb = drawable->driverPrivate;
   int i = 0;
   unsigned attachments[__DRI_BUFFER_COUNT];

   struct brw_renderbuffer *front_rb;
   struct brw_renderbuffer *back_rb;

   front_rb = brw_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   back_rb = brw_get_renderbuffer(fb, BUFFER_BACK_LEFT);

   memset(attachments, 0, sizeof(attachments));
   if ((_mesa_is_front_buffer_drawing(fb) ||
        _mesa_is_front_buffer_reading(fb) ||
        !back_rb) && front_rb) {
      /* If a fake front buffer is in use, then querying for
       * __DRI_BUFFER_FRONT_LEFT will cause the server to copy the image from
       * the real front buffer to the fake front buffer.  So before doing the
       * query, we need to make sure all the pending drawing has landed in the
       * real front buffer.
       */
      brw_batch_flush(brw);
      brw_flush_front(&brw->ctx);

      attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
      attachments[i++] = brw_bits_per_pixel(front_rb);
   } else if (front_rb && brw->front_buffer_dirty) {
      /* We have pending front buffer rendering, but we aren't querying for a
       * front buffer.  If the front buffer we have is a fake front buffer,
       * the X server is going to throw it away when it processes the query.
       * So before doing the query, make sure all the pending drawing has
       * landed in the real front buffer.
       */
      brw_batch_flush(brw);
      brw_flush_front(&brw->ctx);
   }

   if (back_rb) {
      attachments[i++] = __DRI_BUFFER_BACK_LEFT;
      attachments[i++] = brw_bits_per_pixel(back_rb);
   }

   assert(i <= ARRAY_SIZE(attachments));

   *buffers =
      dri_screen->dri2.loader->getBuffersWithFormat(drawable,
                                                    &drawable->w,
                                                    &drawable->h,
                                                    attachments, i / 2,
                                                    buffer_count,
                                                    drawable->loaderPrivate);
}

/**
 * \brief Assign a DRI buffer's DRM region to a renderbuffer.
 *
 * This is called from brw_update_renderbuffers().
 *
 * \par Note:
 *    DRI buffers whose attachment point is DRI2BufferStencil or
 *    DRI2BufferDepthStencil are handled as special cases.
 *
 * \param buffer_name is a human readable name, such as "dri2 front buffer",
 *        that is passed to brw_bo_gem_create_from_name().
 *
 * \see brw_update_renderbuffers()
 */
static void
brw_process_dri2_buffer(struct brw_context *brw,
                        __DRIdrawable *drawable,
                        __DRIbuffer *buffer,
                        struct brw_renderbuffer *rb,
                        const char *buffer_name)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;
   struct brw_bo *bo;

   if (!rb)
      return;

   unsigned num_samples = rb->Base.Base.NumSamples;

   /* We try to avoid closing and reopening the same BO name, because the first
    * use of a mapping of the buffer involves a bunch of page faulting which is
    * moderately expensive.
    */
   struct brw_mipmap_tree *last_mt;
   if (num_samples == 0)
      last_mt = rb->mt;
   else
      last_mt = rb->singlesample_mt;

   uint32_t old_name = 0;
   if (last_mt) {
       /* The bo already has a name because the miptree was created by a
        * previous call to brw_process_dri2_buffer(). If a bo already has a
        * name, then brw_bo_flink() is a low-cost getter.  It does not
        * create a new name.
        */
      brw_bo_flink(last_mt->bo, &old_name);
   }

   if (old_name == buffer->name)
      return;

   if (INTEL_DEBUG(DEBUG_DRI)) {
      fprintf(stderr,
              "attaching buffer %d, at %d, cpp %d, pitch %d\n",
              buffer->name, buffer->attachment,
              buffer->cpp, buffer->pitch);
   }

   bo = brw_bo_gem_create_from_name(brw->bufmgr, buffer_name,
                                          buffer->name);
   if (!bo) {
      fprintf(stderr,
              "Failed to open BO for returned DRI2 buffer "
              "(%dx%d, %s, named %d).\n"
              "This is likely a bug in the X Server that will lead to a "
              "crash soon.\n",
              drawable->w, drawable->h, buffer_name, buffer->name);
      return;
   }

   uint32_t tiling, swizzle;
   brw_bo_get_tiling(bo, &tiling, &swizzle);

   struct brw_mipmap_tree *mt =
      brw_miptree_create_for_bo(brw,
                                bo,
                                brw_rb_format(rb),
                                0,
                                drawable->w,
                                drawable->h,
                                1,
                                buffer->pitch,
                                isl_tiling_from_i915_tiling(tiling),
                                MIPTREE_CREATE_DEFAULT);
   if (!mt) {
      brw_bo_unreference(bo);
      return;
   }

   /* We got this BO from X11.  We cana't assume that we have coherent texture
    * access because X may suddenly decide to use it for scan-out which would
    * destroy coherency.
    */
   bo->cache_coherent = false;

   if (!brw_update_winsys_renderbuffer_miptree(brw, rb, mt,
                                                 drawable->w, drawable->h,
                                                 buffer->pitch)) {
      brw_bo_unreference(bo);
      brw_miptree_release(&mt);
      return;
   }

   if (_mesa_is_front_buffer_drawing(fb) &&
       (buffer->attachment == __DRI_BUFFER_FRONT_LEFT ||
        buffer->attachment == __DRI_BUFFER_FAKE_FRONT_LEFT) &&
       rb->Base.Base.NumSamples > 1) {
      brw_renderbuffer_upsample(brw, rb);
   }

   assert(rb->mt);

   brw_bo_unreference(bo);
}

/**
 * \brief Query DRI image loader to obtain a DRIdrawable's buffers.
 *
 * To determine which DRI buffers to request, examine the renderbuffers
 * attached to the drawable's framebuffer. Then request the buffers from
 * the image loader
 *
 * This is called from brw_update_renderbuffers().
 *
 * \param drawable      Drawable whose buffers are queried.
 * \param buffers       [out] List of buffers returned by DRI2 query.
 * \param buffer_count  [out] Number of buffers returned.
 *
 * \see brw_update_renderbuffers()
 */

static void
brw_update_image_buffer(struct brw_context *intel,
                        __DRIdrawable *drawable,
                        struct brw_renderbuffer *rb,
                        __DRIimage *buffer,
                        enum __DRIimageBufferMask buffer_type)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;

   if (!rb || !buffer->bo)
      return;

   unsigned num_samples = rb->Base.Base.NumSamples;

   /* Check and see if we're already bound to the right
    * buffer object
    */
   struct brw_mipmap_tree *last_mt;
   if (num_samples == 0)
      last_mt = rb->mt;
   else
      last_mt = rb->singlesample_mt;

   if (last_mt && last_mt->bo == buffer->bo) {
      if (buffer_type == __DRI_IMAGE_BUFFER_SHARED) {
         brw_miptree_make_shareable(intel, last_mt);
      }
      return;
   }

   /* Only allow internal compression if samples == 0.  For multisampled
    * window system buffers, the only thing the single-sampled buffer is used
    * for is as a resolve target.  If we do any compression beyond what is
    * supported by the window system, we will just have to resolve so it's
    * probably better to just not bother.
    */
   const bool allow_internal_aux = (num_samples == 0);

   struct brw_mipmap_tree *mt =
      brw_miptree_create_for_dri_image(intel, buffer, GL_TEXTURE_2D,
                                       brw_rb_format(rb),
                                       allow_internal_aux);
   if (!mt)
      return;

   if (!brw_update_winsys_renderbuffer_miptree(intel, rb, mt,
                                                 buffer->width, buffer->height,
                                                 buffer->pitch)) {
      brw_miptree_release(&mt);
      return;
   }

   if (_mesa_is_front_buffer_drawing(fb) &&
       buffer_type == __DRI_IMAGE_BUFFER_FRONT &&
       rb->Base.Base.NumSamples > 1) {
      brw_renderbuffer_upsample(intel, rb);
   }

   if (buffer_type == __DRI_IMAGE_BUFFER_SHARED) {
      /* The compositor and the application may access this image
       * concurrently. The display hardware may even scanout the image while
       * the GPU is rendering to it.  Aux surfaces cause difficulty with
       * concurrent access, so permanently disable aux for this miptree.
       *
       * Perhaps we could improve overall application performance by
       * re-enabling the aux surface when EGL_RENDER_BUFFER transitions to
       * EGL_BACK_BUFFER, then disabling it again when EGL_RENDER_BUFFER
       * returns to EGL_SINGLE_BUFFER. I expect the wins and losses with this
       * approach to be highly dependent on the application's GL usage.
       *
       * I [chadv] expect clever disabling/reenabling to be counterproductive
       * in the use cases I care about: applications that render nearly
       * realtime handwriting to the surface while possibly undergiong
       * simultaneously scanout as a display plane. The app requires low
       * render latency. Even though the app spends most of its time in
       * shared-buffer mode, it also frequently transitions between
       * shared-buffer (EGL_SINGLE_BUFFER) and double-buffer (EGL_BACK_BUFFER)
       * mode.  Visual sutter during the transitions should be avoided.
       *
       * In this case, I [chadv] believe reducing the GPU workload at
       * shared-buffer/double-buffer transitions would offer a smoother app
       * experience than any savings due to aux compression. But I've
       * collected no data to prove my theory.
       */
      brw_miptree_make_shareable(intel, mt);
   }
}

static void
brw_update_image_buffers(struct brw_context *brw, __DRIdrawable *drawable)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;
   __DRIscreen *dri_screen = brw->screen->driScrnPriv;
   struct brw_renderbuffer *front_rb;
   struct brw_renderbuffer *back_rb;
   struct __DRIimageList images;
   mesa_format format;
   uint32_t buffer_mask = 0;
   int ret;

   front_rb = brw_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   back_rb = brw_get_renderbuffer(fb, BUFFER_BACK_LEFT);

   if (back_rb)
      format = brw_rb_format(back_rb);
   else if (front_rb)
      format = brw_rb_format(front_rb);
   else
      return;

   if (front_rb && (_mesa_is_front_buffer_drawing(fb) ||
                    _mesa_is_front_buffer_reading(fb) || !back_rb)) {
      buffer_mask |= __DRI_IMAGE_BUFFER_FRONT;
   }

   if (back_rb)
      buffer_mask |= __DRI_IMAGE_BUFFER_BACK;

   ret = dri_screen->image.loader->getBuffers(drawable,
                                              driGLFormatToImageFormat(format),
                                              &drawable->dri2.stamp,
                                              drawable->loaderPrivate,
                                              buffer_mask,
                                              &images);
   if (!ret)
      return;

   if (images.image_mask & __DRI_IMAGE_BUFFER_FRONT) {
      drawable->w = images.front->width;
      drawable->h = images.front->height;
      brw_update_image_buffer(brw, drawable, front_rb, images.front,
                              __DRI_IMAGE_BUFFER_FRONT);
   }

   if (images.image_mask & __DRI_IMAGE_BUFFER_BACK) {
      drawable->w = images.back->width;
      drawable->h = images.back->height;
      brw_update_image_buffer(brw, drawable, back_rb, images.back,
                              __DRI_IMAGE_BUFFER_BACK);
   }

   if (images.image_mask & __DRI_IMAGE_BUFFER_SHARED) {
      assert(images.image_mask == __DRI_IMAGE_BUFFER_SHARED);
      drawable->w = images.back->width;
      drawable->h = images.back->height;
      brw_update_image_buffer(brw, drawable, back_rb, images.back,
                              __DRI_IMAGE_BUFFER_SHARED);
      brw->is_shared_buffer_bound = true;
   } else {
      brw->is_shared_buffer_bound = false;
      brw->is_shared_buffer_dirty = false;
   }
}
