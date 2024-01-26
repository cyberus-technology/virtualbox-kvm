/*
 * Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 * Intel funded Tungsten Graphics to
 * develop this 3D driver.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "brw_context.h"
#include "brw_wm.h"
#include "brw_state.h"
#include "main/enums.h"
#include "main/formats.h"
#include "main/fbobject.h"
#include "main/samplerobj.h"
#include "main/framebuffer.h"
#include "program/prog_parameter.h"
#include "program/program.h"
#include "brw_mipmap_tree.h"
#include "brw_image.h"
#include "brw_fbo.h"
#include "compiler/brw_nir.h"
#include "brw_program.h"

#include "util/ralloc.h"
#include "util/u_math.h"

static void
assign_fs_binding_table_offsets(const struct intel_device_info *devinfo,
                                const struct gl_program *prog,
                                const struct brw_wm_prog_key *key,
                                struct brw_wm_prog_data *prog_data)
{
   /* Render targets implicitly start at surface index 0.  Even if there are
    * no color regions, we still perform an FB write to a null render target,
    * which will be surface 0.
    */
   uint32_t next_binding_table_offset = MAX2(key->nr_color_regions, 1);

   next_binding_table_offset =
      brw_assign_common_binding_table_offsets(devinfo, prog, &prog_data->base,
                                              next_binding_table_offset);

   if (prog->nir->info.outputs_read && !key->coherent_fb_fetch) {
      prog_data->binding_table.render_target_read_start =
         next_binding_table_offset;
      next_binding_table_offset += key->nr_color_regions;
   }

   /* Update the binding table size */
   prog_data->base.binding_table.size_bytes = next_binding_table_offset * 4;
}

static bool
brw_codegen_wm_prog(struct brw_context *brw,
                    struct brw_program *fp,
                    struct brw_wm_prog_key *key,
                    struct brw_vue_map *vue_map)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   void *mem_ctx = ralloc_context(NULL);
   struct brw_wm_prog_data prog_data;
   const GLuint *program;
   bool start_busy = false;
   double start_time = 0;

   nir_shader *nir = nir_shader_clone(mem_ctx, fp->program.nir);

   memset(&prog_data, 0, sizeof(prog_data));

   /* Use ALT floating point mode for ARB programs so that 0^0 == 1. */
   if (fp->program.info.is_arb_asm)
      prog_data.base.use_alt_mode = true;

   assign_fs_binding_table_offsets(devinfo, &fp->program, key, &prog_data);

   if (!fp->program.info.is_arb_asm) {
      brw_nir_setup_glsl_uniforms(mem_ctx, nir, &fp->program,
                                  &prog_data.base, true);
      if (brw->can_push_ubos) {
         brw_nir_analyze_ubo_ranges(brw->screen->compiler, nir,
                                    NULL, prog_data.base.ubo_ranges);
      }
   } else {
      brw_nir_setup_arb_uniforms(mem_ctx, nir, &fp->program, &prog_data.base);

      if (INTEL_DEBUG(DEBUG_WM))
         brw_dump_arb_asm("fragment", &fp->program);
   }

   if (unlikely(brw->perf_debug)) {
      start_busy = (brw->batch.last_bo &&
                    brw_bo_busy(brw->batch.last_bo));
      start_time = get_time();
   }

   struct brw_compile_fs_params params = {
      .nir = nir,
      .key = key,
      .prog_data = &prog_data,

      .allow_spilling = true,
      .vue_map = vue_map,

      .log_data = brw,
   };

   if (INTEL_DEBUG(DEBUG_SHADER_TIME)) {
      params.shader_time = true;
      params.shader_time_index8 =
         brw_get_shader_time_index(brw, &fp->program, ST_FS8,
                                   !fp->program.info.is_arb_asm);
      params.shader_time_index16 =
         brw_get_shader_time_index(brw, &fp->program, ST_FS16,
                                   !fp->program.info.is_arb_asm);
      params.shader_time_index32 =
         brw_get_shader_time_index(brw, &fp->program, ST_FS32,
                                   !fp->program.info.is_arb_asm);
   }

   program = brw_compile_fs(brw->screen->compiler, mem_ctx, &params);

   if (program == NULL) {
      if (!fp->program.info.is_arb_asm) {
         fp->program.sh.data->LinkStatus = LINKING_FAILURE;
         ralloc_strcat(&fp->program.sh.data->InfoLog, params.error_str);
      }

      _mesa_problem(NULL, "Failed to compile fragment shader: %s\n", params.error_str);

      ralloc_free(mem_ctx);
      return false;
   }

   if (unlikely(brw->perf_debug)) {
      if (fp->compiled_once) {
         brw_debug_recompile(brw, MESA_SHADER_FRAGMENT, fp->program.Id,
                             &key->base);
      }
      fp->compiled_once = true;

      if (start_busy && !brw_bo_busy(brw->batch.last_bo)) {
         perf_debug("FS compile took %.03f ms and stalled the GPU\n",
                    (get_time() - start_time) * 1000);
      }
   }

   brw_alloc_stage_scratch(brw, &brw->wm.base, prog_data.base.total_scratch);

   if (INTEL_DEBUG(DEBUG_WM) && fp->program.info.is_arb_asm)
      fprintf(stderr, "\n");

   /* The param and pull_param arrays will be freed by the shader cache. */
   ralloc_steal(NULL, prog_data.base.param);
   ralloc_steal(NULL, prog_data.base.pull_param);
   brw_upload_cache(&brw->cache, BRW_CACHE_FS_PROG,
                    key, sizeof(struct brw_wm_prog_key),
                    program, prog_data.base.program_size,
                    &prog_data, sizeof(prog_data),
                    &brw->wm.base.prog_offset, &brw->wm.base.prog_data);

   ralloc_free(mem_ctx);

   return true;
}

static uint8_t
gfx6_gather_workaround(GLenum internalformat)
{
   switch (internalformat) {
   case GL_R8I: return WA_SIGN | WA_8BIT;
   case GL_R8UI: return WA_8BIT;
   case GL_R16I: return WA_SIGN | WA_16BIT;
   case GL_R16UI: return WA_16BIT;
   default:
      /* Note that even though GL_R32I and GL_R32UI have format overrides in
       * the surface state, there is no shader w/a required.
       */
      return 0;
   }
}

static void
brw_populate_sampler_prog_key_data(struct gl_context *ctx,
                                   const struct gl_program *prog,
                                   struct brw_sampler_prog_key_data *key)
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   GLbitfield mask = prog->SamplersUsed;

   while (mask) {
      const int s = u_bit_scan(&mask);

      key->swizzles[s] = SWIZZLE_NOOP;
      key->scale_factors[s] = 0.0f;

      int unit_id = prog->SamplerUnits[s];
      const struct gl_texture_unit *unit = &ctx->Texture.Unit[unit_id];

      if (unit->_Current && unit->_Current->Target != GL_TEXTURE_BUFFER) {
         const struct gl_texture_object *t = unit->_Current;
         const struct gl_texture_image *img = t->Image[0][t->Attrib.BaseLevel];
         struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit_id);

         const bool alpha_depth = t->Attrib.DepthMode == GL_ALPHA &&
            (img->_BaseFormat == GL_DEPTH_COMPONENT ||
             img->_BaseFormat == GL_DEPTH_STENCIL);

         /* Haswell handles texture swizzling as surface format overrides
          * (except for GL_ALPHA); all other platforms need MOVs in the shader.
          */
         if (alpha_depth || (devinfo->verx10 <= 70))
            key->swizzles[s] = brw_get_texture_swizzle(ctx, t);

         if (devinfo->ver < 8 &&
             sampler->Attrib.MinFilter != GL_NEAREST &&
             sampler->Attrib.MagFilter != GL_NEAREST) {
            if (sampler->Attrib.WrapS == GL_CLAMP)
               key->gl_clamp_mask[0] |= 1 << s;
            if (sampler->Attrib.WrapT == GL_CLAMP)
               key->gl_clamp_mask[1] |= 1 << s;
            if (sampler->Attrib.WrapR == GL_CLAMP)
               key->gl_clamp_mask[2] |= 1 << s;
         }

         /* gather4 for RG32* is broken in multiple ways on Gfx7. */
         if (devinfo->ver == 7 && prog->info.uses_texture_gather) {
            switch (img->InternalFormat) {
            case GL_RG32I:
            case GL_RG32UI: {
               /* We have to override the format to R32G32_FLOAT_LD.
                * This means that SCS_ALPHA and SCS_ONE will return 0x3f8
                * (1.0) rather than integer 1.  This needs shader hacks.
                *
                * On Ivybridge, we whack W (alpha) to ONE in our key's
                * swizzle.  On Haswell, we look at the original texture
                * swizzle, and use XYZW with channels overridden to ONE,
                * leaving normal texture swizzling to SCS.
                */
               unsigned src_swizzle =
                  devinfo->is_haswell ? t->Attrib._Swizzle : key->swizzles[s];
               for (int i = 0; i < 4; i++) {
                  unsigned src_comp = GET_SWZ(src_swizzle, i);
                  if (src_comp == SWIZZLE_ONE || src_comp == SWIZZLE_W) {
                     key->swizzles[i] &= ~(0x7 << (3 * i));
                     key->swizzles[i] |= SWIZZLE_ONE << (3 * i);
                  }
               }
            }
            FALLTHROUGH;
            case GL_RG32F:
               /* The channel select for green doesn't work - we have to
                * request blue.  Haswell can use SCS for this, but Ivybridge
                * needs a shader workaround.
                */
               if (!devinfo->is_haswell)
                  key->gather_channel_quirk_mask |= 1 << s;
               break;
            }
         }

         /* Gfx6's gather4 is broken for UINT/SINT; we treat them as
          * UNORM/FLOAT instead and fix it in the shader.
          */
         if (devinfo->ver == 6 && prog->info.uses_texture_gather) {
            key->gfx6_gather_wa[s] = gfx6_gather_workaround(img->InternalFormat);
         }

         /* If this is a multisample sampler, and uses the CMS MSAA layout,
          * then we need to emit slightly different code to first sample the
          * MCS surface.
          */
         struct brw_texture_object *intel_tex =
            brw_texture_object((struct gl_texture_object *)t);

         /* From gfx9 onwards some single sampled buffers can also be
          * compressed. These don't need ld2dms sampling along with mcs fetch.
          */
         if (intel_tex->mt->aux_usage == ISL_AUX_USAGE_MCS) {
            assert(devinfo->ver >= 7);
            assert(intel_tex->mt->surf.samples > 1);
            assert(intel_tex->mt->aux_buf);
            assert(intel_tex->mt->surf.msaa_layout == ISL_MSAA_LAYOUT_ARRAY);
            key->compressed_multisample_layout_mask |= 1 << s;

            if (intel_tex->mt->surf.samples >= 16) {
               assert(devinfo->ver >= 9);
               key->msaa_16 |= 1 << s;
            }
         }

         if (t->Target == GL_TEXTURE_EXTERNAL_OES && intel_tex->planar_format) {

            /* Setup possible scaling factor. */
            key->scale_factors[s] = intel_tex->planar_format->scaling_factor;

            switch (intel_tex->planar_format->components) {
            case __DRI_IMAGE_COMPONENTS_Y_UV:
               key->y_uv_image_mask |= 1 << s;
               break;
            case __DRI_IMAGE_COMPONENTS_Y_U_V:
               key->y_u_v_image_mask |= 1 << s;
               break;
            case __DRI_IMAGE_COMPONENTS_Y_XUXV:
               key->yx_xuxv_image_mask |= 1 << s;
               break;
            case __DRI_IMAGE_COMPONENTS_Y_UXVX:
               key->xy_uxvx_image_mask |= 1 << s;
               break;
            case __DRI_IMAGE_COMPONENTS_AYUV:
               key->ayuv_image_mask |= 1 << s;
               break;
            case __DRI_IMAGE_COMPONENTS_XYUV:
               key->xyuv_image_mask |= 1 << s;
               break;
            default:
               break;
            }

            switch (intel_tex->yuv_color_space) {
            case __DRI_YUV_COLOR_SPACE_ITU_REC709:
              key->bt709_mask |= 1 << s;
              break;
            case __DRI_YUV_COLOR_SPACE_ITU_REC2020:
              key->bt2020_mask |= 1 << s;
              break;
            default:
              break;
            }
         }

      }
   }
}

void
brw_populate_base_prog_key(struct gl_context *ctx,
                           const struct brw_program *prog,
                           struct brw_base_prog_key *key)
{
   key->program_string_id = prog->id;
   key->subgroup_size_type = BRW_SUBGROUP_SIZE_UNIFORM;
   brw_populate_sampler_prog_key_data(ctx, &prog->program, &key->tex);
}

void
brw_populate_default_base_prog_key(const struct intel_device_info *devinfo,
                                   const struct brw_program *prog,
                                   struct brw_base_prog_key *key)
{
   key->program_string_id = prog->id;
   key->subgroup_size_type = BRW_SUBGROUP_SIZE_UNIFORM;
   brw_setup_tex_for_precompile(devinfo, &key->tex, &prog->program);
}

static bool
brw_wm_state_dirty(const struct brw_context *brw)
{
   return brw_state_dirty(brw,
                          _NEW_BUFFERS |
                          _NEW_COLOR |
                          _NEW_DEPTH |
                          _NEW_FRAG_CLAMP |
                          _NEW_HINT |
                          _NEW_LIGHT |
                          _NEW_LINE |
                          _NEW_MULTISAMPLE |
                          _NEW_POLYGON |
                          _NEW_STENCIL |
                          _NEW_TEXTURE,
                          BRW_NEW_FRAGMENT_PROGRAM |
                          BRW_NEW_REDUCED_PRIMITIVE |
                          BRW_NEW_STATS_WM |
                          BRW_NEW_VUE_MAP_GEOM_OUT);
}

void
brw_wm_populate_key(struct brw_context *brw, struct brw_wm_prog_key *key)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   const struct gl_program *prog = brw->programs[MESA_SHADER_FRAGMENT];
   const struct brw_program *fp = brw_program_const(prog);
   GLuint lookup = 0;
   GLuint line_aa;

   memset(key, 0, sizeof(*key));

   /* Build the index for table lookup
    */
   if (devinfo->ver < 6) {
      struct brw_renderbuffer *depth_irb =
         brw_get_renderbuffer(ctx->DrawBuffer, BUFFER_DEPTH);

      /* _NEW_COLOR */
      if (prog->info.fs.uses_discard || ctx->Color.AlphaEnabled) {
         lookup |= BRW_WM_IZ_PS_KILL_ALPHATEST_BIT;
      }

      if (prog->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_DEPTH)) {
         lookup |= BRW_WM_IZ_PS_COMPUTES_DEPTH_BIT;
      }

      /* _NEW_DEPTH */
      if (depth_irb && ctx->Depth.Test) {
         lookup |= BRW_WM_IZ_DEPTH_TEST_ENABLE_BIT;

         if (brw_depth_writes_enabled(brw))
            lookup |= BRW_WM_IZ_DEPTH_WRITE_ENABLE_BIT;
      }

      /* _NEW_STENCIL | _NEW_BUFFERS */
      if (brw->stencil_enabled) {
         lookup |= BRW_WM_IZ_STENCIL_TEST_ENABLE_BIT;

         if (ctx->Stencil.WriteMask[0] ||
             ctx->Stencil.WriteMask[ctx->Stencil._BackFace])
            lookup |= BRW_WM_IZ_STENCIL_WRITE_ENABLE_BIT;
      }
      key->iz_lookup = lookup;
   }

   line_aa = BRW_WM_AA_NEVER;

   /* _NEW_LINE, _NEW_POLYGON, BRW_NEW_REDUCED_PRIMITIVE */
   if (ctx->Line.SmoothFlag) {
      if (brw->reduced_primitive == GL_LINES) {
         line_aa = BRW_WM_AA_ALWAYS;
      }
      else if (brw->reduced_primitive == GL_TRIANGLES) {
         if (ctx->Polygon.FrontMode == GL_LINE) {
            line_aa = BRW_WM_AA_SOMETIMES;

            if (ctx->Polygon.BackMode == GL_LINE ||
                (ctx->Polygon.CullFlag &&
                 ctx->Polygon.CullFaceMode == GL_BACK))
               line_aa = BRW_WM_AA_ALWAYS;
         }
         else if (ctx->Polygon.BackMode == GL_LINE) {
            line_aa = BRW_WM_AA_SOMETIMES;

            if ((ctx->Polygon.CullFlag &&
                 ctx->Polygon.CullFaceMode == GL_FRONT))
               line_aa = BRW_WM_AA_ALWAYS;
         }
      }
   }

   key->line_aa = line_aa;

   /* _NEW_HINT */
   key->high_quality_derivatives =
      prog->info.uses_fddx_fddy &&
      ctx->Hint.FragmentShaderDerivative == GL_NICEST;

   if (devinfo->ver < 6)
      key->stats_wm = brw->stats_wm;

   /* _NEW_LIGHT */
   key->flat_shade =
      (prog->info.inputs_read & (VARYING_BIT_COL0 | VARYING_BIT_COL1)) &&
      (ctx->Light.ShadeModel == GL_FLAT);

   /* _NEW_FRAG_CLAMP | _NEW_BUFFERS */
   key->clamp_fragment_color = ctx->Color._ClampFragmentColor;

   /* _NEW_TEXTURE */
   brw_populate_base_prog_key(ctx, fp, &key->base);

   /* _NEW_BUFFERS */
   key->nr_color_regions = ctx->DrawBuffer->_NumColorDrawBuffers;

   /* _NEW_COLOR */
   key->force_dual_color_blend = brw->dual_color_blend_by_location &&
      (ctx->Color.BlendEnabled & 1) && ctx->Color._BlendUsesDualSrc & 0x1;

   /* _NEW_MULTISAMPLE, _NEW_BUFFERS */
   key->alpha_to_coverage =  _mesa_is_alpha_to_coverage_enabled(ctx);

   /* _NEW_COLOR, _NEW_BUFFERS */
   key->alpha_test_replicate_alpha =
      ctx->DrawBuffer->_NumColorDrawBuffers > 1 &&
      _mesa_is_alpha_test_enabled(ctx);

   /* _NEW_BUFFERS _NEW_MULTISAMPLE */
   /* Ignore sample qualifier while computing this flag. */
   if (ctx->Multisample.Enabled) {
      key->persample_interp =
         ctx->Multisample.SampleShading &&
         (ctx->Multisample.MinSampleShadingValue *
          _mesa_geometric_samples(ctx->DrawBuffer) > 1);

      key->multisample_fbo = _mesa_geometric_samples(ctx->DrawBuffer) > 1;
   }

   key->ignore_sample_mask_out = !key->multisample_fbo;

   /* BRW_NEW_VUE_MAP_GEOM_OUT */
   if (devinfo->ver < 6 || util_bitcount64(prog->info.inputs_read &
                                             BRW_FS_VARYING_INPUT_MASK) > 16) {
      key->input_slots_valid = brw->vue_map_geom_out.slots_valid;
   }

   /* _NEW_COLOR | _NEW_BUFFERS */
   /* Pre-gfx6, the hardware alpha test always used each render
    * target's alpha to do alpha test, as opposed to render target 0's alpha
    * like GL requires.  Fix that by building the alpha test into the
    * shader, and we'll skip enabling the fixed function alpha test.
    */
   if (devinfo->ver < 6 && ctx->DrawBuffer->_NumColorDrawBuffers > 1 &&
       ctx->Color.AlphaEnabled) {
      key->alpha_test_func = ctx->Color.AlphaFunc;
      key->alpha_test_ref = ctx->Color.AlphaRef;
   }

   /* Whether reads from the framebuffer should behave coherently. */
   key->coherent_fb_fetch = ctx->Extensions.EXT_shader_framebuffer_fetch;
}

void
brw_upload_wm_prog(struct brw_context *brw)
{
   struct brw_wm_prog_key key;
   struct brw_program *fp =
      (struct brw_program *) brw->programs[MESA_SHADER_FRAGMENT];

   if (!brw_wm_state_dirty(brw))
      return;

   brw_wm_populate_key(brw, &key);

   if (brw_search_cache(&brw->cache, BRW_CACHE_FS_PROG, &key, sizeof(key),
                        &brw->wm.base.prog_offset, &brw->wm.base.prog_data,
                        true))
      return;

   if (brw_disk_cache_upload_program(brw, MESA_SHADER_FRAGMENT))
      return;

   fp = (struct brw_program *) brw->programs[MESA_SHADER_FRAGMENT];
   fp->id = key.base.program_string_id;

   ASSERTED bool success = brw_codegen_wm_prog(brw, fp, &key,
                                                   &brw->vue_map_geom_out);
   assert(success);
}

void
brw_wm_populate_default_key(const struct brw_compiler *compiler,
                            struct brw_wm_prog_key *key,
                            struct gl_program *prog)
{
   const struct intel_device_info *devinfo = compiler->devinfo;

   memset(key, 0, sizeof(*key));

   brw_populate_default_base_prog_key(devinfo, brw_program(prog),
                                      &key->base);

   uint64_t outputs_written = prog->info.outputs_written;

   if (devinfo->ver < 6) {
      if (prog->info.fs.uses_discard)
         key->iz_lookup |= BRW_WM_IZ_PS_KILL_ALPHATEST_BIT;

      if (outputs_written & BITFIELD64_BIT(FRAG_RESULT_DEPTH))
         key->iz_lookup |= BRW_WM_IZ_PS_COMPUTES_DEPTH_BIT;

      /* Just assume depth testing. */
      key->iz_lookup |= BRW_WM_IZ_DEPTH_TEST_ENABLE_BIT;
      key->iz_lookup |= BRW_WM_IZ_DEPTH_WRITE_ENABLE_BIT;
   }

   if (devinfo->ver < 6 || util_bitcount64(prog->info.inputs_read &
                                             BRW_FS_VARYING_INPUT_MASK) > 16) {
      key->input_slots_valid = prog->info.inputs_read | VARYING_BIT_POS;
   }

   key->nr_color_regions = util_bitcount64(outputs_written &
         ~(BITFIELD64_BIT(FRAG_RESULT_DEPTH) |
           BITFIELD64_BIT(FRAG_RESULT_STENCIL) |
           BITFIELD64_BIT(FRAG_RESULT_SAMPLE_MASK)));

   /* Whether reads from the framebuffer should behave coherently. */
   key->coherent_fb_fetch = devinfo->ver >= 9;
}

bool
brw_fs_precompile(struct gl_context *ctx, struct gl_program *prog)
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct brw_wm_prog_key key;

   struct brw_program *bfp = brw_program(prog);

   brw_wm_populate_default_key(brw->screen->compiler, &key, prog);

   /* check brw_wm_populate_default_key coherent_fb_fetch setting */
   assert(key.coherent_fb_fetch ==
          ctx->Extensions.EXT_shader_framebuffer_fetch);

   uint32_t old_prog_offset = brw->wm.base.prog_offset;
   struct brw_stage_prog_data *old_prog_data = brw->wm.base.prog_data;

   struct brw_vue_map vue_map;
   if (devinfo->ver < 6) {
      brw_compute_vue_map(&brw->screen->devinfo, &vue_map,
                          prog->info.inputs_read | VARYING_BIT_POS,
                          false, 1);
   }

   bool success = brw_codegen_wm_prog(brw, bfp, &key, &vue_map);

   brw->wm.base.prog_offset = old_prog_offset;
   brw->wm.base.prog_data = old_prog_data;

   return success;
}
