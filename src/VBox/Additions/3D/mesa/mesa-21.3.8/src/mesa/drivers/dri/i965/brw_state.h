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


#ifndef BRW_STATE_H
#define BRW_STATE_H

#include "brw_context.h"

#ifdef __cplusplus
extern "C" {
#endif

enum intel_msaa_layout;

extern const struct brw_tracked_state brw_blend_constant_color;
extern const struct brw_tracked_state brw_clip_unit;
extern const struct brw_tracked_state brw_vs_pull_constants;
extern const struct brw_tracked_state brw_tcs_pull_constants;
extern const struct brw_tracked_state brw_tes_pull_constants;
extern const struct brw_tracked_state brw_gs_pull_constants;
extern const struct brw_tracked_state brw_wm_pull_constants;
extern const struct brw_tracked_state brw_cs_pull_constants;
extern const struct brw_tracked_state brw_constant_buffer;
extern const struct brw_tracked_state brw_curbe_offsets;
extern const struct brw_tracked_state brw_binding_table_pointers;
extern const struct brw_tracked_state brw_depthbuffer;
extern const struct brw_tracked_state brw_recalculate_urb_fence;
extern const struct brw_tracked_state brw_sf_vp;
extern const struct brw_tracked_state brw_cs_texture_surfaces;
extern const struct brw_tracked_state brw_vs_ubo_surfaces;
extern const struct brw_tracked_state brw_vs_image_surfaces;
extern const struct brw_tracked_state brw_tcs_ubo_surfaces;
extern const struct brw_tracked_state brw_tcs_image_surfaces;
extern const struct brw_tracked_state brw_tes_ubo_surfaces;
extern const struct brw_tracked_state brw_tes_image_surfaces;
extern const struct brw_tracked_state brw_gs_ubo_surfaces;
extern const struct brw_tracked_state brw_gs_image_surfaces;
extern const struct brw_tracked_state brw_renderbuffer_surfaces;
extern const struct brw_tracked_state brw_renderbuffer_read_surfaces;
extern const struct brw_tracked_state brw_texture_surfaces;
extern const struct brw_tracked_state brw_wm_binding_table;
extern const struct brw_tracked_state brw_gs_binding_table;
extern const struct brw_tracked_state brw_tes_binding_table;
extern const struct brw_tracked_state brw_tcs_binding_table;
extern const struct brw_tracked_state brw_vs_binding_table;
extern const struct brw_tracked_state brw_wm_ubo_surfaces;
extern const struct brw_tracked_state brw_wm_image_surfaces;
extern const struct brw_tracked_state brw_cs_ubo_surfaces;
extern const struct brw_tracked_state brw_cs_image_surfaces;

extern const struct brw_tracked_state brw_psp_urb_cbs;

extern const struct brw_tracked_state brw_indices;
extern const struct brw_tracked_state brw_index_buffer;
extern const struct brw_tracked_state gfx7_cs_push_constants;
extern const struct brw_tracked_state gfx6_binding_table_pointers;
extern const struct brw_tracked_state gfx6_gs_binding_table;
extern const struct brw_tracked_state gfx6_renderbuffer_surfaces;
extern const struct brw_tracked_state gfx6_sampler_state;
extern const struct brw_tracked_state gfx6_sol_surface;
extern const struct brw_tracked_state gfx6_sf_vp;
extern const struct brw_tracked_state gfx6_urb;
extern const struct brw_tracked_state gfx7_l3_state;
extern const struct brw_tracked_state gfx7_push_constant_space;
extern const struct brw_tracked_state gfx7_urb;
extern const struct brw_tracked_state gfx8_pma_fix;
extern const struct brw_tracked_state brw_cs_work_groups_surface;

void gfx4_emit_raw_pipe_control(struct brw_context *brw, uint32_t flags,
                                struct brw_bo *bo, uint32_t offset,
                                uint64_t imm);
void gfx45_emit_raw_pipe_control(struct brw_context *brw, uint32_t flags,
                                 struct brw_bo *bo, uint32_t offset,
                                 uint64_t imm);
void gfx5_emit_raw_pipe_control(struct brw_context *brw, uint32_t flags,
                                struct brw_bo *bo, uint32_t offset,
                                uint64_t imm);
void gfx6_emit_raw_pipe_control(struct brw_context *brw, uint32_t flags,
                                struct brw_bo *bo, uint32_t offset,
                                uint64_t imm);
void gfx7_emit_raw_pipe_control(struct brw_context *brw, uint32_t flags,
                                struct brw_bo *bo, uint32_t offset,
                                uint64_t imm);
void gfx75_emit_raw_pipe_control(struct brw_context *brw, uint32_t flags,
                                 struct brw_bo *bo, uint32_t offset,
                                 uint64_t imm);
void gfx8_emit_raw_pipe_control(struct brw_context *brw, uint32_t flags,
                                struct brw_bo *bo, uint32_t offset,
                                uint64_t imm);
void gfx9_emit_raw_pipe_control(struct brw_context *brw, uint32_t flags,
                                struct brw_bo *bo, uint32_t offset,
                                uint64_t imm);
void gfx11_emit_raw_pipe_control(struct brw_context *brw, uint32_t flags,
                                 struct brw_bo *bo, uint32_t offset,
                                 uint64_t imm);

static inline bool
brw_state_dirty(const struct brw_context *brw,
                GLuint mesa_flags, uint64_t brw_flags)
{
   return ((brw->NewGLState & mesa_flags) |
           (brw->ctx.NewDriverState & brw_flags)) != 0;
}

/* brw_binding_tables.c */
void brw_upload_binding_table(struct brw_context *brw,
                              uint32_t packet_name,
                              const struct brw_stage_prog_data *prog_data,
                              struct brw_stage_state *stage_state);

/* brw_misc_state.c */
void brw_upload_invariant_state(struct brw_context *brw);
uint32_t
brw_depthbuffer_format(struct brw_context *brw);

void brw_upload_state_base_address(struct brw_context *brw);

/* gfx8_depth_state.c */
void gfx8_write_pma_stall_bits(struct brw_context *brw,
                               uint32_t pma_stall_bits);

/* brw_disk_cache.c */
void brw_disk_cache_init(struct brw_screen *screen);
bool brw_disk_cache_upload_program(struct brw_context *brw,
                                   gl_shader_stage stage);
void brw_disk_cache_write_compute_program(struct brw_context *brw);
void brw_disk_cache_write_render_programs(struct brw_context *brw);

/***********************************************************************
 * brw_state_upload.c
 */
void brw_upload_render_state(struct brw_context *brw);
void brw_render_state_finished(struct brw_context *brw);
void brw_upload_compute_state(struct brw_context *brw);
void brw_compute_state_finished(struct brw_context *brw);
void brw_init_state(struct brw_context *brw);
void brw_destroy_state(struct brw_context *brw);
void brw_emit_select_pipeline(struct brw_context *brw,
                              enum brw_pipeline pipeline);
void brw_enable_obj_preemption(struct brw_context *brw, bool enable);

static inline void
brw_select_pipeline(struct brw_context *brw, enum brw_pipeline pipeline)
{
   if (unlikely(brw->last_pipeline != pipeline)) {
      assert(pipeline < BRW_NUM_PIPELINES);
      brw_emit_select_pipeline(brw, pipeline);
      brw->last_pipeline = pipeline;
   }
}

/***********************************************************************
 * brw_program_cache.c
 */

void brw_upload_cache(struct brw_cache *cache,
                      enum brw_cache_id cache_id,
                      const void *key,
                      GLuint key_sz,
                      const void *data,
                      GLuint data_sz,
                      const void *aux,
                      GLuint aux_sz,
                      uint32_t *out_offset, void *out_aux);

bool brw_search_cache(struct brw_cache *cache, enum brw_cache_id cache_id,
                      const void *key, GLuint key_size, uint32_t *inout_offset,
                      void *inout_aux, bool flag_state);

const void *brw_find_previous_compile(struct brw_cache *cache,
                                      enum brw_cache_id cache_id,
                                      unsigned program_string_id);

void brw_program_cache_check_size(struct brw_context *brw);

void brw_init_caches( struct brw_context *brw );
void brw_destroy_caches( struct brw_context *brw );

void brw_print_program_cache(struct brw_context *brw);

enum brw_cache_id brw_stage_cache_id(gl_shader_stage stage);

/* brw_batch.c */
void brw_require_statebuffer_space(struct brw_context *brw, int size);
void *brw_state_batch(struct brw_context *brw,
                      int size, int alignment, uint32_t *out_offset);

/* brw_wm_surface_state.c */
uint32_t brw_get_surface_tiling_bits(uint32_t tiling);
uint32_t brw_get_surface_num_multisamples(unsigned num_samples);
enum isl_format brw_isl_format_for_mesa_format(mesa_format mesa_format);

GLuint translate_tex_target(GLenum target);

enum isl_format translate_tex_format(struct brw_context *brw,
                                     mesa_format mesa_format,
                                     GLenum srgb_decode);

int brw_get_texture_swizzle(const struct gl_context *ctx,
                            const struct gl_texture_object *t);

void brw_emit_buffer_surface_state(struct brw_context *brw,
                                   uint32_t *out_offset,
                                   struct brw_bo *bo,
                                   unsigned buffer_offset,
                                   unsigned surface_format,
                                   unsigned buffer_size,
                                   unsigned pitch,
                                   unsigned reloc_flags);

/* brw_sampler_state.c */
void brw_emit_sampler_state(struct brw_context *brw,
                            uint32_t *sampler_state,
                            uint32_t batch_offset_for_sampler_state,
                            unsigned min_filter,
                            unsigned mag_filter,
                            unsigned mip_filter,
                            unsigned max_anisotropy,
                            unsigned address_rounding,
                            unsigned wrap_s,
                            unsigned wrap_t,
                            unsigned wrap_r,
                            unsigned base_level,
                            unsigned min_lod,
                            unsigned max_lod,
                            int lod_bias,
                            unsigned shadow_function,
                            bool non_normalized_coordinates,
                            uint32_t border_color_offset);

/* gfx6_constant_state.c */
void
brw_populate_constant_data(struct brw_context *brw,
                           const struct gl_program *prog,
                           const struct brw_stage_state *stage_state,
                           void *dst,
                           const uint32_t *param,
                           unsigned nr_params);
void
brw_upload_pull_constants(struct brw_context *brw,
                          GLbitfield64 brw_new_constbuf,
                          const struct gl_program *prog,
                          struct brw_stage_state *stage_state,
                          const struct brw_stage_prog_data *prog_data);
void
brw_upload_cs_push_constants(struct brw_context *brw,
                             const struct gl_program *prog,
                             const struct brw_cs_prog_data *cs_prog_data,
                             struct brw_stage_state *stage_state);

/* gfx7_vs_state.c */
void
gfx7_upload_constant_state(struct brw_context *brw,
                           const struct brw_stage_state *stage_state,
                           bool active, unsigned opcode);

/* brw_clip.c */
void brw_upload_clip_prog(struct brw_context *brw);

/* brw_sf.c */
void brw_upload_sf_prog(struct brw_context *brw);

bool brw_is_drawing_points(const struct brw_context *brw);
bool brw_is_drawing_lines(const struct brw_context *brw);

/* gfx7_l3_state.c */
void
gfx7_restore_default_l3_config(struct brw_context *brw);

static inline bool
use_state_point_size(const struct brw_context *brw)
{
   const struct gl_context *ctx = &brw->ctx;

   /* Section 14.4 (Points) of the OpenGL 4.5 specification says:
    *
    *    "If program point size mode is enabled, the derived point size is
    *     taken from the (potentially clipped) shader built-in gl_PointSize
    *     written by:
    *
    *        * the geometry shader, if active;
    *        * the tessellation evaluation shader, if active and no
    *          geometry shader is active;
    *        * the vertex shader, otherwise
    *
    *    and clamped to the implementation-dependent point size range.  If
    *    the value written to gl_PointSize is less than or equal to zero,
    *    or if no value was written to gl_PointSize, results are undefined.
    *    If program point size mode is disabled, the derived point size is
    *    specified with the command
    *
    *       void PointSize(float size);
    *
    *    size specifies the requested size of a point.  The default value
    *    is 1.0."
    *
    * The rules for GLES come from the ES 3.2, OES_geometry_point_size, and
    * OES_tessellation_point_size specifications.  To summarize: if the last
    * stage before rasterization is a GS or TES, then use gl_PointSize from
    * the shader if written.  Otherwise, use 1.0.  If the last stage is a
    * vertex shader, use gl_PointSize, or it is undefined.
    *
    * We can combine these rules into a single condition for both APIs.
    * Using the state point size when the last shader stage doesn't write
    * gl_PointSize satisfies GL's requirements, as it's undefined.  Because
    * ES doesn't have a PointSize() command, the state point size will
    * remain 1.0, satisfying the ES default value in the GS/TES case, and
    * the VS case (1.0 works for "undefined").  Mesa sets the program point
    * mode flag to always-enabled in ES, so we can safely check that, and
    * it'll be ignored for ES.
    *
    * _NEW_PROGRAM | _NEW_POINT
    * BRW_NEW_VUE_MAP_GEOM_OUT
    */
   return (!ctx->VertexProgram.PointSizeEnabled && !ctx->Point._Attenuated) ||
          (brw->vue_map_geom_out.slots_valid & VARYING_BIT_PSIZ) == 0;
}

void brw_copy_pipeline_atoms(struct brw_context *brw,
                             enum brw_pipeline pipeline,
                             const struct brw_tracked_state **atoms,
                             int num_atoms);
void gfx4_init_atoms(struct brw_context *brw);
void gfx45_init_atoms(struct brw_context *brw);
void gfx5_init_atoms(struct brw_context *brw);
void gfx6_init_atoms(struct brw_context *brw);
void gfx7_init_atoms(struct brw_context *brw);
void gfx75_init_atoms(struct brw_context *brw);
void gfx8_init_atoms(struct brw_context *brw);
void gfx9_init_atoms(struct brw_context *brw);
void gfx11_init_atoms(struct brw_context *brw);

/* Memory Object Control State:
 * Specifying zero for L3 means "uncached in L3", at least on Haswell
 * and Baytrail, since there are no PTE flags for setting L3 cacheability.
 * On Ivybridge, the PTEs do have a cache-in-L3 bit, so setting MOCS to 0
 * may still respect that.
 */
#define GFX7_MOCS_L3                    1

/* Ivybridge only: cache in LLC.
 * Specifying zero here means to use the PTE values set by the kernel;
 * non-zero overrides the PTE values.
 */
#define IVB_MOCS_LLC                    (1 << 1)

/* Baytrail only: snoop in CPU cache */
#define BYT_MOCS_SNOOP                  (1 << 1)

/* Haswell only: LLC/eLLC controls (write-back or uncached).
 * Specifying zero here means to use the PTE values set by the kernel,
 * which is useful since it offers additional control (write-through
 * cacheing and age).  Non-zero overrides the PTE values.
 */
#define HSW_MOCS_UC_LLC_UC_ELLC         (1 << 1)
#define HSW_MOCS_WB_LLC_WB_ELLC         (2 << 1)
#define HSW_MOCS_UC_LLC_WB_ELLC         (3 << 1)

/* Broadwell: these defines always use all available caches (L3, LLC, eLLC),
 * and let you force write-back (WB) or write-through (WT) caching, or leave
 * it up to the page table entry (PTE) specified by the kernel.
 */
#define BDW_MOCS_WB  0x78
#define BDW_MOCS_WT  0x58
#define BDW_MOCS_PTE 0x18

/* Skylake: MOCS is now an index into an array of 62 different caching
 * configurations programmed by the kernel.
 */
/* TC=LLC/eLLC, LeCC=WB, LRUM=3, L3CC=WB */
#define SKL_MOCS_WB  (2 << 1)
/* TC=LLC/eLLC, LeCC=PTE, LRUM=3, L3CC=WB */
#define SKL_MOCS_PTE (1 << 1)

/* Cannonlake: MOCS is now an index into an array of 62 different caching
 * configurations programmed by the kernel.
 */
/* TC=LLC/eLLC, LeCC=WB, LRUM=3, L3CC=WB */
#define CNL_MOCS_WB  (2 << 1)
/* TC=LLC/eLLC, LeCC=PTE, LRUM=3, L3CC=WB */
#define CNL_MOCS_PTE (1 << 1)

/* Ice Lake uses same MOCS settings as Cannonlake */
/* TC=LLC/eLLC, LeCC=WB, LRUM=3, L3CC=WB */
#define ICL_MOCS_WB  (2 << 1)
/* TC=LLC/eLLC, LeCC=PTE, LRUM=3, L3CC=WB */
#define ICL_MOCS_PTE (1 << 1)

uint32_t brw_get_bo_mocs(const struct intel_device_info *devinfo,
                         struct brw_bo *bo);

#ifdef __cplusplus
}
#endif

#endif
