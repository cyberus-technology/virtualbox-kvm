/*
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

#include <pthread.h>
#include "main/glspirv.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "program/prog_to_nir.h"
#include "program/program.h"
#include "program/programopt.h"
#include "tnl/tnl.h"
#include "util/ralloc.h"
#include "compiler/glsl/ir.h"
#include "compiler/glsl/program.h"
#include "compiler/glsl/gl_nir.h"
#include "compiler/glsl/glsl_to_nir.h"

#include "brw_program.h"
#include "brw_context.h"
#include "compiler/brw_nir.h"
#include "brw_defines.h"
#include "brw_batch.h"

#include "brw_cs.h"
#include "brw_gs.h"
#include "brw_vs.h"
#include "brw_wm.h"
#include "brw_state.h"

#include "main/shaderapi.h"
#include "main/shaderobj.h"

static bool
brw_nir_lower_uniforms(nir_shader *nir, bool is_scalar)
{
   if (is_scalar) {
      nir_assign_var_locations(nir, nir_var_uniform, &nir->num_uniforms,
                               type_size_scalar_bytes);
      return nir_lower_io(nir, nir_var_uniform, type_size_scalar_bytes, 0);
   } else {
      nir_assign_var_locations(nir, nir_var_uniform, &nir->num_uniforms,
                               type_size_vec4_bytes);
      return nir_lower_io(nir, nir_var_uniform, type_size_vec4_bytes, 0);
   }
}

static struct gl_program *brw_new_program(struct gl_context *ctx,
                                          gl_shader_stage stage,
                                          GLuint id, bool is_arb_asm);

nir_shader *
brw_create_nir(struct brw_context *brw,
               const struct gl_shader_program *shader_prog,
               struct gl_program *prog,
               gl_shader_stage stage,
               bool is_scalar)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;
   const nir_shader_compiler_options *options =
      ctx->Const.ShaderCompilerOptions[stage].NirOptions;
   nir_shader *nir;

   /* First, lower the GLSL/Mesa IR or SPIR-V to NIR */
   if (shader_prog) {
      if (shader_prog->data->spirv) {
         nir = _mesa_spirv_to_nir(ctx, shader_prog, stage, options);
      } else {
         nir = glsl_to_nir(ctx, shader_prog, stage, options);

         /* Remap the locations to slots so those requiring two slots will
          * occupy two locations. For instance, if we have in the IR code a
          * dvec3 attr0 in location 0 and vec4 attr1 in location 1, in NIR attr0
          * will use locations/slots 0 and 1, and attr1 will use location/slot 2
          */
         if (nir->info.stage == MESA_SHADER_VERTEX)
            nir_remap_dual_slot_attributes(nir, &prog->DualSlotInputs);
      }
      assert (nir);

      nir_remove_dead_variables(nir, nir_var_shader_in | nir_var_shader_out,
                                NULL);
      nir_validate_shader(nir, "after glsl_to_nir or spirv_to_nir");
      NIR_PASS_V(nir, nir_lower_io_to_temporaries,
                 nir_shader_get_entrypoint(nir), true, false);
   } else {
      nir = prog_to_nir(prog, options);
      NIR_PASS_V(nir, nir_lower_regs_to_ssa); /* turn registers into SSA */
   }
   nir_validate_shader(nir, "before brw_preprocess_nir");

   nir_shader_gather_info(nir, nir_shader_get_entrypoint(nir));

   if (!ctx->SoftFP64 && ((nir->info.bit_sizes_int | nir->info.bit_sizes_float) & 64) &&
       (options->lower_doubles_options & nir_lower_fp64_full_software)) {
      ctx->SoftFP64 = glsl_float64_funcs_to_nir(ctx, options);
   }

   brw_preprocess_nir(brw->screen->compiler, nir, ctx->SoftFP64);

   if (stage == MESA_SHADER_TESS_CTRL) {
      /* Lower gl_PatchVerticesIn from a sys. value to a uniform on Gfx8+. */
      static const gl_state_index16 tokens[STATE_LENGTH] =
         { STATE_TCS_PATCH_VERTICES_IN };
      nir_lower_patch_vertices(nir, 0, devinfo->ver >= 8 ? tokens : NULL);
   }

   if (stage == MESA_SHADER_TESS_EVAL) {
      /* Lower gl_PatchVerticesIn to a constant if we have a TCS, or
       * a uniform if we don't.
       */
      struct gl_linked_shader *tcs =
         shader_prog->_LinkedShaders[MESA_SHADER_TESS_CTRL];
      uint32_t static_patch_vertices =
         tcs ? tcs->Program->nir->info.tess.tcs_vertices_out : 0;
      static const gl_state_index16 tokens[STATE_LENGTH] =
         { STATE_TES_PATCH_VERTICES_IN };
      nir_lower_patch_vertices(nir, static_patch_vertices, tokens);
   }

   if (stage == MESA_SHADER_FRAGMENT) {
      static const struct nir_lower_wpos_ytransform_options wpos_options = {
         .state_tokens = {STATE_FB_WPOS_Y_TRANSFORM, 0, 0},
         .fs_coord_pixel_center_integer = 1,
         .fs_coord_origin_upper_left = 1,
      };

      bool progress = false;
      NIR_PASS(progress, nir, nir_lower_wpos_ytransform, &wpos_options);
      if (progress) {
         _mesa_add_state_reference(prog->Parameters,
                                   wpos_options.state_tokens);
      }
   }

   return nir;
}

static void
shared_type_info(const struct glsl_type *type, unsigned *size, unsigned *align)
{
   assert(glsl_type_is_vector_or_scalar(type));

   uint32_t comp_size = glsl_type_is_boolean(type)
      ? 4 : glsl_get_bit_size(type) / 8;
   unsigned length = glsl_get_vector_elements(type);
   *size = comp_size * length,
   *align = comp_size * (length == 3 ? 4 : length);
}

void
brw_nir_lower_resources(nir_shader *nir, struct gl_shader_program *shader_prog,
                        struct gl_program *prog,
                        const struct intel_device_info *devinfo)
{
   NIR_PASS_V(nir, brw_nir_lower_uniforms, nir->options->lower_to_scalar);
   NIR_PASS_V(prog->nir, gl_nir_lower_samplers, shader_prog);
   BITSET_COPY(prog->info.textures_used, prog->nir->info.textures_used);
   BITSET_COPY(prog->info.textures_used_by_txf, prog->nir->info.textures_used_by_txf);

   NIR_PASS_V(prog->nir, brw_nir_lower_storage_image, devinfo);

   if (prog->nir->info.stage == MESA_SHADER_COMPUTE &&
       shader_prog->data->spirv) {
      NIR_PASS_V(prog->nir, nir_lower_vars_to_explicit_types,
                 nir_var_mem_shared, shared_type_info);
      NIR_PASS_V(prog->nir, nir_lower_explicit_io,
                 nir_var_mem_shared, nir_address_format_32bit_offset);
   }

   NIR_PASS_V(prog->nir, gl_nir_lower_buffers, shader_prog);
   /* Do a round of constant folding to clean up address calculations */
   NIR_PASS_V(prog->nir, nir_opt_constant_folding);
}

void
brw_shader_gather_info(nir_shader *nir, struct gl_program *prog)
{
   nir_shader_gather_info(nir, nir_shader_get_entrypoint(nir));

   /* Copy the info we just generated back into the gl_program */
   const char *prog_name = prog->info.name;
   const char *prog_label = prog->info.label;
   prog->info = nir->info;
   prog->info.name = prog_name;
   prog->info.label = prog_label;
}

static unsigned
get_new_program_id(struct brw_screen *screen)
{
   return p_atomic_inc_return(&screen->program_id);
}

static struct gl_program *
brw_new_program(struct gl_context *ctx,
                gl_shader_stage stage,
                GLuint id, bool is_arb_asm)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_program *prog = rzalloc(NULL, struct brw_program);

   if (prog) {
      prog->id = get_new_program_id(brw->screen);

      return _mesa_init_gl_program(&prog->program, stage, id, is_arb_asm);
   }

   return NULL;
}

static void
brw_delete_program(struct gl_context *ctx, struct gl_program *prog)
{
   struct brw_context *brw = brw_context(ctx);

   /* Beware!  prog's refcount has reached zero, and it's about to be freed.
    *
    * In brw_upload_pipeline_state(), we compare brw->programs[i] to
    * ctx->FooProgram._Current, and flag BRW_NEW_FOO_PROGRAM if the
    * pointer has changed.
    *
    * We cannot leave brw->programs[i] as a dangling pointer to the dead
    * program.  malloc() may allocate the same memory for a new gl_program,
    * causing us to see matching pointers...but totally different programs.
    *
    * We cannot set brw->programs[i] to NULL, either.  If we've deleted the
    * active program, Mesa may set ctx->FooProgram._Current to NULL.  That
    * would cause us to see matching pointers (NULL == NULL), and fail to
    * detect that a program has changed since our last draw.
    *
    * So, set it to a bogus gl_program pointer that will never match,
    * causing us to properly reevaluate the state on our next draw.
    *
    * Getting this wrong causes heisenbugs which are very hard to catch,
    * as you need a very specific allocation pattern to hit the problem.
    */
   static const struct gl_program deleted_program;

   for (int i = 0; i < MESA_SHADER_STAGES; i++) {
      if (brw->programs[i] == prog)
         brw->programs[i] = (struct gl_program *) &deleted_program;
   }

   _mesa_delete_program( ctx, prog );
}


static GLboolean
brw_program_string_notify(struct gl_context *ctx,
                          GLenum target,
                          struct gl_program *prog)
{
   assert(target == GL_VERTEX_PROGRAM_ARB || !prog->arb.IsPositionInvariant);

   struct brw_context *brw = brw_context(ctx);
   const struct brw_compiler *compiler = brw->screen->compiler;

   switch (target) {
   case GL_FRAGMENT_PROGRAM_ARB: {
      struct brw_program *newFP = brw_program(prog);
      const struct brw_program *curFP =
         brw_program_const(brw->programs[MESA_SHADER_FRAGMENT]);

      if (newFP == curFP)
         brw->ctx.NewDriverState |= BRW_NEW_FRAGMENT_PROGRAM;
      _mesa_program_fragment_position_to_sysval(&newFP->program);
      newFP->id = get_new_program_id(brw->screen);

      prog->nir = brw_create_nir(brw, NULL, prog, MESA_SHADER_FRAGMENT, true);

      brw_nir_lower_resources(prog->nir, NULL, prog, &brw->screen->devinfo);

      brw_shader_gather_info(prog->nir, prog);

      brw_fs_precompile(ctx, prog);
      break;
   }
   case GL_VERTEX_PROGRAM_ARB: {
      struct brw_program *newVP = brw_program(prog);
      const struct brw_program *curVP =
         brw_program_const(brw->programs[MESA_SHADER_VERTEX]);

      if (newVP == curVP)
         brw->ctx.NewDriverState |= BRW_NEW_VERTEX_PROGRAM;
      if (newVP->program.arb.IsPositionInvariant) {
         _mesa_insert_mvp_code(ctx, &newVP->program);
      }
      newVP->id = get_new_program_id(brw->screen);

      /* Also tell tnl about it:
       */
      _tnl_program_string(ctx, target, prog);

      prog->nir = brw_create_nir(brw, NULL, prog, MESA_SHADER_VERTEX,
                                 compiler->scalar_stage[MESA_SHADER_VERTEX]);

      brw_nir_lower_resources(prog->nir, NULL, prog, &brw->screen->devinfo);

      brw_shader_gather_info(prog->nir, prog);

      brw_vs_precompile(ctx, prog);
      break;
   }
   default:
      /*
       * driver->ProgramStringNotify is only called for ARB programs, fixed
       * function vertex programs, and ir_to_mesa (which isn't used by the
       * i965 back-end).  Therefore, even after geometry shaders are added,
       * this function should only ever be called with a target of
       * GL_VERTEX_PROGRAM_ARB or GL_FRAGMENT_PROGRAM_ARB.
       */
      unreachable("Unexpected target in brwProgramStringNotify");
   }

   return true;
}

static void
brw_memory_barrier(struct gl_context *ctx, GLbitfield barriers)
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   unsigned bits = PIPE_CONTROL_DATA_CACHE_FLUSH | PIPE_CONTROL_CS_STALL;
   assert(devinfo->ver >= 7 && devinfo->ver <= 11);

   if (barriers & (GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT |
                   GL_ELEMENT_ARRAY_BARRIER_BIT |
                   GL_COMMAND_BARRIER_BIT))
      bits |= PIPE_CONTROL_VF_CACHE_INVALIDATE;

   if (barriers & GL_UNIFORM_BARRIER_BIT)
      bits |= (PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE |
               PIPE_CONTROL_CONST_CACHE_INVALIDATE);

   if (barriers & GL_TEXTURE_FETCH_BARRIER_BIT)
      bits |= PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE;

   if (barriers & (GL_TEXTURE_UPDATE_BARRIER_BIT |
                   GL_PIXEL_BUFFER_BARRIER_BIT))
      bits |= (PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE |
               PIPE_CONTROL_RENDER_TARGET_FLUSH);

   if (barriers & GL_FRAMEBUFFER_BARRIER_BIT)
      bits |= (PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE |
               PIPE_CONTROL_RENDER_TARGET_FLUSH);

   /* Typed surface messages are handled by the render cache on IVB, so we
    * need to flush it too.
    */
   if (devinfo->verx10 == 70)
      bits |= PIPE_CONTROL_RENDER_TARGET_FLUSH;

   brw_emit_pipe_control_flush(brw, bits);
}

static void
brw_framebuffer_fetch_barrier(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (!ctx->Extensions.EXT_shader_framebuffer_fetch) {
      if (devinfo->ver >= 6) {
         brw_emit_pipe_control_flush(brw,
                                     PIPE_CONTROL_RENDER_TARGET_FLUSH |
                                     PIPE_CONTROL_CS_STALL);
         brw_emit_pipe_control_flush(brw,
                                     PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE);
      } else {
         brw_emit_pipe_control_flush(brw,
                                     PIPE_CONTROL_RENDER_TARGET_FLUSH);
      }
   }
}

void
brw_get_scratch_bo(struct brw_context *brw,
                   struct brw_bo **scratch_bo, int size)
{
   struct brw_bo *old_bo = *scratch_bo;

   if (old_bo && old_bo->size < size) {
      brw_bo_unreference(old_bo);
      old_bo = NULL;
   }

   if (!old_bo) {
      *scratch_bo =
         brw_bo_alloc(brw->bufmgr, "scratch bo", size, BRW_MEMZONE_SCRATCH);
   }
}

/**
 * Reserve enough scratch space for the given stage to hold \p per_thread_size
 * bytes times the given \p thread_count.
 */
void
brw_alloc_stage_scratch(struct brw_context *brw,
                        struct brw_stage_state *stage_state,
                        unsigned per_thread_size)
{
   if (stage_state->per_thread_scratch >= per_thread_size)
      return;

   stage_state->per_thread_scratch = per_thread_size;

   if (stage_state->scratch_bo)
      brw_bo_unreference(stage_state->scratch_bo);

   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   assert(stage_state->stage < ARRAY_SIZE(devinfo->max_scratch_ids));
   unsigned max_ids = devinfo->max_scratch_ids[stage_state->stage];
   stage_state->scratch_bo =
      brw_bo_alloc(brw->bufmgr, "shader scratch space",
                   per_thread_size * max_ids, BRW_MEMZONE_SCRATCH);
}

void
brw_init_frag_prog_functions(struct dd_function_table *functions)
{
   assert(functions->ProgramStringNotify == _tnl_program_string);

   functions->NewProgram = brw_new_program;
   functions->DeleteProgram = brw_delete_program;
   functions->ProgramStringNotify = brw_program_string_notify;

   functions->LinkShader = brw_link_shader;

   functions->MemoryBarrier = brw_memory_barrier;
   functions->FramebufferFetchBarrier = brw_framebuffer_fetch_barrier;
}

struct shader_times {
   uint64_t time;
   uint64_t written;
   uint64_t reset;
};

void
brw_init_shader_time(struct brw_context *brw)
{
   const int max_entries = 2048;
   brw->shader_time.bo =
      brw_bo_alloc(brw->bufmgr, "shader time",
                   max_entries * BRW_SHADER_TIME_STRIDE * 3,
                   BRW_MEMZONE_OTHER);
   brw->shader_time.names = rzalloc_array(brw, const char *, max_entries);
   brw->shader_time.ids = rzalloc_array(brw, int, max_entries);
   brw->shader_time.types = rzalloc_array(brw, enum shader_time_shader_type,
                                          max_entries);
   brw->shader_time.cumulative = rzalloc_array(brw, struct shader_times,
                                               max_entries);
   brw->shader_time.max_entries = max_entries;
}

static int
compare_time(const void *a, const void *b)
{
   uint64_t * const *a_val = a;
   uint64_t * const *b_val = b;

   /* We don't just subtract because we're turning the value to an int. */
   if (**a_val < **b_val)
      return -1;
   else if (**a_val == **b_val)
      return 0;
   else
      return 1;
}

static void
print_shader_time_line(const char *stage, const char *name,
                       int shader_num, uint64_t time, uint64_t total)
{
   fprintf(stderr, "%-6s%-18s", stage, name);

   if (shader_num != 0)
      fprintf(stderr, "%4d: ", shader_num);
   else
      fprintf(stderr, "    : ");

   fprintf(stderr, "%16lld (%7.2f Gcycles)      %4.1f%%\n",
           (long long)time,
           (double)time / 1000000000.0,
           (double)time / total * 100.0);
}

static void
brw_report_shader_time(struct brw_context *brw)
{
   if (!brw->shader_time.bo || !brw->shader_time.num_entries)
      return;

   uint64_t scaled[brw->shader_time.num_entries];
   uint64_t *sorted[brw->shader_time.num_entries];
   uint64_t total_by_type[ST_CS + 1];
   memset(total_by_type, 0, sizeof(total_by_type));
   double total = 0;
   for (int i = 0; i < brw->shader_time.num_entries; i++) {
      uint64_t written = 0, reset = 0;
      enum shader_time_shader_type type = brw->shader_time.types[i];

      sorted[i] = &scaled[i];

      switch (type) {
      case ST_VS:
      case ST_TCS:
      case ST_TES:
      case ST_GS:
      case ST_FS8:
      case ST_FS16:
      case ST_FS32:
      case ST_CS:
         written = brw->shader_time.cumulative[i].written;
         reset = brw->shader_time.cumulative[i].reset;
         break;

      default:
         /* I sometimes want to print things that aren't the 3 shader times.
          * Just print the sum in that case.
          */
         written = 1;
         reset = 0;
         break;
      }

      uint64_t time = brw->shader_time.cumulative[i].time;
      if (written) {
         scaled[i] = time / written * (written + reset);
      } else {
         scaled[i] = time;
      }

      switch (type) {
      case ST_VS:
      case ST_TCS:
      case ST_TES:
      case ST_GS:
      case ST_FS8:
      case ST_FS16:
      case ST_FS32:
      case ST_CS:
         total_by_type[type] += scaled[i];
         break;
      default:
         break;
      }

      total += scaled[i];
   }

   if (total == 0) {
      fprintf(stderr, "No shader time collected yet\n");
      return;
   }

   qsort(sorted, brw->shader_time.num_entries, sizeof(sorted[0]), compare_time);

   fprintf(stderr, "\n");
   fprintf(stderr, "type          ID                  cycles spent                   %% of total\n");
   for (int s = 0; s < brw->shader_time.num_entries; s++) {
      const char *stage;
      /* Work back from the sorted pointers times to a time to print. */
      int i = sorted[s] - scaled;

      if (scaled[i] == 0)
         continue;

      int shader_num = brw->shader_time.ids[i];
      const char *shader_name = brw->shader_time.names[i];

      switch (brw->shader_time.types[i]) {
      case ST_VS:
         stage = "vs";
         break;
      case ST_TCS:
         stage = "tcs";
         break;
      case ST_TES:
         stage = "tes";
         break;
      case ST_GS:
         stage = "gs";
         break;
      case ST_FS8:
         stage = "fs8";
         break;
      case ST_FS16:
         stage = "fs16";
         break;
      case ST_FS32:
         stage = "fs32";
         break;
      case ST_CS:
         stage = "cs";
         break;
      default:
         stage = "other";
         break;
      }

      print_shader_time_line(stage, shader_name, shader_num,
                             scaled[i], total);
   }

   fprintf(stderr, "\n");
   print_shader_time_line("total", "vs", 0, total_by_type[ST_VS], total);
   print_shader_time_line("total", "tcs", 0, total_by_type[ST_TCS], total);
   print_shader_time_line("total", "tes", 0, total_by_type[ST_TES], total);
   print_shader_time_line("total", "gs", 0, total_by_type[ST_GS], total);
   print_shader_time_line("total", "fs8", 0, total_by_type[ST_FS8], total);
   print_shader_time_line("total", "fs16", 0, total_by_type[ST_FS16], total);
   print_shader_time_line("total", "fs32", 0, total_by_type[ST_FS32], total);
   print_shader_time_line("total", "cs", 0, total_by_type[ST_CS], total);
}

static void
brw_collect_shader_time(struct brw_context *brw)
{
   if (!brw->shader_time.bo)
      return;

   /* This probably stalls on the last rendering.  We could fix that by
    * delaying reading the reports, but it doesn't look like it's a big
    * overhead compared to the cost of tracking the time in the first place.
    */
   void *bo_map = brw_bo_map(brw, brw->shader_time.bo, MAP_READ | MAP_WRITE);

   for (int i = 0; i < brw->shader_time.num_entries; i++) {
      uint32_t *times = bo_map + i * 3 * BRW_SHADER_TIME_STRIDE;

      brw->shader_time.cumulative[i].time += times[BRW_SHADER_TIME_STRIDE * 0 / 4];
      brw->shader_time.cumulative[i].written += times[BRW_SHADER_TIME_STRIDE * 1 / 4];
      brw->shader_time.cumulative[i].reset += times[BRW_SHADER_TIME_STRIDE * 2 / 4];
   }

   /* Zero the BO out to clear it out for our next collection.
    */
   memset(bo_map, 0, brw->shader_time.bo->size);
   brw_bo_unmap(brw->shader_time.bo);
}

void
brw_collect_and_report_shader_time(struct brw_context *brw)
{
   brw_collect_shader_time(brw);

   if (brw->shader_time.report_time == 0 ||
       get_time() - brw->shader_time.report_time >= 1.0) {
      brw_report_shader_time(brw);
      brw->shader_time.report_time = get_time();
   }
}

/**
 * Chooses an index in the shader_time buffer and sets up tracking information
 * for our printouts.
 *
 * Note that this holds on to references to the underlying programs, which may
 * change their lifetimes compared to normal operation.
 */
int
brw_get_shader_time_index(struct brw_context *brw, struct gl_program *prog,
                          enum shader_time_shader_type type, bool is_glsl_sh)
{
   int shader_time_index = brw->shader_time.num_entries++;
   assert(shader_time_index < brw->shader_time.max_entries);
   brw->shader_time.types[shader_time_index] = type;

   const char *name;
   if (prog->Id == 0) {
      name = "ff";
   } else if (is_glsl_sh) {
      name = prog->info.label ?
         ralloc_strdup(brw->shader_time.names, prog->info.label) : "glsl";
   } else {
      name = "prog";
   }

   brw->shader_time.names[shader_time_index] = name;
   brw->shader_time.ids[shader_time_index] = prog->Id;

   return shader_time_index;
}

void
brw_destroy_shader_time(struct brw_context *brw)
{
   brw_bo_unreference(brw->shader_time.bo);
   brw->shader_time.bo = NULL;
}

void
brw_stage_prog_data_free(const void *p)
{
   struct brw_stage_prog_data *prog_data = (struct brw_stage_prog_data *)p;

   ralloc_free(prog_data->param);
   ralloc_free(prog_data->pull_param);
}

void
brw_dump_arb_asm(const char *stage, struct gl_program *prog)
{
   fprintf(stderr, "ARB_%s_program %d ir for native %s shader\n",
           stage, prog->Id, stage);
   _mesa_print_program(prog);
}

void
brw_setup_tex_for_precompile(const struct intel_device_info *devinfo,
                             struct brw_sampler_prog_key_data *tex,
                             const struct gl_program *prog)
{
   const bool has_shader_channel_select = devinfo->verx10 >= 75;
   unsigned sampler_count = util_last_bit(prog->SamplersUsed);
   for (unsigned i = 0; i < sampler_count; i++) {
      if (!has_shader_channel_select && (prog->ShadowSamplers & (1 << i))) {
         /* Assume DEPTH_TEXTURE_MODE is the default: X, X, X, 1 */
         tex->swizzles[i] =
            MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_ONE);
      } else {
         /* Color sampler: assume no swizzling. */
         tex->swizzles[i] = SWIZZLE_XYZW;
      }
   }
}

/**
 * Sets up the starting offsets for the groups of binding table entries
 * common to all pipeline stages.
 *
 * Unused groups are initialized to 0xd0d0d0d0 to make it obvious that they're
 * unused but also make sure that addition of small offsets to them will
 * trigger some of our asserts that surface indices are < BRW_MAX_SURFACES.
 */
uint32_t
brw_assign_common_binding_table_offsets(const struct intel_device_info *devinfo,
                                        const struct gl_program *prog,
                                        struct brw_stage_prog_data *stage_prog_data,
                                        uint32_t next_binding_table_offset)
{
   int num_textures = util_last_bit(prog->SamplersUsed);

   stage_prog_data->binding_table.texture_start = next_binding_table_offset;
   next_binding_table_offset += num_textures;

   if (prog->info.num_ubos) {
      assert(prog->info.num_ubos <= BRW_MAX_UBO);
      stage_prog_data->binding_table.ubo_start = next_binding_table_offset;
      next_binding_table_offset += prog->info.num_ubos;
   } else {
      stage_prog_data->binding_table.ubo_start = 0xd0d0d0d0;
   }

   if (prog->info.num_ssbos || prog->info.num_abos) {
      assert(prog->info.num_abos <= BRW_MAX_ABO);
      assert(prog->info.num_ssbos <= BRW_MAX_SSBO);
      stage_prog_data->binding_table.ssbo_start = next_binding_table_offset;
      next_binding_table_offset += prog->info.num_abos + prog->info.num_ssbos;
   } else {
      stage_prog_data->binding_table.ssbo_start = 0xd0d0d0d0;
   }

   if (INTEL_DEBUG(DEBUG_SHADER_TIME)) {
      stage_prog_data->binding_table.shader_time_start = next_binding_table_offset;
      next_binding_table_offset++;
   } else {
      stage_prog_data->binding_table.shader_time_start = 0xd0d0d0d0;
   }

   if (prog->info.uses_texture_gather) {
      if (devinfo->ver >= 8) {
         stage_prog_data->binding_table.gather_texture_start =
            stage_prog_data->binding_table.texture_start;
      } else {
         stage_prog_data->binding_table.gather_texture_start = next_binding_table_offset;
         next_binding_table_offset += num_textures;
      }
   } else {
      stage_prog_data->binding_table.gather_texture_start = 0xd0d0d0d0;
   }

   if (prog->info.num_images) {
      stage_prog_data->binding_table.image_start = next_binding_table_offset;
      next_binding_table_offset += prog->info.num_images;
   } else {
      stage_prog_data->binding_table.image_start = 0xd0d0d0d0;
   }

   /* This may or may not be used depending on how the compile goes. */
   stage_prog_data->binding_table.pull_constants_start = next_binding_table_offset;
   next_binding_table_offset++;

   /* Plane 0 is just the regular texture section */
   stage_prog_data->binding_table.plane_start[0] = stage_prog_data->binding_table.texture_start;

   stage_prog_data->binding_table.plane_start[1] = next_binding_table_offset;
   next_binding_table_offset += num_textures;

   stage_prog_data->binding_table.plane_start[2] = next_binding_table_offset;
   next_binding_table_offset += num_textures;

   /* Set the binding table size.  Some callers may append new entries
    * and increase this accordingly.
    */
   stage_prog_data->binding_table.size_bytes = next_binding_table_offset * 4;

   assert(next_binding_table_offset <= BRW_MAX_SURFACES);
   return next_binding_table_offset;
}

void
brw_populate_default_key(const struct brw_compiler *compiler,
                         union brw_any_prog_key *prog_key,
                         struct gl_shader_program *sh_prog,
                         struct gl_program *prog)
{
   switch (prog->info.stage) {
   case MESA_SHADER_VERTEX:
      brw_vs_populate_default_key(compiler, &prog_key->vs, prog);
      break;
   case MESA_SHADER_TESS_CTRL:
      brw_tcs_populate_default_key(compiler, &prog_key->tcs, sh_prog, prog);
      break;
   case MESA_SHADER_TESS_EVAL:
      brw_tes_populate_default_key(compiler, &prog_key->tes, sh_prog, prog);
      break;
   case MESA_SHADER_GEOMETRY:
      brw_gs_populate_default_key(compiler, &prog_key->gs, prog);
      break;
   case MESA_SHADER_FRAGMENT:
      brw_wm_populate_default_key(compiler, &prog_key->wm, prog);
      break;
   case MESA_SHADER_COMPUTE:
      brw_cs_populate_default_key(compiler, &prog_key->cs, prog);
      break;
   default:
      unreachable("Unsupported stage!");
   }
}

void
brw_debug_recompile(struct brw_context *brw,
                    gl_shader_stage stage,
                    unsigned api_id,
                    struct brw_base_prog_key *key)
{
   const struct brw_compiler *compiler = brw->screen->compiler;
   enum brw_cache_id cache_id = brw_stage_cache_id(stage);

   brw_shader_perf_log(compiler, brw, "Recompiling %s shader for program %d\n",
                       _mesa_shader_stage_to_string(stage), api_id);

   const void *old_key =
      brw_find_previous_compile(&brw->cache, cache_id, key->program_string_id);

   brw_debug_key_recompile(compiler, brw, stage, old_key, key);
}
