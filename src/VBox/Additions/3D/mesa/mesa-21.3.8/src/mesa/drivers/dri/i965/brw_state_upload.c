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



#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "brw_program.h"
#include "drivers/common/meta.h"
#include "brw_batch.h"
#include "brw_buffers.h"
#include "brw_vs.h"
#include "brw_ff_gs.h"
#include "brw_gs.h"
#include "brw_wm.h"
#include "brw_cs.h"
#include "genxml/genX_bits.h"
#include "main/framebuffer.h"

void
brw_enable_obj_preemption(struct brw_context *brw, bool enable)
{
   ASSERTED const struct intel_device_info *devinfo = &brw->screen->devinfo;
   assert(devinfo->ver >= 9);

   if (enable == brw->object_preemption)
      return;

   /* A fixed function pipe flush is required before modifying this field */
   brw_emit_end_of_pipe_sync(brw, PIPE_CONTROL_RENDER_TARGET_FLUSH);

   bool replay_mode = enable ?
      GFX9_REPLAY_MODE_MIDOBJECT : GFX9_REPLAY_MODE_MIDBUFFER;

   /* enable object level preemption */
   brw_load_register_imm32(brw, CS_CHICKEN1,
                           replay_mode | GFX9_REPLAY_MODE_MASK);

   brw->object_preemption = enable;
}

static void
brw_upload_gfx11_slice_hashing_state(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   int subslices_delta =
      devinfo->ppipe_subslices[0] - devinfo->ppipe_subslices[1];
   if (subslices_delta == 0)
      return;

   unsigned size = GFX11_SLICE_HASH_TABLE_length * 4;
   uint32_t hash_address;

   uint32_t *map = brw_state_batch(brw, size, 64, &hash_address);

   unsigned idx = 0;

   unsigned sl_small = 0;
   unsigned sl_big = 1;
   if (subslices_delta > 0) {
      sl_small = 1;
      sl_big = 0;
   }

   /**
    * Create a 16x16 slice hashing table like the following one:
    *
    * [ 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1 ]
    * [ 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1 ]
    * [ 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0 ]
    * [ 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1 ]
    * [ 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1 ]
    * [ 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0 ]
    * [ 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1 ]
    * [ 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1 ]
    * [ 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0 ]
    * [ 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1 ]
    * [ 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1 ]
    * [ 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0 ]
    * [ 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1 ]
    * [ 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1 ]
    * [ 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0 ]
    * [ 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1 ]
    *
    * The table above is used when the pixel pipe 0 has less subslices than
    * pixel pipe 1. When pixel pipe 0 has more subslices, then a similar table
    * with 0's and 1's inverted is used.
    */
   for (int i = 0; i < GFX11_SLICE_HASH_TABLE_length; i++) {
      uint32_t dw = 0;

      for (int j = 0; j < 8; j++) {
         unsigned slice = idx++ % 3 ? sl_big : sl_small;
         dw |= slice << (j * 4);
      }
      map[i] = dw;
   }

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_SLICE_TABLE_STATE_POINTERS << 16 | (2 - 2));
   OUT_RELOC(brw->batch.state.bo, 0, hash_address | 1);
   ADVANCE_BATCH();

   /* From gfx10/gfx11 workaround table in h/w specs:
    *
    *    "On 3DSTATE_3D_MODE, driver must always program bits 31:16 of DW1
    *     a value of 0xFFFF"
    *
    * This means that whenever we update a field with this instruction, we need
    * to update all the others.
    *
    * Since this is the first time we emit this
    * instruction, we are only setting the fSLICE_HASHING_TABLE_ENABLE flag,
    * and leaving everything else at their default state (0).
    */
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_3D_MODE  << 16 | (2 - 2));
   OUT_BATCH(0xffff0000 | SLICE_HASHING_TABLE_ENABLE);
   ADVANCE_BATCH();
}

static void
brw_upload_initial_gpu_state(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   const struct brw_compiler *compiler = brw->screen->compiler;

   /* On platforms with hardware contexts, we can set our initial GPU state
    * right away rather than doing it via state atoms.  This saves a small
    * amount of overhead on every draw call.
    */
   if (!brw->hw_ctx)
      return;

   if (devinfo->ver == 6)
      brw_emit_post_sync_nonzero_flush(brw);

   brw_upload_invariant_state(brw);

   if (devinfo->ver == 11) {
      /* The default behavior of bit 5 "Headerless Message for Pre-emptable
       * Contexts" in SAMPLER MODE register is set to 0, which means
       * headerless sampler messages are not allowed for pre-emptable
       * contexts. Set the bit 5 to 1 to allow them.
       */
      brw_load_register_imm32(brw, GFX11_SAMPLER_MODE,
                              HEADERLESS_MESSAGE_FOR_PREEMPTABLE_CONTEXTS_MASK |
                              HEADERLESS_MESSAGE_FOR_PREEMPTABLE_CONTEXTS);

      /* Bit 1 "Enabled Texel Offset Precision Fix" must be set in
       * HALF_SLICE_CHICKEN7 register.
       */
      brw_load_register_imm32(brw, HALF_SLICE_CHICKEN7,
                              TEXEL_OFFSET_FIX_MASK |
                              TEXEL_OFFSET_FIX_ENABLE);

      /* Wa_1406697149: Bit 9 "Error Detection Behavior Control" must be set
       * in L3CNTLREG register. The default setting of the bit is not the
       * desirable behavior.
       */
      brw_load_register_imm32(brw, GFX8_L3CNTLREG,
                              GFX8_L3CNTLREG_EDBC_NO_HANG);
   }

   /* hardware specification recommends disabling repacking for
    * the compatibility with decompression mechanism in display controller.
    */
   if (devinfo->disable_ccs_repack) {
      brw_load_register_imm32(brw, GFX7_CACHE_MODE_0,
                              GFX11_DISABLE_REPACKING_FOR_COMPRESSION |
                              REG_MASK(GFX11_DISABLE_REPACKING_FOR_COMPRESSION));
   }

   if (devinfo->ver == 9) {
      /* Recommended optimizations for Victim Cache eviction and floating
       * point blending.
       */
      brw_load_register_imm32(brw, GFX7_CACHE_MODE_1,
                              REG_MASK(GFX9_FLOAT_BLEND_OPTIMIZATION_ENABLE) |
                              REG_MASK(GFX9_MSC_RAW_HAZARD_AVOIDANCE_BIT) |
                              REG_MASK(GFX9_PARTIAL_RESOLVE_DISABLE_IN_VC) |
                              GFX9_FLOAT_BLEND_OPTIMIZATION_ENABLE |
                              GFX9_MSC_RAW_HAZARD_AVOIDANCE_BIT |
                              GFX9_PARTIAL_RESOLVE_DISABLE_IN_VC);
   }

   if (devinfo->ver >= 8) {
      gfx8_emit_3dstate_sample_pattern(brw);

      BEGIN_BATCH(5);
      OUT_BATCH(_3DSTATE_WM_HZ_OP << 16 | (5 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();

      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_WM_CHROMAKEY << 16 | (2 - 2));
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* Set the "CONSTANT_BUFFER Address Offset Disable" bit, so
    * 3DSTATE_CONSTANT_XS buffer 0 is an absolute address.
    *
    * This is only safe on kernels with context isolation support.
    */
   if (!compiler->constant_buffer_0_is_relative) {
      if (devinfo->ver >= 9) {
         BEGIN_BATCH(3);
         OUT_BATCH(MI_LOAD_REGISTER_IMM | (3 - 2));
         OUT_BATCH(CS_DEBUG_MODE2);
         OUT_BATCH(REG_MASK(CSDBG2_CONSTANT_BUFFER_ADDRESS_OFFSET_DISABLE) |
                   CSDBG2_CONSTANT_BUFFER_ADDRESS_OFFSET_DISABLE);
         ADVANCE_BATCH();
      } else if (devinfo->ver == 8) {
         BEGIN_BATCH(3);
         OUT_BATCH(MI_LOAD_REGISTER_IMM | (3 - 2));
         OUT_BATCH(INSTPM);
         OUT_BATCH(REG_MASK(INSTPM_CONSTANT_BUFFER_ADDRESS_OFFSET_DISABLE) |
                   INSTPM_CONSTANT_BUFFER_ADDRESS_OFFSET_DISABLE);
         ADVANCE_BATCH();
      }
   }

   brw->object_preemption = false;

   if (devinfo->ver >= 10)
      brw_enable_obj_preemption(brw, true);

   if (devinfo->ver == 11)
      brw_upload_gfx11_slice_hashing_state(brw);
}

static inline const struct brw_tracked_state *
brw_get_pipeline_atoms(struct brw_context *brw,
                       enum brw_pipeline pipeline)
{
   switch (pipeline) {
   case BRW_RENDER_PIPELINE:
      return brw->render_atoms;
   case BRW_COMPUTE_PIPELINE:
      return brw->compute_atoms;
   default:
      STATIC_ASSERT(BRW_NUM_PIPELINES == 2);
      unreachable("Unsupported pipeline");
      return NULL;
   }
}

void
brw_copy_pipeline_atoms(struct brw_context *brw,
                        enum brw_pipeline pipeline,
                        const struct brw_tracked_state **atoms,
                        int num_atoms)
{
   /* This is to work around brw_context::atoms being declared const.  We want
    * it to be const, but it needs to be initialized somehow!
    */
   struct brw_tracked_state *context_atoms =
      (struct brw_tracked_state *) brw_get_pipeline_atoms(brw, pipeline);

   for (int i = 0; i < num_atoms; i++) {
      context_atoms[i] = *atoms[i];
      assert(context_atoms[i].dirty.mesa | context_atoms[i].dirty.brw);
      assert(context_atoms[i].emit);
   }

   brw->num_atoms[pipeline] = num_atoms;
}

void brw_init_state( struct brw_context *brw )
{
   struct gl_context *ctx = &brw->ctx;
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   /* Force the first brw_select_pipeline to emit pipeline select */
   brw->last_pipeline = BRW_NUM_PIPELINES;

   brw_init_caches(brw);

   if (devinfo->ver >= 11)
      gfx11_init_atoms(brw);
   else if (devinfo->ver >= 10)
      unreachable("Gfx10 support dropped.");
   else if (devinfo->ver >= 9)
      gfx9_init_atoms(brw);
   else if (devinfo->ver >= 8)
      gfx8_init_atoms(brw);
   else if (devinfo->is_haswell)
      gfx75_init_atoms(brw);
   else if (devinfo->ver >= 7)
      gfx7_init_atoms(brw);
   else if (devinfo->ver >= 6)
      gfx6_init_atoms(brw);
   else if (devinfo->ver >= 5)
      gfx5_init_atoms(brw);
   else if (devinfo->is_g4x)
      gfx45_init_atoms(brw);
   else
      gfx4_init_atoms(brw);

   brw_upload_initial_gpu_state(brw);

   brw->NewGLState = ~0;
   brw->ctx.NewDriverState = ~0ull;

   /* ~0 is a nonsensical value which won't match anything we program, so
    * the programming will take effect on the first time around.
    */
   brw->pma_stall_bits = ~0;

   /* Make sure that brw->ctx.NewDriverState has enough bits to hold all possible
    * dirty flags.
    */
   STATIC_ASSERT(BRW_NUM_STATE_BITS <= 8 * sizeof(brw->ctx.NewDriverState));

   ctx->DriverFlags.NewTransformFeedback = BRW_NEW_TRANSFORM_FEEDBACK;
   ctx->DriverFlags.NewTransformFeedbackProg = BRW_NEW_TRANSFORM_FEEDBACK;
   ctx->DriverFlags.NewRasterizerDiscard = BRW_NEW_RASTERIZER_DISCARD;
   ctx->DriverFlags.NewUniformBuffer = BRW_NEW_UNIFORM_BUFFER;
   ctx->DriverFlags.NewShaderStorageBuffer = BRW_NEW_UNIFORM_BUFFER;
   ctx->DriverFlags.NewTextureBuffer = BRW_NEW_TEXTURE_BUFFER;
   ctx->DriverFlags.NewAtomicBuffer = BRW_NEW_UNIFORM_BUFFER;
   ctx->DriverFlags.NewImageUnits = BRW_NEW_IMAGE_UNITS;
   ctx->DriverFlags.NewTessState = BRW_NEW_DEFAULT_TESS_LEVELS;
   ctx->DriverFlags.NewIntelConservativeRasterization = BRW_NEW_CONSERVATIVE_RASTERIZATION;
}


void brw_destroy_state( struct brw_context *brw )
{
   brw_destroy_caches(brw);
}

/***********************************************************************
 */

static bool
check_state(const struct brw_state_flags *a, const struct brw_state_flags *b)
{
   return ((a->mesa & b->mesa) | (a->brw & b->brw)) != 0;
}

static void
accumulate_state(struct brw_state_flags *a, const struct brw_state_flags *b)
{
   a->mesa |= b->mesa;
   a->brw |= b->brw;
}


static void
xor_states(struct brw_state_flags *result,
           const struct brw_state_flags *a,
           const struct brw_state_flags *b)
{
   result->mesa = a->mesa ^ b->mesa;
   result->brw = a->brw ^ b->brw;
}

struct dirty_bit_map {
   uint64_t bit;
   char *name;
   uint32_t count;
};

#define DEFINE_BIT(name) {name, #name, 0}

static struct dirty_bit_map mesa_bits[] = {
   DEFINE_BIT(_NEW_MODELVIEW),
   DEFINE_BIT(_NEW_PROJECTION),
   DEFINE_BIT(_NEW_TEXTURE_MATRIX),
   DEFINE_BIT(_NEW_COLOR),
   DEFINE_BIT(_NEW_DEPTH),
   DEFINE_BIT(_NEW_FOG),
   DEFINE_BIT(_NEW_HINT),
   DEFINE_BIT(_NEW_LIGHT),
   DEFINE_BIT(_NEW_LINE),
   DEFINE_BIT(_NEW_PIXEL),
   DEFINE_BIT(_NEW_POINT),
   DEFINE_BIT(_NEW_POLYGON),
   DEFINE_BIT(_NEW_POLYGONSTIPPLE),
   DEFINE_BIT(_NEW_SCISSOR),
   DEFINE_BIT(_NEW_STENCIL),
   DEFINE_BIT(_NEW_TEXTURE_OBJECT),
   DEFINE_BIT(_NEW_TRANSFORM),
   DEFINE_BIT(_NEW_VIEWPORT),
   DEFINE_BIT(_NEW_TEXTURE_STATE),
   DEFINE_BIT(_NEW_RENDERMODE),
   DEFINE_BIT(_NEW_BUFFERS),
   DEFINE_BIT(_NEW_CURRENT_ATTRIB),
   DEFINE_BIT(_NEW_MULTISAMPLE),
   DEFINE_BIT(_NEW_TRACK_MATRIX),
   DEFINE_BIT(_NEW_PROGRAM),
   DEFINE_BIT(_NEW_PROGRAM_CONSTANTS),
   DEFINE_BIT(_NEW_FRAG_CLAMP),
   {0, 0, 0}
};

static struct dirty_bit_map brw_bits[] = {
   DEFINE_BIT(BRW_NEW_FS_PROG_DATA),
   DEFINE_BIT(BRW_NEW_BLORP_BLIT_PROG_DATA),
   DEFINE_BIT(BRW_NEW_SF_PROG_DATA),
   DEFINE_BIT(BRW_NEW_VS_PROG_DATA),
   DEFINE_BIT(BRW_NEW_FF_GS_PROG_DATA),
   DEFINE_BIT(BRW_NEW_GS_PROG_DATA),
   DEFINE_BIT(BRW_NEW_TCS_PROG_DATA),
   DEFINE_BIT(BRW_NEW_TES_PROG_DATA),
   DEFINE_BIT(BRW_NEW_CLIP_PROG_DATA),
   DEFINE_BIT(BRW_NEW_CS_PROG_DATA),
   DEFINE_BIT(BRW_NEW_URB_FENCE),
   DEFINE_BIT(BRW_NEW_FRAGMENT_PROGRAM),
   DEFINE_BIT(BRW_NEW_GEOMETRY_PROGRAM),
   DEFINE_BIT(BRW_NEW_TESS_PROGRAMS),
   DEFINE_BIT(BRW_NEW_VERTEX_PROGRAM),
   DEFINE_BIT(BRW_NEW_REDUCED_PRIMITIVE),
   DEFINE_BIT(BRW_NEW_PATCH_PRIMITIVE),
   DEFINE_BIT(BRW_NEW_PRIMITIVE),
   DEFINE_BIT(BRW_NEW_CONTEXT),
   DEFINE_BIT(BRW_NEW_PSP),
   DEFINE_BIT(BRW_NEW_SURFACES),
   DEFINE_BIT(BRW_NEW_BINDING_TABLE_POINTERS),
   DEFINE_BIT(BRW_NEW_INDICES),
   DEFINE_BIT(BRW_NEW_VERTICES),
   DEFINE_BIT(BRW_NEW_DEFAULT_TESS_LEVELS),
   DEFINE_BIT(BRW_NEW_BATCH),
   DEFINE_BIT(BRW_NEW_INDEX_BUFFER),
   DEFINE_BIT(BRW_NEW_VS_CONSTBUF),
   DEFINE_BIT(BRW_NEW_TCS_CONSTBUF),
   DEFINE_BIT(BRW_NEW_TES_CONSTBUF),
   DEFINE_BIT(BRW_NEW_GS_CONSTBUF),
   DEFINE_BIT(BRW_NEW_PROGRAM_CACHE),
   DEFINE_BIT(BRW_NEW_STATE_BASE_ADDRESS),
   DEFINE_BIT(BRW_NEW_VUE_MAP_GEOM_OUT),
   DEFINE_BIT(BRW_NEW_TRANSFORM_FEEDBACK),
   DEFINE_BIT(BRW_NEW_RASTERIZER_DISCARD),
   DEFINE_BIT(BRW_NEW_STATS_WM),
   DEFINE_BIT(BRW_NEW_UNIFORM_BUFFER),
   DEFINE_BIT(BRW_NEW_IMAGE_UNITS),
   DEFINE_BIT(BRW_NEW_META_IN_PROGRESS),
   DEFINE_BIT(BRW_NEW_PUSH_CONSTANT_ALLOCATION),
   DEFINE_BIT(BRW_NEW_NUM_SAMPLES),
   DEFINE_BIT(BRW_NEW_TEXTURE_BUFFER),
   DEFINE_BIT(BRW_NEW_GFX4_UNIT_STATE),
   DEFINE_BIT(BRW_NEW_CC_VP),
   DEFINE_BIT(BRW_NEW_SF_VP),
   DEFINE_BIT(BRW_NEW_CLIP_VP),
   DEFINE_BIT(BRW_NEW_SAMPLER_STATE_TABLE),
   DEFINE_BIT(BRW_NEW_VS_ATTRIB_WORKAROUNDS),
   DEFINE_BIT(BRW_NEW_COMPUTE_PROGRAM),
   DEFINE_BIT(BRW_NEW_CS_WORK_GROUPS),
   DEFINE_BIT(BRW_NEW_URB_SIZE),
   DEFINE_BIT(BRW_NEW_CC_STATE),
   DEFINE_BIT(BRW_NEW_BLORP),
   DEFINE_BIT(BRW_NEW_VIEWPORT_COUNT),
   DEFINE_BIT(BRW_NEW_CONSERVATIVE_RASTERIZATION),
   DEFINE_BIT(BRW_NEW_DRAW_CALL),
   DEFINE_BIT(BRW_NEW_AUX_STATE),
   {0, 0, 0}
};

static void
brw_update_dirty_count(struct dirty_bit_map *bit_map, uint64_t bits)
{
   for (int i = 0; bit_map[i].bit != 0; i++) {
      if (bit_map[i].bit & bits)
         bit_map[i].count++;
   }
}

static void
brw_print_dirty_count(struct dirty_bit_map *bit_map)
{
   for (int i = 0; bit_map[i].bit != 0; i++) {
      if (bit_map[i].count > 1) {
         fprintf(stderr, "0x%016"PRIx64": %12d (%s)\n",
                 bit_map[i].bit, bit_map[i].count, bit_map[i].name);
      }
   }
}

static inline void
brw_upload_tess_programs(struct brw_context *brw)
{
   if (brw->programs[MESA_SHADER_TESS_EVAL]) {
      brw_upload_tcs_prog(brw);
      brw_upload_tes_prog(brw);
   } else {
      brw->tcs.base.prog_data = NULL;
      brw->tes.base.prog_data = NULL;
   }
}

static inline void
brw_upload_programs(struct brw_context *brw,
                    enum brw_pipeline pipeline)
{
   struct gl_context *ctx = &brw->ctx;
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (pipeline == BRW_RENDER_PIPELINE) {
      brw_upload_vs_prog(brw);
      brw_upload_tess_programs(brw);

      if (brw->programs[MESA_SHADER_GEOMETRY]) {
         brw_upload_gs_prog(brw);
      } else {
         brw->gs.base.prog_data = NULL;
         if (devinfo->ver < 7)
            brw_upload_ff_gs_prog(brw);
      }

      /* Update the VUE map for data exiting the GS stage of the pipeline.
       * This comes from the last enabled shader stage.
       */
      GLbitfield64 old_slots = brw->vue_map_geom_out.slots_valid;
      bool old_separate = brw->vue_map_geom_out.separate;
      struct brw_vue_prog_data *vue_prog_data;
      if (brw->programs[MESA_SHADER_GEOMETRY])
         vue_prog_data = brw_vue_prog_data(brw->gs.base.prog_data);
      else if (brw->programs[MESA_SHADER_TESS_EVAL])
         vue_prog_data = brw_vue_prog_data(brw->tes.base.prog_data);
      else
         vue_prog_data = brw_vue_prog_data(brw->vs.base.prog_data);

      brw->vue_map_geom_out = vue_prog_data->vue_map;

      /* If the layout has changed, signal BRW_NEW_VUE_MAP_GEOM_OUT. */
      if (old_slots != brw->vue_map_geom_out.slots_valid ||
          old_separate != brw->vue_map_geom_out.separate)
         brw->ctx.NewDriverState |= BRW_NEW_VUE_MAP_GEOM_OUT;

      if ((old_slots ^ brw->vue_map_geom_out.slots_valid) &
          VARYING_BIT_VIEWPORT) {
         ctx->NewDriverState |= BRW_NEW_VIEWPORT_COUNT;
         brw->clip.viewport_count =
            (brw->vue_map_geom_out.slots_valid & VARYING_BIT_VIEWPORT) ?
            ctx->Const.MaxViewports : 1;
      }

      brw_upload_wm_prog(brw);

      if (devinfo->ver < 6) {
         brw_upload_clip_prog(brw);
         brw_upload_sf_prog(brw);
      }

      brw_disk_cache_write_render_programs(brw);
   } else if (pipeline == BRW_COMPUTE_PIPELINE) {
      brw_upload_cs_prog(brw);
      brw_disk_cache_write_compute_program(brw);
   }
}

static inline void
merge_ctx_state(struct brw_context *brw,
                struct brw_state_flags *state)
{
   state->mesa |= brw->NewGLState;
   state->brw |= brw->ctx.NewDriverState;
}

static ALWAYS_INLINE void
check_and_emit_atom(struct brw_context *brw,
                    struct brw_state_flags *state,
                    const struct brw_tracked_state *atom)
{
   if (check_state(state, &atom->dirty)) {
      atom->emit(brw);
      merge_ctx_state(brw, state);
   }
}

static inline void
brw_upload_pipeline_state(struct brw_context *brw,
                          enum brw_pipeline pipeline)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;
   int i;
   static int dirty_count = 0;
   struct brw_state_flags state = brw->state.pipelines[pipeline];
   const unsigned fb_samples =
      MAX2(_mesa_geometric_samples(ctx->DrawBuffer), 1);

   brw_select_pipeline(brw, pipeline);

   if (pipeline == BRW_RENDER_PIPELINE && brw->current_hash_scale != 1)
      brw_emit_hashing_mode(brw, UINT_MAX, UINT_MAX, 1);

   if (INTEL_DEBUG(DEBUG_REEMIT)) {
      /* Always re-emit all state. */
      brw->NewGLState = ~0;
      ctx->NewDriverState = ~0ull;
   }

   if (pipeline == BRW_RENDER_PIPELINE) {
      if (brw->programs[MESA_SHADER_FRAGMENT] !=
          ctx->FragmentProgram._Current) {
         brw->programs[MESA_SHADER_FRAGMENT] = ctx->FragmentProgram._Current;
         brw->ctx.NewDriverState |= BRW_NEW_FRAGMENT_PROGRAM;
      }

      if (brw->programs[MESA_SHADER_TESS_EVAL] !=
          ctx->TessEvalProgram._Current) {
         brw->programs[MESA_SHADER_TESS_EVAL] = ctx->TessEvalProgram._Current;
         brw->ctx.NewDriverState |= BRW_NEW_TESS_PROGRAMS;
      }

      if (brw->programs[MESA_SHADER_TESS_CTRL] !=
          ctx->TessCtrlProgram._Current) {
         brw->programs[MESA_SHADER_TESS_CTRL] = ctx->TessCtrlProgram._Current;
         brw->ctx.NewDriverState |= BRW_NEW_TESS_PROGRAMS;
      }

      if (brw->programs[MESA_SHADER_GEOMETRY] !=
          ctx->GeometryProgram._Current) {
         brw->programs[MESA_SHADER_GEOMETRY] = ctx->GeometryProgram._Current;
         brw->ctx.NewDriverState |= BRW_NEW_GEOMETRY_PROGRAM;
      }

      if (brw->programs[MESA_SHADER_VERTEX] != ctx->VertexProgram._Current) {
         brw->programs[MESA_SHADER_VERTEX] = ctx->VertexProgram._Current;
         brw->ctx.NewDriverState |= BRW_NEW_VERTEX_PROGRAM;
      }
   }

   if (brw->programs[MESA_SHADER_COMPUTE] != ctx->ComputeProgram._Current) {
      brw->programs[MESA_SHADER_COMPUTE] = ctx->ComputeProgram._Current;
      brw->ctx.NewDriverState |= BRW_NEW_COMPUTE_PROGRAM;
   }

   if (brw->meta_in_progress != _mesa_meta_in_progress(ctx)) {
      brw->meta_in_progress = _mesa_meta_in_progress(ctx);
      brw->ctx.NewDriverState |= BRW_NEW_META_IN_PROGRESS;
   }

   if (brw->num_samples != fb_samples) {
      brw->num_samples = fb_samples;
      brw->ctx.NewDriverState |= BRW_NEW_NUM_SAMPLES;
   }

   /* Exit early if no state is flagged as dirty */
   merge_ctx_state(brw, &state);
   if ((state.mesa | state.brw) == 0)
      return;

   /* Emit Sandybridge workaround flushes on every primitive, for safety. */
   if (devinfo->ver == 6)
      brw_emit_post_sync_nonzero_flush(brw);

   brw_upload_programs(brw, pipeline);
   merge_ctx_state(brw, &state);

   brw_upload_state_base_address(brw);

   const struct brw_tracked_state *atoms =
      brw_get_pipeline_atoms(brw, pipeline);
   const int num_atoms = brw->num_atoms[pipeline];

   if (INTEL_DEBUG(DEBUG_ANY)) {
      /* Debug version which enforces various sanity checks on the
       * state flags which are generated and checked to help ensure
       * state atoms are ordered correctly in the list.
       */
      struct brw_state_flags examined, prev;
      memset(&examined, 0, sizeof(examined));
      prev = state;

      for (i = 0; i < num_atoms; i++) {
         const struct brw_tracked_state *atom = &atoms[i];
         struct brw_state_flags generated;

         check_and_emit_atom(brw, &state, atom);

         accumulate_state(&examined, &atom->dirty);

         /* generated = (prev ^ state)
          * if (examined & generated)
          *     fail;
          */
         xor_states(&generated, &prev, &state);
         assert(!check_state(&examined, &generated));
         prev = state;
      }
   }
   else {
      for (i = 0; i < num_atoms; i++) {
         const struct brw_tracked_state *atom = &atoms[i];

         check_and_emit_atom(brw, &state, atom);
      }
   }

   if (INTEL_DEBUG(DEBUG_STATE)) {
      STATIC_ASSERT(ARRAY_SIZE(brw_bits) == BRW_NUM_STATE_BITS + 1);

      brw_update_dirty_count(mesa_bits, state.mesa);
      brw_update_dirty_count(brw_bits, state.brw);
      if (dirty_count++ % 1000 == 0) {
         brw_print_dirty_count(mesa_bits);
         brw_print_dirty_count(brw_bits);
         fprintf(stderr, "\n");
      }
   }
}

/***********************************************************************
 * Emit all state:
 */
void brw_upload_render_state(struct brw_context *brw)
{
   brw_upload_pipeline_state(brw, BRW_RENDER_PIPELINE);
}

static inline void
brw_pipeline_state_finished(struct brw_context *brw,
                            enum brw_pipeline pipeline)
{
   /* Save all dirty state into the other pipelines */
   for (unsigned i = 0; i < BRW_NUM_PIPELINES; i++) {
      if (i != pipeline) {
         brw->state.pipelines[i].mesa |= brw->NewGLState;
         brw->state.pipelines[i].brw |= brw->ctx.NewDriverState;
      } else {
         memset(&brw->state.pipelines[i], 0, sizeof(struct brw_state_flags));
      }
   }

   brw->NewGLState = 0;
   brw->ctx.NewDriverState = 0ull;
}

/**
 * Clear dirty bits to account for the fact that the state emitted by
 * brw_upload_render_state() has been committed to the hardware. This is a
 * separate call from brw_upload_render_state() because it's possible that
 * after the call to brw_upload_render_state(), we will discover that we've
 * run out of aperture space, and need to rewind the batch buffer to the state
 * it had before the brw_upload_render_state() call.
 */
void
brw_render_state_finished(struct brw_context *brw)
{
   brw_pipeline_state_finished(brw, BRW_RENDER_PIPELINE);
}

void
brw_upload_compute_state(struct brw_context *brw)
{
   brw_upload_pipeline_state(brw, BRW_COMPUTE_PIPELINE);
}

void
brw_compute_state_finished(struct brw_context *brw)
{
   brw_pipeline_state_finished(brw, BRW_COMPUTE_PIPELINE);
}
