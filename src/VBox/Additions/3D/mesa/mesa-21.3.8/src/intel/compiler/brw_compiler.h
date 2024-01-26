/*
 * Copyright © 2010 - 2015 Intel Corporation
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

#ifndef BRW_COMPILER_H
#define BRW_COMPILER_H

#include <stdio.h>
#include "dev/intel_device_info.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "util/ralloc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ra_regs;
struct nir_shader;
struct brw_program;

typedef struct nir_shader nir_shader;

struct brw_compiler {
   const struct intel_device_info *devinfo;

   struct {
      struct ra_regs *regs;

      /**
       * Array of the ra classes for the unaligned contiguous register
       * block sizes used.
       */
      struct ra_class **classes;
   } vec4_reg_set;

   struct {
      struct ra_regs *regs;

      /**
       * Array of the ra classes for the unaligned contiguous register
       * block sizes used, indexed by register size.
       */
      struct ra_class *classes[16];

      /**
       * ra class for the aligned barycentrics we use for PLN, which doesn't
       * appear in *classes.
       */
      struct ra_class *aligned_bary_class;
   } fs_reg_sets[3];

   void (*shader_debug_log)(void *, unsigned *id, const char *str, ...) PRINTFLIKE(3, 4);
   void (*shader_perf_log)(void *, unsigned *id, const char *str, ...) PRINTFLIKE(3, 4);

   bool scalar_stage[MESA_ALL_SHADER_STAGES];
   bool use_tcs_8_patch;
   struct gl_shader_compiler_options glsl_compiler_options[MESA_ALL_SHADER_STAGES];

   /**
    * Apply workarounds for SIN and COS output range problems.
    * This can negatively impact performance.
    */
   bool precise_trig;

   /**
    * Is 3DSTATE_CONSTANT_*'s Constant Buffer 0 relative to Dynamic State
    * Base Address?  (If not, it's a normal GPU address.)
    */
   bool constant_buffer_0_is_relative;

   /**
    * Whether or not the driver supports pull constants.  If not, the compiler
    * will attempt to push everything.
    */
   bool supports_pull_constants;

   /**
    * Whether or not the driver supports NIR shader constants.  This controls
    * whether nir_opt_large_constants will be run.
    */
   bool supports_shader_constants;

   /**
    * Whether or not the driver wants uniform params to be compacted by the
    * back-end compiler.
    */
   bool compact_params;

   /**
    * Whether or not the driver wants variable group size to be lowered by the
    * back-end compiler.
    */
   bool lower_variable_group_size;

   /**
    * Whether indirect UBO loads should use the sampler or go through the
    * data/constant cache.  For the sampler, UBO surface states have to be set
    * up with VK_FORMAT_R32G32B32A32_FLOAT whereas if it's going through the
    * constant or data cache, UBOs must use VK_FORMAT_RAW.
    */
   bool indirect_ubos_use_sampler;
};

#define brw_shader_debug_log(compiler, data, fmt, ... ) do {    \
   static unsigned id = 0;                                      \
   compiler->shader_debug_log(data, &id, fmt, ##__VA_ARGS__);   \
} while (0)

#define brw_shader_perf_log(compiler, data, fmt, ... ) do {     \
   static unsigned id = 0;                                      \
   compiler->shader_perf_log(data, &id, fmt, ##__VA_ARGS__);    \
} while (0)

/**
 * We use a constant subgroup size of 32.  It really only needs to be a
 * maximum and, since we do SIMD32 for compute shaders in some cases, it
 * needs to be at least 32.  SIMD8 and SIMD16 shaders will still claim a
 * subgroup size of 32 but will act as if 16 or 24 of those channels are
 * disabled.
 */
#define BRW_SUBGROUP_SIZE 32

static inline bool
brw_shader_stage_is_bindless(gl_shader_stage stage)
{
   return stage >= MESA_SHADER_RAYGEN &&
          stage <= MESA_SHADER_CALLABLE;
}

/**
 * Program key structures.
 *
 * When drawing, we look for the currently bound shaders in the program
 * cache.  This is essentially a hash table lookup, and these are the keys.
 *
 * Sometimes OpenGL features specified as state need to be simulated via
 * shader code, due to a mismatch between the API and the hardware.  This
 * is often referred to as "non-orthagonal state" or "NOS".  We store NOS
 * in the program key so it's considered when searching for a program.  If
 * we haven't seen a particular combination before, we have to recompile a
 * new specialized version.
 *
 * Shader compilation should not look up state in gl_context directly, but
 * instead use the copy in the program key.  This guarantees recompiles will
 * happen correctly.
 *
 *  @{
 */

enum PACKED gfx6_gather_sampler_wa {
   WA_SIGN = 1,      /* whether we need to sign extend */
   WA_8BIT = 2,      /* if we have an 8bit format needing wa */
   WA_16BIT = 4,     /* if we have a 16bit format needing wa */
};

/**
 * Sampler information needed by VS, WM, and GS program cache keys.
 */
struct brw_sampler_prog_key_data {
   /**
    * EXT_texture_swizzle and DEPTH_TEXTURE_MODE swizzles.
    */
   uint16_t swizzles[MAX_SAMPLERS];

   uint32_t gl_clamp_mask[3];

   /**
    * For RG32F, gather4's channel select is broken.
    */
   uint32_t gather_channel_quirk_mask;

   /**
    * Whether this sampler uses the compressed multisample surface layout.
    */
   uint32_t compressed_multisample_layout_mask;

   /**
    * Whether this sampler is using 16x multisampling. If so fetching from
    * this sampler will be handled with a different instruction, ld2dms_w
    * instead of ld2dms.
    */
   uint32_t msaa_16;

   /**
    * For Sandybridge, which shader w/a we need for gather quirks.
    */
   enum gfx6_gather_sampler_wa gfx6_gather_wa[MAX_SAMPLERS];

   /**
    * Texture units that have a YUV image bound.
    */
   uint32_t y_u_v_image_mask;
   uint32_t y_uv_image_mask;
   uint32_t yx_xuxv_image_mask;
   uint32_t xy_uxvx_image_mask;
   uint32_t ayuv_image_mask;
   uint32_t xyuv_image_mask;
   uint32_t bt709_mask;
   uint32_t bt2020_mask;

   /* Scale factor for each texture. */
   float scale_factors[32];
};

/** An enum representing what kind of input gl_SubgroupSize is. */
enum PACKED brw_subgroup_size_type
{
   BRW_SUBGROUP_SIZE_API_CONSTANT,     /**< Default Vulkan behavior */
   BRW_SUBGROUP_SIZE_UNIFORM,          /**< OpenGL behavior */
   BRW_SUBGROUP_SIZE_VARYING,          /**< VK_EXT_subgroup_size_control */

   /* These enums are specifically chosen so that the value of the enum is
    * also the subgroup size.  If any new values are added, they must respect
    * this invariant.
    */
   BRW_SUBGROUP_SIZE_REQUIRE_8   = 8,  /**< VK_EXT_subgroup_size_control */
   BRW_SUBGROUP_SIZE_REQUIRE_16  = 16, /**< VK_EXT_subgroup_size_control */
   BRW_SUBGROUP_SIZE_REQUIRE_32  = 32, /**< VK_EXT_subgroup_size_control */
};

struct brw_base_prog_key {
   unsigned program_string_id;

   enum brw_subgroup_size_type subgroup_size_type;
   bool robust_buffer_access;
   struct brw_sampler_prog_key_data tex;
};

/**
 * The VF can't natively handle certain types of attributes, such as GL_FIXED
 * or most 10_10_10_2 types.  These flags enable various VS workarounds to
 * "fix" attributes at the beginning of shaders.
 */
#define BRW_ATTRIB_WA_COMPONENT_MASK    7  /* mask for GL_FIXED scale channel count */
#define BRW_ATTRIB_WA_NORMALIZE     8   /* normalize in shader */
#define BRW_ATTRIB_WA_BGRA          16  /* swap r/b channels in shader */
#define BRW_ATTRIB_WA_SIGN          32  /* interpret as signed in shader */
#define BRW_ATTRIB_WA_SCALE         64  /* interpret as scaled in shader */

/**
 * OpenGL attribute slots fall in [0, VERT_ATTRIB_MAX - 1] with the range
 * [VERT_ATTRIB_GENERIC0, VERT_ATTRIB_MAX - 1] reserved for up to 16 user
 * input vertex attributes. In Vulkan, we expose up to 28 user vertex input
 * attributes that are mapped to slots also starting at VERT_ATTRIB_GENERIC0.
 */
#define MAX_GL_VERT_ATTRIB     VERT_ATTRIB_MAX
#define MAX_VK_VERT_ATTRIB     (VERT_ATTRIB_GENERIC0 + 28)

/**
 * Max number of binding table entries used for stream output.
 *
 * From the OpenGL 3.0 spec, table 6.44 (Transform Feedback State), the
 * minimum value of MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS is 64.
 *
 * On Gfx6, the size of transform feedback data is limited not by the number
 * of components but by the number of binding table entries we set aside.  We
 * use one binding table entry for a float, one entry for a vector, and one
 * entry per matrix column.  Since the only way we can communicate our
 * transform feedback capabilities to the client is via
 * MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, we need to plan for the
 * worst case, in which all the varyings are floats, so we use up one binding
 * table entry per component.  Therefore we need to set aside at least 64
 * binding table entries for use by transform feedback.
 *
 * Note: since we don't currently pack varyings, it is currently impossible
 * for the client to actually use up all of these binding table entries--if
 * all of their varyings were floats, they would run out of varying slots and
 * fail to link.  But that's a bug, so it seems prudent to go ahead and
 * allocate the number of binding table entries we will need once the bug is
 * fixed.
 */
#define BRW_MAX_SOL_BINDINGS 64

/** The program key for Vertex Shaders. */
struct brw_vs_prog_key {
   struct brw_base_prog_key base;

   /**
    * Per-attribute workaround flags
    *
    * For each attribute, a combination of BRW_ATTRIB_WA_*.
    *
    * For OpenGL, where we expose a maximum of 16 user input atttributes
    * we only need up to VERT_ATTRIB_MAX slots, however, in Vulkan
    * slots preceding VERT_ATTRIB_GENERIC0 are unused and we can
    * expose up to 28 user input vertex attributes that are mapped to slots
    * starting at VERT_ATTRIB_GENERIC0, so this array needs to be large
    * enough to hold this many slots.
    */
   uint8_t gl_attrib_wa_flags[MAX2(MAX_GL_VERT_ATTRIB, MAX_VK_VERT_ATTRIB)];

   bool copy_edgeflag:1;

   bool clamp_vertex_color:1;

   /**
    * How many user clipping planes are being uploaded to the vertex shader as
    * push constants.
    *
    * These are used for lowering legacy gl_ClipVertex/gl_Position clipping to
    * clip distances.
    */
   unsigned nr_userclip_plane_consts:4;

   /**
    * For pre-Gfx6 hardware, a bitfield indicating which texture coordinates
    * are going to be replaced with point coordinates (as a consequence of a
    * call to glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE)).  Because
    * our SF thread requires exact matching between VS outputs and FS inputs,
    * these texture coordinates will need to be unconditionally included in
    * the VUE, even if they aren't written by the vertex shader.
    */
   uint8_t point_coord_replace;
};

/** The program key for Tessellation Control Shaders. */
struct brw_tcs_prog_key
{
   struct brw_base_prog_key base;

   GLenum tes_primitive_mode;

   unsigned input_vertices;

   /** A bitfield of per-patch outputs written. */
   uint32_t patch_outputs_written;

   /** A bitfield of per-vertex outputs written. */
   uint64_t outputs_written;

   bool quads_workaround;
};

/** The program key for Tessellation Evaluation Shaders. */
struct brw_tes_prog_key
{
   struct brw_base_prog_key base;

   /** A bitfield of per-patch inputs read. */
   uint32_t patch_inputs_read;

   /** A bitfield of per-vertex inputs read. */
   uint64_t inputs_read;

   /**
    * How many user clipping planes are being uploaded to the tessellation
    * evaluation shader as push constants.
    *
    * These are used for lowering legacy gl_ClipVertex/gl_Position clipping to
    * clip distances.
    */
   unsigned nr_userclip_plane_consts:4;
};

/** The program key for Geometry Shaders. */
struct brw_gs_prog_key
{
   struct brw_base_prog_key base;

   /**
    * How many user clipping planes are being uploaded to the geometry shader
    * as push constants.
    *
    * These are used for lowering legacy gl_ClipVertex/gl_Position clipping to
    * clip distances.
    */
   unsigned nr_userclip_plane_consts:4;
};

enum brw_sf_primitive {
   BRW_SF_PRIM_POINTS = 0,
   BRW_SF_PRIM_LINES = 1,
   BRW_SF_PRIM_TRIANGLES = 2,
   BRW_SF_PRIM_UNFILLED_TRIS = 3,
};

struct brw_sf_prog_key {
   uint64_t attrs;
   bool contains_flat_varying;
   unsigned char interp_mode[65]; /* BRW_VARYING_SLOT_COUNT */
   uint8_t point_sprite_coord_replace;
   enum brw_sf_primitive primitive:2;
   bool do_twoside_color:1;
   bool frontface_ccw:1;
   bool do_point_sprite:1;
   bool do_point_coord:1;
   bool sprite_origin_lower_left:1;
   bool userclip_active:1;
};

enum brw_clip_mode {
   BRW_CLIP_MODE_NORMAL             = 0,
   BRW_CLIP_MODE_CLIP_ALL           = 1,
   BRW_CLIP_MODE_CLIP_NON_REJECTED  = 2,
   BRW_CLIP_MODE_REJECT_ALL         = 3,
   BRW_CLIP_MODE_ACCEPT_ALL         = 4,
   BRW_CLIP_MODE_KERNEL_CLIP        = 5,
};

enum brw_clip_fill_mode {
   BRW_CLIP_FILL_MODE_LINE = 0,
   BRW_CLIP_FILL_MODE_POINT = 1,
   BRW_CLIP_FILL_MODE_FILL = 2,
   BRW_CLIP_FILL_MODE_CULL = 3,
};

/* Note that if unfilled primitives are being emitted, we have to fix
 * up polygon offset and flatshading at this point:
 */
struct brw_clip_prog_key {
   uint64_t attrs;
   bool contains_flat_varying;
   bool contains_noperspective_varying;
   unsigned char interp_mode[65]; /* BRW_VARYING_SLOT_COUNT */
   unsigned primitive:4;
   unsigned nr_userclip:4;
   bool pv_first:1;
   bool do_unfilled:1;
   enum brw_clip_fill_mode fill_cw:2;  /* includes cull information */
   enum brw_clip_fill_mode fill_ccw:2; /* includes cull information */
   bool offset_cw:1;
   bool offset_ccw:1;
   bool copy_bfc_cw:1;
   bool copy_bfc_ccw:1;
   enum brw_clip_mode clip_mode:3;

   float offset_factor;
   float offset_units;
   float offset_clamp;
};

/* A big lookup table is used to figure out which and how many
 * additional regs will inserted before the main payload in the WM
 * program execution.  These mainly relate to depth and stencil
 * processing and the early-depth-test optimization.
 */
enum brw_wm_iz_bits {
   BRW_WM_IZ_PS_KILL_ALPHATEST_BIT     = 0x1,
   BRW_WM_IZ_PS_COMPUTES_DEPTH_BIT     = 0x2,
   BRW_WM_IZ_DEPTH_WRITE_ENABLE_BIT    = 0x4,
   BRW_WM_IZ_DEPTH_TEST_ENABLE_BIT     = 0x8,
   BRW_WM_IZ_STENCIL_WRITE_ENABLE_BIT  = 0x10,
   BRW_WM_IZ_STENCIL_TEST_ENABLE_BIT   = 0x20,
   BRW_WM_IZ_BIT_MAX                   = 0x40
};

enum brw_wm_aa_enable {
   BRW_WM_AA_NEVER,
   BRW_WM_AA_SOMETIMES,
   BRW_WM_AA_ALWAYS
};

/** The program key for Fragment/Pixel Shaders. */
struct brw_wm_prog_key {
   struct brw_base_prog_key base;

   /* Some collection of BRW_WM_IZ_* */
   uint8_t iz_lookup;
   bool stats_wm:1;
   bool flat_shade:1;
   unsigned nr_color_regions:5;
   bool alpha_test_replicate_alpha:1;
   bool alpha_to_coverage:1;
   bool clamp_fragment_color:1;
   bool persample_interp:1;
   bool multisample_fbo:1;
   bool frag_coord_adds_sample_pos:1;
   enum brw_wm_aa_enable line_aa:2;
   bool high_quality_derivatives:1;
   bool force_dual_color_blend:1;
   bool coherent_fb_fetch:1;
   bool ignore_sample_mask_out:1;
   bool coarse_pixel:1;

   uint8_t color_outputs_valid;
   uint64_t input_slots_valid;
   GLenum alpha_test_func;          /* < For Gfx4/5 MRT alpha test */
   float alpha_test_ref;
};

struct brw_cs_prog_key {
   struct brw_base_prog_key base;
};

struct brw_bs_prog_key {
   struct brw_base_prog_key base;
};

struct brw_ff_gs_prog_key {
   uint64_t attrs;

   /**
    * Hardware primitive type being drawn, e.g. _3DPRIM_TRILIST.
    */
   unsigned primitive:8;

   unsigned pv_first:1;
   unsigned need_gs_prog:1;

   /**
    * Number of varyings that are output to transform feedback.
    */
   unsigned num_transform_feedback_bindings:7; /* 0-BRW_MAX_SOL_BINDINGS */

   /**
    * Map from the index of a transform feedback binding table entry to the
    * gl_varying_slot that should be streamed out through that binding table
    * entry.
    */
   unsigned char transform_feedback_bindings[BRW_MAX_SOL_BINDINGS];

   /**
    * Map from the index of a transform feedback binding table entry to the
    * swizzles that should be used when streaming out data through that
    * binding table entry.
    */
   unsigned char transform_feedback_swizzles[BRW_MAX_SOL_BINDINGS];
};

/* brw_any_prog_key is any of the keys that map to an API stage */
union brw_any_prog_key {
   struct brw_base_prog_key base;
   struct brw_vs_prog_key vs;
   struct brw_tcs_prog_key tcs;
   struct brw_tes_prog_key tes;
   struct brw_gs_prog_key gs;
   struct brw_wm_prog_key wm;
   struct brw_cs_prog_key cs;
   struct brw_bs_prog_key bs;
};

/*
 * Image metadata structure as laid out in the shader parameter
 * buffer.  Entries have to be 16B-aligned for the vec4 back-end to be
 * able to use them.  That's okay because the padding and any unused
 * entries [most of them except when we're doing untyped surface
 * access] will be removed by the uniform packing pass.
 */
#define BRW_IMAGE_PARAM_OFFSET_OFFSET           0
#define BRW_IMAGE_PARAM_SIZE_OFFSET             4
#define BRW_IMAGE_PARAM_STRIDE_OFFSET           8
#define BRW_IMAGE_PARAM_TILING_OFFSET           12
#define BRW_IMAGE_PARAM_SWIZZLING_OFFSET        16
#define BRW_IMAGE_PARAM_SIZE                    20

struct brw_image_param {
   /** Offset applied to the X and Y surface coordinates. */
   uint32_t offset[2];

   /** Surface X, Y and Z dimensions. */
   uint32_t size[3];

   /** X-stride in bytes, Y-stride in pixels, horizontal slice stride in
    * pixels, vertical slice stride in pixels.
    */
   uint32_t stride[4];

   /** Log2 of the tiling modulus in the X, Y and Z dimension. */
   uint32_t tiling[3];

   /**
    * Right shift to apply for bit 6 address swizzling.  Two different
    * swizzles can be specified and will be applied one after the other.  The
    * resulting address will be:
    *
    *  addr' = addr ^ ((1 << 6) & ((addr >> swizzling[0]) ^
    *                              (addr >> swizzling[1])))
    *
    * Use \c 0xff if any of the swizzles is not required.
    */
   uint32_t swizzling[2];
};

/** Max number of render targets in a shader */
#define BRW_MAX_DRAW_BUFFERS 8

/**
 * Binding table index for the first gfx6 SOL binding.
 */
#define BRW_GFX6_SOL_BINDING_START 0

/**
 * Stride in bytes between shader_time entries.
 *
 * We separate entries by a cacheline to reduce traffic between EUs writing to
 * different entries.
 */
#define BRW_SHADER_TIME_STRIDE 64

struct brw_ubo_range
{
   uint16_t block;
   uint8_t start;
   uint8_t length;
};

/* We reserve the first 2^16 values for builtins */
#define BRW_PARAM_IS_BUILTIN(param) (((param) & 0xffff0000) == 0)

enum brw_param_builtin {
   BRW_PARAM_BUILTIN_ZERO,

   BRW_PARAM_BUILTIN_CLIP_PLANE_0_X,
   BRW_PARAM_BUILTIN_CLIP_PLANE_0_Y,
   BRW_PARAM_BUILTIN_CLIP_PLANE_0_Z,
   BRW_PARAM_BUILTIN_CLIP_PLANE_0_W,
   BRW_PARAM_BUILTIN_CLIP_PLANE_1_X,
   BRW_PARAM_BUILTIN_CLIP_PLANE_1_Y,
   BRW_PARAM_BUILTIN_CLIP_PLANE_1_Z,
   BRW_PARAM_BUILTIN_CLIP_PLANE_1_W,
   BRW_PARAM_BUILTIN_CLIP_PLANE_2_X,
   BRW_PARAM_BUILTIN_CLIP_PLANE_2_Y,
   BRW_PARAM_BUILTIN_CLIP_PLANE_2_Z,
   BRW_PARAM_BUILTIN_CLIP_PLANE_2_W,
   BRW_PARAM_BUILTIN_CLIP_PLANE_3_X,
   BRW_PARAM_BUILTIN_CLIP_PLANE_3_Y,
   BRW_PARAM_BUILTIN_CLIP_PLANE_3_Z,
   BRW_PARAM_BUILTIN_CLIP_PLANE_3_W,
   BRW_PARAM_BUILTIN_CLIP_PLANE_4_X,
   BRW_PARAM_BUILTIN_CLIP_PLANE_4_Y,
   BRW_PARAM_BUILTIN_CLIP_PLANE_4_Z,
   BRW_PARAM_BUILTIN_CLIP_PLANE_4_W,
   BRW_PARAM_BUILTIN_CLIP_PLANE_5_X,
   BRW_PARAM_BUILTIN_CLIP_PLANE_5_Y,
   BRW_PARAM_BUILTIN_CLIP_PLANE_5_Z,
   BRW_PARAM_BUILTIN_CLIP_PLANE_5_W,
   BRW_PARAM_BUILTIN_CLIP_PLANE_6_X,
   BRW_PARAM_BUILTIN_CLIP_PLANE_6_Y,
   BRW_PARAM_BUILTIN_CLIP_PLANE_6_Z,
   BRW_PARAM_BUILTIN_CLIP_PLANE_6_W,
   BRW_PARAM_BUILTIN_CLIP_PLANE_7_X,
   BRW_PARAM_BUILTIN_CLIP_PLANE_7_Y,
   BRW_PARAM_BUILTIN_CLIP_PLANE_7_Z,
   BRW_PARAM_BUILTIN_CLIP_PLANE_7_W,

   BRW_PARAM_BUILTIN_TESS_LEVEL_OUTER_X,
   BRW_PARAM_BUILTIN_TESS_LEVEL_OUTER_Y,
   BRW_PARAM_BUILTIN_TESS_LEVEL_OUTER_Z,
   BRW_PARAM_BUILTIN_TESS_LEVEL_OUTER_W,
   BRW_PARAM_BUILTIN_TESS_LEVEL_INNER_X,
   BRW_PARAM_BUILTIN_TESS_LEVEL_INNER_Y,

   BRW_PARAM_BUILTIN_PATCH_VERTICES_IN,

   BRW_PARAM_BUILTIN_BASE_WORK_GROUP_ID_X,
   BRW_PARAM_BUILTIN_BASE_WORK_GROUP_ID_Y,
   BRW_PARAM_BUILTIN_BASE_WORK_GROUP_ID_Z,
   BRW_PARAM_BUILTIN_SUBGROUP_ID,
   BRW_PARAM_BUILTIN_WORK_GROUP_SIZE_X,
   BRW_PARAM_BUILTIN_WORK_GROUP_SIZE_Y,
   BRW_PARAM_BUILTIN_WORK_GROUP_SIZE_Z,
   BRW_PARAM_BUILTIN_WORK_DIM,
};

#define BRW_PARAM_BUILTIN_CLIP_PLANE(idx, comp) \
   (BRW_PARAM_BUILTIN_CLIP_PLANE_0_X + ((idx) << 2) + (comp))

#define BRW_PARAM_BUILTIN_IS_CLIP_PLANE(param)  \
   ((param) >= BRW_PARAM_BUILTIN_CLIP_PLANE_0_X && \
    (param) <= BRW_PARAM_BUILTIN_CLIP_PLANE_7_W)

#define BRW_PARAM_BUILTIN_CLIP_PLANE_IDX(param) \
   (((param) - BRW_PARAM_BUILTIN_CLIP_PLANE_0_X) >> 2)

#define BRW_PARAM_BUILTIN_CLIP_PLANE_COMP(param) \
   (((param) - BRW_PARAM_BUILTIN_CLIP_PLANE_0_X) & 0x3)

enum brw_shader_reloc_id {
   BRW_SHADER_RELOC_CONST_DATA_ADDR_LOW,
   BRW_SHADER_RELOC_CONST_DATA_ADDR_HIGH,
   BRW_SHADER_RELOC_SHADER_START_OFFSET,
   BRW_SHADER_RELOC_RESUME_SBT_ADDR_LOW,
   BRW_SHADER_RELOC_RESUME_SBT_ADDR_HIGH,
};

enum brw_shader_reloc_type {
   /** An arbitrary 32-bit value */
   BRW_SHADER_RELOC_TYPE_U32,
   /** A MOV instruction with an immediate source */
   BRW_SHADER_RELOC_TYPE_MOV_IMM,
};

/** Represents a code relocation
 *
 * Relocatable constants are immediates in the code which we want to be able
 * to replace post-compile with the actual value.
 */
struct brw_shader_reloc {
   /** The 32-bit ID of the relocatable constant */
   uint32_t id;

   /** Type of this relocation */
   enum brw_shader_reloc_type type;

   /** The offset in the shader to the relocated value
    *
    * For MOV_IMM relocs, this is an offset to the MOV instruction.  This
    * allows us to do some sanity checking while we update the value.
    */
   uint32_t offset;

   /** Value to be added to the relocated value before it is written */
   uint32_t delta;
};

/** A value to write to a relocation */
struct brw_shader_reloc_value {
   /** The 32-bit ID of the relocatable constant */
   uint32_t id;

   /** The value with which to replace the relocated immediate */
   uint32_t value;
};

struct brw_stage_prog_data {
   struct {
      /** size of our binding table. */
      uint32_t size_bytes;

      /** @{
       * surface indices for the various groups of surfaces
       */
      uint32_t pull_constants_start;
      uint32_t texture_start;
      uint32_t gather_texture_start;
      uint32_t ubo_start;
      uint32_t ssbo_start;
      uint32_t image_start;
      uint32_t shader_time_start;
      uint32_t plane_start[3];
      /** @} */
   } binding_table;

   struct brw_ubo_range ubo_ranges[4];

   GLuint nr_params;       /**< number of float params/constants */
   GLuint nr_pull_params;

   gl_shader_stage stage;

   /* zero_push_reg is a bitfield which indicates what push registers (if any)
    * should be zeroed by SW at the start of the shader.  The corresponding
    * push_reg_mask_param specifies the param index (in 32-bit units) where
    * the actual runtime 64-bit mask will be pushed.  The shader will zero
    * push reg i if
    *
    *    reg_used & zero_push_reg & ~*push_reg_mask_param & (1ull << i)
    *
    * If this field is set, brw_compiler::compact_params must be false.
    */
   uint64_t zero_push_reg;
   unsigned push_reg_mask_param;

   unsigned curb_read_length;
   unsigned total_scratch;
   unsigned total_shared;

   unsigned program_size;

   unsigned const_data_size;
   unsigned const_data_offset;

   unsigned num_relocs;
   const struct brw_shader_reloc *relocs;

   /** Does this program pull from any UBO or other constant buffers? */
   bool has_ubo_pull;

   /**
    * Register where the thread expects to find input data from the URB
    * (typically uniforms, followed by vertex or fragment attributes).
    */
   unsigned dispatch_grf_start_reg;

   bool use_alt_mode; /**< Use ALT floating point mode?  Otherwise, IEEE. */

   /* 32-bit identifiers for all push/pull parameters.  These can be anything
    * the driver wishes them to be; the core of the back-end compiler simply
    * re-arranges them.  The one restriction is that the bottom 2^16 values
    * are reserved for builtins defined in the brw_param_builtin enum defined
    * above.
    */
   uint32_t *param;
   uint32_t *pull_param;

   /* Whether shader uses atomic operations. */
   bool uses_atomic_load_store;
};

static inline uint32_t *
brw_stage_prog_data_add_params(struct brw_stage_prog_data *prog_data,
                               unsigned nr_new_params)
{
   unsigned old_nr_params = prog_data->nr_params;
   prog_data->nr_params += nr_new_params;
   prog_data->param = reralloc(ralloc_parent(prog_data->param),
                               prog_data->param, uint32_t,
                               prog_data->nr_params);
   return prog_data->param + old_nr_params;
}

enum brw_barycentric_mode {
   BRW_BARYCENTRIC_PERSPECTIVE_PIXEL       = 0,
   BRW_BARYCENTRIC_PERSPECTIVE_CENTROID    = 1,
   BRW_BARYCENTRIC_PERSPECTIVE_SAMPLE      = 2,
   BRW_BARYCENTRIC_NONPERSPECTIVE_PIXEL    = 3,
   BRW_BARYCENTRIC_NONPERSPECTIVE_CENTROID = 4,
   BRW_BARYCENTRIC_NONPERSPECTIVE_SAMPLE   = 5,
   BRW_BARYCENTRIC_MODE_COUNT              = 6
};
#define BRW_BARYCENTRIC_NONPERSPECTIVE_BITS \
   ((1 << BRW_BARYCENTRIC_NONPERSPECTIVE_PIXEL) | \
    (1 << BRW_BARYCENTRIC_NONPERSPECTIVE_CENTROID) | \
    (1 << BRW_BARYCENTRIC_NONPERSPECTIVE_SAMPLE))

enum brw_pixel_shader_computed_depth_mode {
   BRW_PSCDEPTH_OFF   = 0, /* PS does not compute depth */
   BRW_PSCDEPTH_ON    = 1, /* PS computes depth; no guarantee about value */
   BRW_PSCDEPTH_ON_GE = 2, /* PS guarantees output depth >= source depth */
   BRW_PSCDEPTH_ON_LE = 3, /* PS guarantees output depth <= source depth */
};

/* Data about a particular attempt to compile a program.  Note that
 * there can be many of these, each in a different GL state
 * corresponding to a different brw_wm_prog_key struct, with different
 * compiled programs.
 */
struct brw_wm_prog_data {
   struct brw_stage_prog_data base;

   GLuint num_varying_inputs;

   uint8_t reg_blocks_8;
   uint8_t reg_blocks_16;
   uint8_t reg_blocks_32;

   uint8_t dispatch_grf_start_reg_16;
   uint8_t dispatch_grf_start_reg_32;
   uint32_t prog_offset_16;
   uint32_t prog_offset_32;

   struct {
      /** @{
       * surface indices the WM-specific surfaces
       */
      uint32_t render_target_read_start;
      /** @} */
   } binding_table;

   uint8_t computed_depth_mode;
   bool computed_stencil;

   bool early_fragment_tests;
   bool post_depth_coverage;
   bool inner_coverage;
   bool dispatch_8;
   bool dispatch_16;
   bool dispatch_32;
   bool dual_src_blend;
   bool persample_dispatch;
   bool uses_pos_offset;
   bool uses_omask;
   bool uses_kill;
   bool uses_src_depth;
   bool uses_src_w;
   bool uses_depth_w_coefficients;
   bool uses_sample_mask;
   bool has_render_target_reads;
   bool has_side_effects;
   bool pulls_bary;

   bool contains_flat_varying;
   bool contains_noperspective_varying;

   /**
    * Shader is ran at the coarse pixel shading dispatch rate (3DSTATE_CPS).
    */
   bool per_coarse_pixel_dispatch;

   /**
    * Mask of which interpolation modes are required by the fragment shader.
    * Used in hardware setup on gfx6+.
    */
   uint32_t barycentric_interp_modes;

   /**
    * Mask of which FS inputs are marked flat by the shader source.  This is
    * needed for setting up 3DSTATE_SF/SBE.
    */
   uint32_t flat_inputs;

   /**
    * The FS inputs
    */
   uint64_t inputs;

   /* Mapping of VUE slots to interpolation modes.
    * Used by the Gfx4-5 clip/sf/wm stages.
    */
   unsigned char interp_mode[65]; /* BRW_VARYING_SLOT_COUNT */

   /**
    * Map from gl_varying_slot to the position within the FS setup data
    * payload where the varying's attribute vertex deltas should be delivered.
    * For varying slots that are not used by the FS, the value is -1.
    */
   int urb_setup[VARYING_SLOT_MAX];

   /**
    * Cache structure into the urb_setup array above that contains the
    * attribute numbers of active varyings out of urb_setup.
    * The actual count is stored in urb_setup_attribs_count.
    */
   uint8_t urb_setup_attribs[VARYING_SLOT_MAX];
   uint8_t urb_setup_attribs_count;
};

/** Returns the SIMD width corresponding to a given KSP index
 *
 * The "Variable Pixel Dispatch" table in the PRM (which can be found, for
 * example in Vol. 7 of the SKL PRM) has a mapping from dispatch widths to
 * kernel start pointer (KSP) indices that is based on what dispatch widths
 * are enabled.  This function provides, effectively, the reverse mapping.
 *
 * If the given KSP is valid with respect to the SIMD8/16/32 enables, a SIMD
 * width of 8, 16, or 32 is returned.  If the KSP is invalid, 0 is returned.
 */
static inline unsigned
brw_fs_simd_width_for_ksp(unsigned ksp_idx, bool simd8_enabled,
                          bool simd16_enabled, bool simd32_enabled)
{
   /* This function strictly ignores contiguous dispatch */
   switch (ksp_idx) {
   case 0:
      return simd8_enabled ? 8 :
             (simd16_enabled && !simd32_enabled) ? 16 :
             (simd32_enabled && !simd16_enabled) ? 32 : 0;
   case 1:
      return (simd32_enabled && (simd16_enabled || simd8_enabled)) ? 32 : 0;
   case 2:
      return (simd16_enabled && (simd32_enabled || simd8_enabled)) ? 16 : 0;
   default:
      unreachable("Invalid KSP index");
   }
}

#define brw_wm_state_simd_width_for_ksp(wm_state, ksp_idx) \
   brw_fs_simd_width_for_ksp((ksp_idx), (wm_state)._8PixelDispatchEnable, \
                             (wm_state)._16PixelDispatchEnable, \
                             (wm_state)._32PixelDispatchEnable)

#define brw_wm_state_has_ksp(wm_state, ksp_idx) \
   (brw_wm_state_simd_width_for_ksp((wm_state), (ksp_idx)) != 0)

static inline uint32_t
_brw_wm_prog_data_prog_offset(const struct brw_wm_prog_data *prog_data,
                              unsigned simd_width)
{
   switch (simd_width) {
   case 8: return 0;
   case 16: return prog_data->prog_offset_16;
   case 32: return prog_data->prog_offset_32;
   default: return 0;
   }
}

#define brw_wm_prog_data_prog_offset(prog_data, wm_state, ksp_idx) \
   _brw_wm_prog_data_prog_offset(prog_data, \
      brw_wm_state_simd_width_for_ksp(wm_state, ksp_idx))

static inline uint8_t
_brw_wm_prog_data_dispatch_grf_start_reg(const struct brw_wm_prog_data *prog_data,
                                         unsigned simd_width)
{
   switch (simd_width) {
   case 8: return prog_data->base.dispatch_grf_start_reg;
   case 16: return prog_data->dispatch_grf_start_reg_16;
   case 32: return prog_data->dispatch_grf_start_reg_32;
   default: return 0;
   }
}

#define brw_wm_prog_data_dispatch_grf_start_reg(prog_data, wm_state, ksp_idx) \
   _brw_wm_prog_data_dispatch_grf_start_reg(prog_data, \
      brw_wm_state_simd_width_for_ksp(wm_state, ksp_idx))

static inline uint8_t
_brw_wm_prog_data_reg_blocks(const struct brw_wm_prog_data *prog_data,
                             unsigned simd_width)
{
   switch (simd_width) {
   case 8: return prog_data->reg_blocks_8;
   case 16: return prog_data->reg_blocks_16;
   case 32: return prog_data->reg_blocks_32;
   default: return 0;
   }
}

#define brw_wm_prog_data_reg_blocks(prog_data, wm_state, ksp_idx) \
   _brw_wm_prog_data_reg_blocks(prog_data, \
      brw_wm_state_simd_width_for_ksp(wm_state, ksp_idx))

struct brw_push_const_block {
   unsigned dwords;     /* Dword count, not reg aligned */
   unsigned regs;
   unsigned size;       /* Bytes, register aligned */
};

struct brw_cs_prog_data {
   struct brw_stage_prog_data base;

   unsigned local_size[3];

   /* Program offsets for the 8/16/32 SIMD variants.  Multiple variants are
    * kept when using variable group size, and the right one can only be
    * decided at dispatch time.
    */
   unsigned prog_offset[3];

   /* Bitmask indicating which program offsets are valid. */
   unsigned prog_mask;

   /* Bitmask indicating which programs have spilled. */
   unsigned prog_spilled;

   bool uses_barrier;
   bool uses_num_work_groups;
   bool uses_inline_data;
   bool uses_btd_stack_ids;

   struct {
      struct brw_push_const_block cross_thread;
      struct brw_push_const_block per_thread;
   } push;

   struct {
      /** @{
       * surface indices the CS-specific surfaces
       */
      uint32_t work_groups_start;
      /** @} */
   } binding_table;
};

static inline uint32_t
brw_cs_prog_data_prog_offset(const struct brw_cs_prog_data *prog_data,
                             unsigned dispatch_width)
{
   assert(dispatch_width == 8 ||
          dispatch_width == 16 ||
          dispatch_width == 32);
   const unsigned index = dispatch_width / 16;
   assert(prog_data->prog_mask & (1 << index));
   return prog_data->prog_offset[index];
}

struct brw_bs_prog_data {
   struct brw_stage_prog_data base;

   /** SIMD size of the root shader */
   uint8_t simd_size;

   /** Maximum stack size of all shaders */
   uint32_t max_stack_size;

   /** Offset into the shader where the resume SBT is located */
   uint32_t resume_sbt_offset;
};

struct brw_ff_gs_prog_data {
   unsigned urb_read_length;
   unsigned total_grf;

   /**
    * Gfx6 transform feedback: Amount by which the streaming vertex buffer
    * indices should be incremented each time the GS is invoked.
    */
   unsigned svbi_postincrement_value;
};

/**
 * Enum representing the i965-specific vertex results that don't correspond
 * exactly to any element of gl_varying_slot.  The values of this enum are
 * assigned such that they don't conflict with gl_varying_slot.
 */
typedef enum
{
   BRW_VARYING_SLOT_NDC = VARYING_SLOT_MAX,
   BRW_VARYING_SLOT_PAD,
   /**
    * Technically this is not a varying but just a placeholder that
    * compile_sf_prog() inserts into its VUE map to cause the gl_PointCoord
    * builtin variable to be compiled correctly. see compile_sf_prog() for
    * more info.
    */
   BRW_VARYING_SLOT_PNTC,
   BRW_VARYING_SLOT_COUNT
} brw_varying_slot;

/**
 * We always program SF to start reading at an offset of 1 (2 varying slots)
 * from the start of the vertex URB entry.  This causes it to skip:
 * - VARYING_SLOT_PSIZ and BRW_VARYING_SLOT_NDC on gfx4-5
 * - VARYING_SLOT_PSIZ and VARYING_SLOT_POS on gfx6+
 */
#define BRW_SF_URB_ENTRY_READ_OFFSET 1

/**
 * Bitmask indicating which fragment shader inputs represent varyings (and
 * hence have to be delivered to the fragment shader by the SF/SBE stage).
 */
#define BRW_FS_VARYING_INPUT_MASK \
   (BITFIELD64_RANGE(0, VARYING_SLOT_MAX) & \
    ~VARYING_BIT_POS & ~VARYING_BIT_FACE)

/**
 * Data structure recording the relationship between the gl_varying_slot enum
 * and "slots" within the vertex URB entry (VUE).  A "slot" is defined as a
 * single octaword within the VUE (128 bits).
 *
 * Note that each BRW register contains 256 bits (2 octawords), so when
 * accessing the VUE in URB_NOSWIZZLE mode, each register corresponds to two
 * consecutive VUE slots.  When accessing the VUE in URB_INTERLEAVED mode (as
 * in a vertex shader), each register corresponds to a single VUE slot, since
 * it contains data for two separate vertices.
 */
struct brw_vue_map {
   /**
    * Bitfield representing all varying slots that are (a) stored in this VUE
    * map, and (b) actually written by the shader.  Does not include any of
    * the additional varying slots defined in brw_varying_slot.
    */
   uint64_t slots_valid;

   /**
    * Is this VUE map for a separate shader pipeline?
    *
    * Separable programs (GL_ARB_separate_shader_objects) can be mixed and matched
    * without the linker having a chance to dead code eliminate unused varyings.
    *
    * This means that we have to use a fixed slot layout, based on the output's
    * location field, rather than assigning slots in a compact contiguous block.
    */
   bool separate;

   /**
    * Map from gl_varying_slot value to VUE slot.  For gl_varying_slots that are
    * not stored in a slot (because they are not written, or because
    * additional processing is applied before storing them in the VUE), the
    * value is -1.
    */
   signed char varying_to_slot[VARYING_SLOT_TESS_MAX];

   /**
    * Map from VUE slot to gl_varying_slot value.  For slots that do not
    * directly correspond to a gl_varying_slot, the value comes from
    * brw_varying_slot.
    *
    * For slots that are not in use, the value is BRW_VARYING_SLOT_PAD.
    */
   signed char slot_to_varying[VARYING_SLOT_TESS_MAX];

   /**
    * Total number of VUE slots in use
    */
   int num_slots;

   /**
    * Number of per-patch VUE slots. Only valid for tessellation control
    * shader outputs and tessellation evaluation shader inputs.
    */
   int num_per_patch_slots;

   /**
    * Number of per-vertex VUE slots. Only valid for tessellation control
    * shader outputs and tessellation evaluation shader inputs.
    */
   int num_per_vertex_slots;
};

void brw_print_vue_map(FILE *fp, const struct brw_vue_map *vue_map,
                       gl_shader_stage stage);

/**
 * Convert a VUE slot number into a byte offset within the VUE.
 */
static inline GLuint brw_vue_slot_to_offset(GLuint slot)
{
   return 16*slot;
}

/**
 * Convert a vertex output (brw_varying_slot) into a byte offset within the
 * VUE.
 */
static inline
GLuint brw_varying_to_offset(const struct brw_vue_map *vue_map, GLuint varying)
{
   return brw_vue_slot_to_offset(vue_map->varying_to_slot[varying]);
}

void brw_compute_vue_map(const struct intel_device_info *devinfo,
                         struct brw_vue_map *vue_map,
                         uint64_t slots_valid,
                         bool separate_shader,
                         uint32_t pos_slots);

void brw_compute_tess_vue_map(struct brw_vue_map *const vue_map,
                              uint64_t slots_valid,
                              uint32_t is_patch);

/* brw_interpolation_map.c */
void brw_setup_vue_interpolation(const struct brw_vue_map *vue_map,
                                 struct nir_shader *nir,
                                 struct brw_wm_prog_data *prog_data);

enum shader_dispatch_mode {
   DISPATCH_MODE_4X1_SINGLE = 0,
   DISPATCH_MODE_4X2_DUAL_INSTANCE = 1,
   DISPATCH_MODE_4X2_DUAL_OBJECT = 2,
   DISPATCH_MODE_SIMD8 = 3,

   DISPATCH_MODE_TCS_SINGLE_PATCH = 0,
   DISPATCH_MODE_TCS_8_PATCH = 2,
};

/**
 * @defgroup Tessellator parameter enumerations.
 *
 * These correspond to the hardware values in 3DSTATE_TE, and are provided
 * as part of the tessellation evaluation shader.
 *
 * @{
 */
enum brw_tess_partitioning {
   BRW_TESS_PARTITIONING_INTEGER         = 0,
   BRW_TESS_PARTITIONING_ODD_FRACTIONAL  = 1,
   BRW_TESS_PARTITIONING_EVEN_FRACTIONAL = 2,
};

enum brw_tess_output_topology {
   BRW_TESS_OUTPUT_TOPOLOGY_POINT   = 0,
   BRW_TESS_OUTPUT_TOPOLOGY_LINE    = 1,
   BRW_TESS_OUTPUT_TOPOLOGY_TRI_CW  = 2,
   BRW_TESS_OUTPUT_TOPOLOGY_TRI_CCW = 3,
};

enum brw_tess_domain {
   BRW_TESS_DOMAIN_QUAD    = 0,
   BRW_TESS_DOMAIN_TRI     = 1,
   BRW_TESS_DOMAIN_ISOLINE = 2,
};
/** @} */

struct brw_vue_prog_data {
   struct brw_stage_prog_data base;
   struct brw_vue_map vue_map;

   /** Should the hardware deliver input VUE handles for URB pull loads? */
   bool include_vue_handles;

   GLuint urb_read_length;
   GLuint total_grf;

   uint32_t clip_distance_mask;
   uint32_t cull_distance_mask;

   /* Used for calculating urb partitions.  In the VS, this is the size of the
    * URB entry used for both input and output to the thread.  In the GS, this
    * is the size of the URB entry used for output.
    */
   GLuint urb_entry_size;

   enum shader_dispatch_mode dispatch_mode;
};

struct brw_vs_prog_data {
   struct brw_vue_prog_data base;

   GLbitfield64 inputs_read;
   GLbitfield64 double_inputs_read;

   unsigned nr_attribute_slots;

   bool uses_vertexid;
   bool uses_instanceid;
   bool uses_is_indexed_draw;
   bool uses_firstvertex;
   bool uses_baseinstance;
   bool uses_drawid;
};

struct brw_tcs_prog_data
{
   struct brw_vue_prog_data base;

   /** Should the non-SINGLE_PATCH payload provide primitive ID? */
   bool include_primitive_id;

   /** Number vertices in output patch */
   int instances;

   /** Track patch count threshold */
   int patch_count_threshold;
};


struct brw_tes_prog_data
{
   struct brw_vue_prog_data base;

   enum brw_tess_partitioning partitioning;
   enum brw_tess_output_topology output_topology;
   enum brw_tess_domain domain;
};

struct brw_gs_prog_data
{
   struct brw_vue_prog_data base;

   unsigned vertices_in;

   /**
    * Size of an output vertex, measured in HWORDS (32 bytes).
    */
   unsigned output_vertex_size_hwords;

   unsigned output_topology;

   /**
    * Size of the control data (cut bits or StreamID bits), in hwords (32
    * bytes).  0 if there is no control data.
    */
   unsigned control_data_header_size_hwords;

   /**
    * Format of the control data (either GFX7_GS_CONTROL_DATA_FORMAT_GSCTL_SID
    * if the control data is StreamID bits, or
    * GFX7_GS_CONTROL_DATA_FORMAT_GSCTL_CUT if the control data is cut bits).
    * Ignored if control_data_header_size is 0.
    */
   unsigned control_data_format;

   bool include_primitive_id;

   /**
    * The number of vertices emitted, if constant - otherwise -1.
    */
   int static_vertex_count;

   int invocations;

   /**
    * Gfx6: Provoking vertex convention for odd-numbered triangles
    * in tristrips.
    */
   GLuint pv_first:1;

   /**
    * Gfx6: Number of varyings that are output to transform feedback.
    */
   GLuint num_transform_feedback_bindings:7; /* 0-BRW_MAX_SOL_BINDINGS */

   /**
    * Gfx6: Map from the index of a transform feedback binding table entry to the
    * gl_varying_slot that should be streamed out through that binding table
    * entry.
    */
   unsigned char transform_feedback_bindings[64 /* BRW_MAX_SOL_BINDINGS */];

   /**
    * Gfx6: Map from the index of a transform feedback binding table entry to the
    * swizzles that should be used when streaming out data through that
    * binding table entry.
    */
   unsigned char transform_feedback_swizzles[64 /* BRW_MAX_SOL_BINDINGS */];
};

struct brw_sf_prog_data {
   uint32_t urb_read_length;
   uint32_t total_grf;

   /* Each vertex may have upto 12 attributes, 4 components each,
    * except WPOS which requires only 2.  (11*4 + 2) == 44 ==> 11
    * rows.
    *
    * Actually we use 4 for each, so call it 12 rows.
    */
   unsigned urb_entry_size;
};

struct brw_clip_prog_data {
   uint32_t curb_read_length;	/* user planes? */
   uint32_t clip_mode;
   uint32_t urb_read_length;
   uint32_t total_grf;
};

/* brw_any_prog_data is prog_data for any stage that maps to an API stage */
union brw_any_prog_data {
   struct brw_stage_prog_data base;
   struct brw_vue_prog_data vue;
   struct brw_vs_prog_data vs;
   struct brw_tcs_prog_data tcs;
   struct brw_tes_prog_data tes;
   struct brw_gs_prog_data gs;
   struct brw_wm_prog_data wm;
   struct brw_cs_prog_data cs;
   struct brw_bs_prog_data bs;
};

#define DEFINE_PROG_DATA_DOWNCAST(STAGE, CHECK)                            \
static inline struct brw_##STAGE##_prog_data *                             \
brw_##STAGE##_prog_data(struct brw_stage_prog_data *prog_data)             \
{                                                                          \
   if (prog_data)                                                          \
      assert(CHECK);                                                       \
   return (struct brw_##STAGE##_prog_data *) prog_data;                    \
}                                                                          \
static inline const struct brw_##STAGE##_prog_data *                       \
brw_##STAGE##_prog_data_const(const struct brw_stage_prog_data *prog_data) \
{                                                                          \
   if (prog_data)                                                          \
      assert(CHECK);                                                       \
   return (const struct brw_##STAGE##_prog_data *) prog_data;              \
}

DEFINE_PROG_DATA_DOWNCAST(vs,  prog_data->stage == MESA_SHADER_VERTEX)
DEFINE_PROG_DATA_DOWNCAST(tcs, prog_data->stage == MESA_SHADER_TESS_CTRL)
DEFINE_PROG_DATA_DOWNCAST(tes, prog_data->stage == MESA_SHADER_TESS_EVAL)
DEFINE_PROG_DATA_DOWNCAST(gs,  prog_data->stage == MESA_SHADER_GEOMETRY)
DEFINE_PROG_DATA_DOWNCAST(wm,  prog_data->stage == MESA_SHADER_FRAGMENT)
DEFINE_PROG_DATA_DOWNCAST(cs,  prog_data->stage == MESA_SHADER_COMPUTE)
DEFINE_PROG_DATA_DOWNCAST(bs,  brw_shader_stage_is_bindless(prog_data->stage))

DEFINE_PROG_DATA_DOWNCAST(vue, prog_data->stage == MESA_SHADER_VERTEX ||
                               prog_data->stage == MESA_SHADER_TESS_CTRL ||
                               prog_data->stage == MESA_SHADER_TESS_EVAL ||
                               prog_data->stage == MESA_SHADER_GEOMETRY)

/* These are not really brw_stage_prog_data. */
DEFINE_PROG_DATA_DOWNCAST(ff_gs, true)
DEFINE_PROG_DATA_DOWNCAST(clip,  true)
DEFINE_PROG_DATA_DOWNCAST(sf,    true)
#undef DEFINE_PROG_DATA_DOWNCAST

struct brw_compile_stats {
   uint32_t dispatch_width; /**< 0 for vec4 */
   uint32_t instructions;
   uint32_t sends;
   uint32_t loops;
   uint32_t cycles;
   uint32_t spills;
   uint32_t fills;
};

/** @} */

struct brw_compiler *
brw_compiler_create(void *mem_ctx, const struct intel_device_info *devinfo);

/**
 * Returns a compiler configuration for use with disk shader cache
 *
 * This value only needs to change for settings that can cause different
 * program generation between two runs on the same hardware.
 *
 * For example, it doesn't need to be different for gen 8 and gen 9 hardware,
 * but it does need to be different if INTEL_DEBUG=nocompact is or isn't used.
 */
uint64_t
brw_get_compiler_config_value(const struct brw_compiler *compiler);

unsigned
brw_prog_data_size(gl_shader_stage stage);

unsigned
brw_prog_key_size(gl_shader_stage stage);

void
brw_prog_key_set_id(union brw_any_prog_key *key, gl_shader_stage, unsigned id);

/**
 * Parameters for compiling a vertex shader.
 *
 * Some of these will be modified during the shader compilation.
 */
struct brw_compile_vs_params {
   nir_shader *nir;

   const struct brw_vs_prog_key *key;
   struct brw_vs_prog_data *prog_data;

   bool edgeflag_is_last; /* true for gallium */
   bool shader_time;
   int shader_time_index;

   struct brw_compile_stats *stats;

   void *log_data;

   char *error_str;

   /* If unset, DEBUG_VS is used. */
   uint64_t debug_flag;
};

/**
 * Compile a vertex shader.
 *
 * Returns the final assembly and updates the parameters structure.
 */
const unsigned *
brw_compile_vs(const struct brw_compiler *compiler,
               void *mem_ctx,
               struct brw_compile_vs_params *params);

/**
 * Compile a tessellation control shader.
 *
 * Returns the final assembly and the program's size.
 */
const unsigned *
brw_compile_tcs(const struct brw_compiler *compiler,
                void *log_data,
                void *mem_ctx,
                const struct brw_tcs_prog_key *key,
                struct brw_tcs_prog_data *prog_data,
                nir_shader *nir,
                int shader_time_index,
                struct brw_compile_stats *stats,
                char **error_str);

/**
 * Compile a tessellation evaluation shader.
 *
 * Returns the final assembly and the program's size.
 */
const unsigned *
brw_compile_tes(const struct brw_compiler *compiler, void *log_data,
                void *mem_ctx,
                const struct brw_tes_prog_key *key,
                const struct brw_vue_map *input_vue_map,
                struct brw_tes_prog_data *prog_data,
                nir_shader *nir,
                int shader_time_index,
                struct brw_compile_stats *stats,
                char **error_str);

/**
 * Compile a vertex shader.
 *
 * Returns the final assembly and the program's size.
 */
const unsigned *
brw_compile_gs(const struct brw_compiler *compiler, void *log_data,
               void *mem_ctx,
               const struct brw_gs_prog_key *key,
               struct brw_gs_prog_data *prog_data,
               nir_shader *nir,
               int shader_time_index,
               struct brw_compile_stats *stats,
               char **error_str);

/**
 * Compile a strips and fans shader.
 *
 * This is a fixed-function shader determined entirely by the shader key and
 * a VUE map.
 *
 * Returns the final assembly and the program's size.
 */
const unsigned *
brw_compile_sf(const struct brw_compiler *compiler,
               void *mem_ctx,
               const struct brw_sf_prog_key *key,
               struct brw_sf_prog_data *prog_data,
               struct brw_vue_map *vue_map,
               unsigned *final_assembly_size);

/**
 * Compile a clipper shader.
 *
 * This is a fixed-function shader determined entirely by the shader key and
 * a VUE map.
 *
 * Returns the final assembly and the program's size.
 */
const unsigned *
brw_compile_clip(const struct brw_compiler *compiler,
                 void *mem_ctx,
                 const struct brw_clip_prog_key *key,
                 struct brw_clip_prog_data *prog_data,
                 struct brw_vue_map *vue_map,
                 unsigned *final_assembly_size);

/**
 * Parameters for compiling a fragment shader.
 *
 * Some of these will be modified during the shader compilation.
 */
struct brw_compile_fs_params {
   nir_shader *nir;

   const struct brw_wm_prog_key *key;
   struct brw_wm_prog_data *prog_data;
   const struct brw_vue_map *vue_map;

   bool shader_time;
   int shader_time_index8;
   int shader_time_index16;
   int shader_time_index32;

   bool allow_spilling;
   bool use_rep_send;

   struct brw_compile_stats *stats;

   void *log_data;

   char *error_str;

   /* If unset, DEBUG_WM is used. */
   uint64_t debug_flag;
};

/**
 * Compile a fragment shader.
 *
 * Returns the final assembly and updates the parameters structure.
 */
const unsigned *
brw_compile_fs(const struct brw_compiler *compiler,
               void *mem_ctx,
               struct brw_compile_fs_params *params);

/**
 * Parameters for compiling a compute shader.
 *
 * Some of these will be modified during the shader compilation.
 */
struct brw_compile_cs_params {
   nir_shader *nir;

   const struct brw_cs_prog_key *key;
   struct brw_cs_prog_data *prog_data;

   bool shader_time;
   int shader_time_index;

   struct brw_compile_stats *stats;

   void *log_data;

   char *error_str;

   /* If unset, DEBUG_CS is used. */
   uint64_t debug_flag;
};

/**
 * Compile a compute shader.
 *
 * Returns the final assembly and updates the parameters structure.
 */
const unsigned *
brw_compile_cs(const struct brw_compiler *compiler,
               void *mem_ctx,
               struct brw_compile_cs_params *params);

/**
 * Compile a Ray Tracing shader.
 *
 * Returns the final assembly and the program's size.
 */
const unsigned *
brw_compile_bs(const struct brw_compiler *compiler, void *log_data,
               void *mem_ctx,
               const struct brw_bs_prog_key *key,
               struct brw_bs_prog_data *prog_data,
               struct nir_shader *shader,
               unsigned num_resume_shaders,
               struct nir_shader **resume_shaders,
               struct brw_compile_stats *stats,
               char **error_str);

/**
 * Compile a fixed function geometry shader.
 *
 * Returns the final assembly and the program's size.
 */
const unsigned *
brw_compile_ff_gs_prog(struct brw_compiler *compiler,
		       void *mem_ctx,
		       const struct brw_ff_gs_prog_key *key,
		       struct brw_ff_gs_prog_data *prog_data,
		       struct brw_vue_map *vue_map,
		       unsigned *final_assembly_size);

void brw_debug_key_recompile(const struct brw_compiler *c, void *log,
                             gl_shader_stage stage,
                             const struct brw_base_prog_key *old_key,
                             const struct brw_base_prog_key *key);

/* Shared Local Memory Size is specified as powers of two,
 * and also have a Gen-dependent minimum value if not zero.
 */
static inline uint32_t
intel_calculate_slm_size(unsigned gen, uint32_t bytes)
{
   assert(bytes <= 64 * 1024);
   if (bytes > 0)
      return MAX2(util_next_power_of_two(bytes), gen >= 9 ? 1024 : 4096);
   else
      return 0;
}

static inline uint32_t
encode_slm_size(unsigned gen, uint32_t bytes)
{
   uint32_t slm_size = 0;

   /* Shared Local Memory is specified as powers of two, and encoded in
    * INTERFACE_DESCRIPTOR_DATA with the following representations:
    *
    * Size   | 0 kB | 1 kB | 2 kB | 4 kB | 8 kB | 16 kB | 32 kB | 64 kB |
    * -------------------------------------------------------------------
    * Gfx7-8 |    0 | none | none |    1 |    2 |     4 |     8 |    16 |
    * -------------------------------------------------------------------
    * Gfx9+  |    0 |    1 |    2 |    3 |    4 |     5 |     6 |     7 |
    */

   if (bytes > 0) {
      slm_size = intel_calculate_slm_size(gen, bytes);
      assert(util_is_power_of_two_nonzero(slm_size));

      if (gen >= 9) {
         /* Turn an exponent of 10 (1024 kB) into 1. */
         assert(slm_size >= 1024);
         slm_size = ffs(slm_size) - 10;
      } else {
         assert(slm_size >= 4096);
         /* Convert to the pre-Gfx9 representation. */
         slm_size = slm_size / 4096;
      }
   }

   return slm_size;
}

unsigned
brw_cs_push_const_total_size(const struct brw_cs_prog_data *cs_prog_data,
                             unsigned threads);

void
brw_write_shader_relocs(const struct intel_device_info *devinfo,
                        void *program,
                        const struct brw_stage_prog_data *prog_data,
                        struct brw_shader_reloc_value *values,
                        unsigned num_values);

struct brw_cs_dispatch_info {
   uint32_t group_size;
   uint32_t simd_size;
   uint32_t threads;

   /* RightExecutionMask field used in GPGPU_WALKER. */
   uint32_t right_mask;
};

/**
 * Get the dispatch information for a shader to be used with GPGPU_WALKER and
 * similar instructions.
 *
 * If override_local_size is not NULL, it must to point to a 3-element that
 * will override the value from prog_data->local_size.  This is used by
 * ARB_compute_variable_group_size, where the size is set only at dispatch
 * time (so prog_data is outdated).
 */
struct brw_cs_dispatch_info
brw_cs_get_dispatch_info(const struct intel_device_info *devinfo,
                         const struct brw_cs_prog_data *prog_data,
                         const unsigned *override_local_size);

/**
 * Return true if the given shader stage is dispatched contiguously by the
 * relevant fixed function starting from channel 0 of the SIMD thread, which
 * implies that the dispatch mask of a thread can be assumed to have the form
 * '2^n - 1' for some n.
 */
static inline bool
brw_stage_has_packed_dispatch(ASSERTED const struct intel_device_info *devinfo,
                              gl_shader_stage stage,
                              const struct brw_stage_prog_data *prog_data)
{
   /* The code below makes assumptions about the hardware's thread dispatch
    * behavior that could be proven wrong in future generations -- Make sure
    * to do a full test run with brw_fs_test_dispatch_packing() hooked up to
    * the NIR front-end before changing this assertion.
    */
   assert(devinfo->ver <= 12);

   switch (stage) {
   case MESA_SHADER_FRAGMENT: {
      /* The PSD discards subspans coming in with no lit samples, which in the
       * per-pixel shading case implies that each subspan will either be fully
       * lit (due to the VMask being used to allow derivative computations),
       * or not dispatched at all.  In per-sample dispatch mode individual
       * samples from the same subspan have a fixed relative location within
       * the SIMD thread, so dispatch of unlit samples cannot be avoided in
       * general and we should return false.
       */
      const struct brw_wm_prog_data *wm_prog_data =
         (const struct brw_wm_prog_data *)prog_data;
      return !wm_prog_data->persample_dispatch;
   }
   case MESA_SHADER_COMPUTE:
      /* Compute shaders will be spawned with either a fully enabled dispatch
       * mask or with whatever bottom/right execution mask was given to the
       * GPGPU walker command to be used along the workgroup edges -- In both
       * cases the dispatch mask is required to be tightly packed for our
       * invocation index calculations to work.
       */
      return true;
   default:
      /* Most remaining fixed functions are limited to use a packed dispatch
       * mask due to the hardware representation of the dispatch mask as a
       * single counter representing the number of enabled channels.
       */
      return true;
   }
}

/**
 * Computes the first varying slot in the URB produced by the previous stage
 * that is used in the next stage. We do this by testing the varying slots in
 * the previous stage's vue map against the inputs read in the next stage.
 *
 * Note that:
 *
 * - Each URB offset contains two varying slots and we can only skip a
 *   full offset if both slots are unused, so the value we return here is always
 *   rounded down to the closest multiple of two.
 *
 * - gl_Layer and gl_ViewportIndex don't have their own varying slots, they are
 *   part of the vue header, so if these are read we can't skip anything.
 */
static inline int
brw_compute_first_urb_slot_required(uint64_t inputs_read,
                                    const struct brw_vue_map *prev_stage_vue_map)
{
   if ((inputs_read & (VARYING_BIT_LAYER | VARYING_BIT_VIEWPORT)) == 0) {
      for (int i = 0; i < prev_stage_vue_map->num_slots; i++) {
         int varying = prev_stage_vue_map->slot_to_varying[i];
         if (varying > 0 && (inputs_read & BITFIELD64_BIT(varying)) != 0)
            return ROUND_DOWN_TO(i, 2);
      }
   }

   return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* BRW_COMPILER_H */
