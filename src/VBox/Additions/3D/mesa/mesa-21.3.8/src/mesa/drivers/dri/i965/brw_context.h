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


#ifndef BRWCONTEXT_INC
#define BRWCONTEXT_INC

#include <stdbool.h>
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/errors.h"
#include "brw_structs.h"
#include "brw_pipe_control.h"
#include "compiler/brw_compiler.h"

#include "isl/isl.h"
#include "blorp/blorp.h"

#include <brw_bufmgr.h>

#include "dev/intel_debug.h"
#include "common/intel_decoder.h"
#include "brw_screen.h"
#include "brw_tex_obj.h"
#include "perf/intel_perf.h"
#include "perf/intel_perf_query.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Glossary:
 *
 * URB - uniform resource buffer.  A mid-sized buffer which is
 * partitioned between the fixed function units and used for passing
 * values (vertices, primitives, constants) between them.
 *
 * CURBE - constant URB entry.  An urb region (entry) used to hold
 * constant values which the fixed function units can be instructed to
 * preload into the GRF when spawning a thread.
 *
 * VUE - vertex URB entry.  An urb entry holding a vertex and usually
 * a vertex header.  The header contains control information and
 * things like primitive type, Begin/end flags and clip codes.
 *
 * PUE - primitive URB entry.  An urb entry produced by the setup (SF)
 * unit holding rasterization and interpolation parameters.
 *
 * GRF - general register file.  One of several register files
 * addressable by programmed threads.  The inputs (r0, payload, curbe,
 * urb) of the thread are preloaded to this area before the thread is
 * spawned.  The registers are individually 8 dwords wide and suitable
 * for general usage.  Registers holding thread input values are not
 * special and may be overwritten.
 *
 * MRF - message register file.  Threads communicate (and terminate)
 * by sending messages.  Message parameters are placed in contiguous
 * MRF registers.  All program output is via these messages.  URB
 * entries are populated by sending a message to the shared URB
 * function containing the new data, together with a control word,
 * often an unmodified copy of R0.
 *
 * R0 - GRF register 0.  Typically holds control information used when
 * sending messages to other threads.
 *
 * EU or GFX4 EU: The name of the programmable subsystem of the
 * i965 hardware.  Threads are executed by the EU, the registers
 * described above are part of the EU architecture.
 *
 * Fixed function units:
 *
 * CS - Command streamer.  Notional first unit, little software
 * interaction.  Holds the URB entries used for constant data, ie the
 * CURBEs.
 *
 * VF/VS - Vertex Fetch / Vertex Shader.  The fixed function part of
 * this unit is responsible for pulling vertices out of vertex buffers
 * in vram and injecting them into the processing pipe as VUEs.  If
 * enabled, it first passes them to a VS thread which is a good place
 * for the driver to implement any active vertex shader.
 *
 * HS - Hull Shader (Tessellation Control Shader)
 *
 * TE - Tessellation Engine (Tessellation Primitive Generation)
 *
 * DS - Domain Shader (Tessellation Evaluation Shader)
 *
 * GS - Geometry Shader.  This corresponds to a new DX10 concept.  If
 * enabled, incoming strips etc are passed to GS threads in individual
 * line/triangle/point units.  The GS thread may perform arbitary
 * computation and emit whatever primtives with whatever vertices it
 * chooses.  This makes GS an excellent place to implement GL's
 * unfilled polygon modes, though of course it is capable of much
 * more.  Additionally, GS is used to translate away primitives not
 * handled by latter units, including Quads and Lineloops.
 *
 * CS - Clipper.  Mesa's clipping algorithms are imported to run on
 * this unit.  The fixed function part performs cliptesting against
 * the 6 fixed clipplanes and makes descisions on whether or not the
 * incoming primitive needs to be passed to a thread for clipping.
 * User clip planes are handled via cooperation with the VS thread.
 *
 * SF - Strips Fans or Setup: Triangles are prepared for
 * rasterization.  Interpolation coefficients are calculated.
 * Flatshading and two-side lighting usually performed here.
 *
 * WM - Windower.  Interpolation of vertex attributes performed here.
 * Fragment shader implemented here.  SIMD aspects of EU taken full
 * advantage of, as pixels are processed in blocks of 16.
 *
 * CC - Color Calculator.  No EU threads associated with this unit.
 * Handles blending and (presumably) depth and stencil testing.
 */

struct brw_context;
struct brw_inst;
struct brw_vs_prog_key;
struct brw_vue_prog_key;
struct brw_wm_prog_key;
struct brw_wm_prog_data;
struct brw_cs_prog_key;
struct brw_cs_prog_data;
struct brw_label;

enum brw_pipeline {
   BRW_RENDER_PIPELINE,
   BRW_COMPUTE_PIPELINE,

   BRW_NUM_PIPELINES
};

enum brw_cache_id {
   BRW_CACHE_FS_PROG,
   BRW_CACHE_BLORP_PROG,
   BRW_CACHE_SF_PROG,
   BRW_CACHE_VS_PROG,
   BRW_CACHE_FF_GS_PROG,
   BRW_CACHE_GS_PROG,
   BRW_CACHE_TCS_PROG,
   BRW_CACHE_TES_PROG,
   BRW_CACHE_CLIP_PROG,
   BRW_CACHE_CS_PROG,

   BRW_MAX_CACHE
};

enum gfx9_astc5x5_wa_tex_type {
   GFX9_ASTC5X5_WA_TEX_TYPE_ASTC5x5 = 1 << 0,
   GFX9_ASTC5X5_WA_TEX_TYPE_AUX     = 1 << 1,
};

enum brw_state_id {
   /* brw_cache_ids must come first - see brw_program_cache.c */
   BRW_STATE_URB_FENCE = BRW_MAX_CACHE,
   BRW_STATE_FRAGMENT_PROGRAM,
   BRW_STATE_GEOMETRY_PROGRAM,
   BRW_STATE_TESS_PROGRAMS,
   BRW_STATE_VERTEX_PROGRAM,
   BRW_STATE_REDUCED_PRIMITIVE,
   BRW_STATE_PATCH_PRIMITIVE,
   BRW_STATE_PRIMITIVE,
   BRW_STATE_CONTEXT,
   BRW_STATE_PSP,
   BRW_STATE_SURFACES,
   BRW_STATE_BINDING_TABLE_POINTERS,
   BRW_STATE_INDICES,
   BRW_STATE_VERTICES,
   BRW_STATE_DEFAULT_TESS_LEVELS,
   BRW_STATE_BATCH,
   BRW_STATE_INDEX_BUFFER,
   BRW_STATE_VS_CONSTBUF,
   BRW_STATE_TCS_CONSTBUF,
   BRW_STATE_TES_CONSTBUF,
   BRW_STATE_GS_CONSTBUF,
   BRW_STATE_PROGRAM_CACHE,
   BRW_STATE_STATE_BASE_ADDRESS,
   BRW_STATE_VUE_MAP_GEOM_OUT,
   BRW_STATE_TRANSFORM_FEEDBACK,
   BRW_STATE_RASTERIZER_DISCARD,
   BRW_STATE_STATS_WM,
   BRW_STATE_UNIFORM_BUFFER,
   BRW_STATE_IMAGE_UNITS,
   BRW_STATE_META_IN_PROGRESS,
   BRW_STATE_PUSH_CONSTANT_ALLOCATION,
   BRW_STATE_NUM_SAMPLES,
   BRW_STATE_TEXTURE_BUFFER,
   BRW_STATE_GFX4_UNIT_STATE,
   BRW_STATE_CC_VP,
   BRW_STATE_SF_VP,
   BRW_STATE_CLIP_VP,
   BRW_STATE_SAMPLER_STATE_TABLE,
   BRW_STATE_VS_ATTRIB_WORKAROUNDS,
   BRW_STATE_COMPUTE_PROGRAM,
   BRW_STATE_CS_WORK_GROUPS,
   BRW_STATE_URB_SIZE,
   BRW_STATE_CC_STATE,
   BRW_STATE_BLORP,
   BRW_STATE_VIEWPORT_COUNT,
   BRW_STATE_CONSERVATIVE_RASTERIZATION,
   BRW_STATE_DRAW_CALL,
   BRW_STATE_AUX,
   BRW_NUM_STATE_BITS
};

/**
 * BRW_NEW_*_PROG_DATA and BRW_NEW_*_PROGRAM are similar, but distinct.
 *
 * BRW_NEW_*_PROGRAM relates to the gl_shader_program/gl_program structures.
 * When the currently bound shader program differs from the previous draw
 * call, these will be flagged.  They cover brw->{stage}_program and
 * ctx->{Stage}Program->_Current.
 *
 * BRW_NEW_*_PROG_DATA is flagged when the effective shaders change, from a
 * driver perspective.  Even if the same shader is bound at the API level,
 * we may need to switch between multiple versions of that shader to handle
 * changes in non-orthagonal state.
 *
 * Additionally, multiple shader programs may have identical vertex shaders
 * (for example), or compile down to the same code in the backend.  We combine
 * those into a single program cache entry.
 *
 * BRW_NEW_*_PROG_DATA occurs when switching program cache entries, which
 * covers the brw_*_prog_data structures, and brw->*.prog_offset.
 */
#define BRW_NEW_FS_PROG_DATA            (1ull << BRW_CACHE_FS_PROG)
/* XXX: The BRW_NEW_BLORP_BLIT_PROG_DATA dirty bit is unused (as BLORP doesn't
 * use the normal state upload paths), but the cache is still used.  To avoid
 * polluting the brw_program_cache code with special cases, we retain the
 * dirty bit for now.  It should eventually be removed.
 */
#define BRW_NEW_BLORP_BLIT_PROG_DATA    (1ull << BRW_CACHE_BLORP_PROG)
#define BRW_NEW_SF_PROG_DATA            (1ull << BRW_CACHE_SF_PROG)
#define BRW_NEW_VS_PROG_DATA            (1ull << BRW_CACHE_VS_PROG)
#define BRW_NEW_FF_GS_PROG_DATA         (1ull << BRW_CACHE_FF_GS_PROG)
#define BRW_NEW_GS_PROG_DATA            (1ull << BRW_CACHE_GS_PROG)
#define BRW_NEW_TCS_PROG_DATA           (1ull << BRW_CACHE_TCS_PROG)
#define BRW_NEW_TES_PROG_DATA           (1ull << BRW_CACHE_TES_PROG)
#define BRW_NEW_CLIP_PROG_DATA          (1ull << BRW_CACHE_CLIP_PROG)
#define BRW_NEW_CS_PROG_DATA            (1ull << BRW_CACHE_CS_PROG)
#define BRW_NEW_URB_FENCE               (1ull << BRW_STATE_URB_FENCE)
#define BRW_NEW_FRAGMENT_PROGRAM        (1ull << BRW_STATE_FRAGMENT_PROGRAM)
#define BRW_NEW_GEOMETRY_PROGRAM        (1ull << BRW_STATE_GEOMETRY_PROGRAM)
#define BRW_NEW_TESS_PROGRAMS           (1ull << BRW_STATE_TESS_PROGRAMS)
#define BRW_NEW_VERTEX_PROGRAM          (1ull << BRW_STATE_VERTEX_PROGRAM)
#define BRW_NEW_REDUCED_PRIMITIVE       (1ull << BRW_STATE_REDUCED_PRIMITIVE)
#define BRW_NEW_PATCH_PRIMITIVE         (1ull << BRW_STATE_PATCH_PRIMITIVE)
#define BRW_NEW_PRIMITIVE               (1ull << BRW_STATE_PRIMITIVE)
#define BRW_NEW_CONTEXT                 (1ull << BRW_STATE_CONTEXT)
#define BRW_NEW_PSP                     (1ull << BRW_STATE_PSP)
#define BRW_NEW_SURFACES                (1ull << BRW_STATE_SURFACES)
#define BRW_NEW_BINDING_TABLE_POINTERS  (1ull << BRW_STATE_BINDING_TABLE_POINTERS)
#define BRW_NEW_INDICES                 (1ull << BRW_STATE_INDICES)
#define BRW_NEW_VERTICES                (1ull << BRW_STATE_VERTICES)
#define BRW_NEW_DEFAULT_TESS_LEVELS     (1ull << BRW_STATE_DEFAULT_TESS_LEVELS)
/**
 * Used for any batch entry with a relocated pointer that will be used
 * by any 3D rendering.
 */
#define BRW_NEW_BATCH                   (1ull << BRW_STATE_BATCH)
/** \see brw.state.depth_region */
#define BRW_NEW_INDEX_BUFFER            (1ull << BRW_STATE_INDEX_BUFFER)
#define BRW_NEW_VS_CONSTBUF             (1ull << BRW_STATE_VS_CONSTBUF)
#define BRW_NEW_TCS_CONSTBUF            (1ull << BRW_STATE_TCS_CONSTBUF)
#define BRW_NEW_TES_CONSTBUF            (1ull << BRW_STATE_TES_CONSTBUF)
#define BRW_NEW_GS_CONSTBUF             (1ull << BRW_STATE_GS_CONSTBUF)
#define BRW_NEW_PROGRAM_CACHE           (1ull << BRW_STATE_PROGRAM_CACHE)
#define BRW_NEW_STATE_BASE_ADDRESS      (1ull << BRW_STATE_STATE_BASE_ADDRESS)
#define BRW_NEW_VUE_MAP_GEOM_OUT        (1ull << BRW_STATE_VUE_MAP_GEOM_OUT)
#define BRW_NEW_VIEWPORT_COUNT          (1ull << BRW_STATE_VIEWPORT_COUNT)
#define BRW_NEW_TRANSFORM_FEEDBACK      (1ull << BRW_STATE_TRANSFORM_FEEDBACK)
#define BRW_NEW_RASTERIZER_DISCARD      (1ull << BRW_STATE_RASTERIZER_DISCARD)
#define BRW_NEW_STATS_WM                (1ull << BRW_STATE_STATS_WM)
#define BRW_NEW_UNIFORM_BUFFER          (1ull << BRW_STATE_UNIFORM_BUFFER)
#define BRW_NEW_IMAGE_UNITS             (1ull << BRW_STATE_IMAGE_UNITS)
#define BRW_NEW_META_IN_PROGRESS        (1ull << BRW_STATE_META_IN_PROGRESS)
#define BRW_NEW_PUSH_CONSTANT_ALLOCATION (1ull << BRW_STATE_PUSH_CONSTANT_ALLOCATION)
#define BRW_NEW_NUM_SAMPLES             (1ull << BRW_STATE_NUM_SAMPLES)
#define BRW_NEW_TEXTURE_BUFFER          (1ull << BRW_STATE_TEXTURE_BUFFER)
#define BRW_NEW_GFX4_UNIT_STATE         (1ull << BRW_STATE_GFX4_UNIT_STATE)
#define BRW_NEW_CC_VP                   (1ull << BRW_STATE_CC_VP)
#define BRW_NEW_SF_VP                   (1ull << BRW_STATE_SF_VP)
#define BRW_NEW_CLIP_VP                 (1ull << BRW_STATE_CLIP_VP)
#define BRW_NEW_SAMPLER_STATE_TABLE     (1ull << BRW_STATE_SAMPLER_STATE_TABLE)
#define BRW_NEW_VS_ATTRIB_WORKAROUNDS   (1ull << BRW_STATE_VS_ATTRIB_WORKAROUNDS)
#define BRW_NEW_COMPUTE_PROGRAM         (1ull << BRW_STATE_COMPUTE_PROGRAM)
#define BRW_NEW_CS_WORK_GROUPS          (1ull << BRW_STATE_CS_WORK_GROUPS)
#define BRW_NEW_URB_SIZE                (1ull << BRW_STATE_URB_SIZE)
#define BRW_NEW_CC_STATE                (1ull << BRW_STATE_CC_STATE)
#define BRW_NEW_BLORP                   (1ull << BRW_STATE_BLORP)
#define BRW_NEW_CONSERVATIVE_RASTERIZATION (1ull << BRW_STATE_CONSERVATIVE_RASTERIZATION)
#define BRW_NEW_DRAW_CALL               (1ull << BRW_STATE_DRAW_CALL)
#define BRW_NEW_AUX_STATE               (1ull << BRW_STATE_AUX)

struct brw_state_flags {
   /** State update flags signalled by mesa internals */
   GLuint mesa;
   /**
    * State update flags signalled as the result of brw_tracked_state updates
    */
   uint64_t brw;
};


/** Subclass of Mesa program */
struct brw_program {
   struct gl_program program;
   GLuint id;

   bool compiled_once;
};

/** Number of texture sampler units */
#define BRW_MAX_TEX_UNIT 32

/** Max number of UBOs in a shader */
#define BRW_MAX_UBO 14

/** Max number of SSBOs in a shader */
#define BRW_MAX_SSBO 12

/** Max number of atomic counter buffer objects in a shader */
#define BRW_MAX_ABO 16

/** Max number of image uniforms in a shader */
#define BRW_MAX_IMAGES 32

/** Maximum number of actual buffers used for stream output */
#define BRW_MAX_SOL_BUFFERS 4

#define BRW_MAX_SURFACES   (BRW_MAX_DRAW_BUFFERS +                      \
                            BRW_MAX_TEX_UNIT * 2 + /* normal, gather */ \
                            BRW_MAX_UBO +                               \
                            BRW_MAX_SSBO +                              \
                            BRW_MAX_ABO +                               \
                            BRW_MAX_IMAGES +                            \
                            2 + /* shader time, pull constants */       \
                            1 /* cs num work groups */)

struct brw_cache {
   struct brw_context *brw;

   struct brw_cache_item **items;
   struct brw_bo *bo;
   void *map;
   GLuint size, n_items;

   uint32_t next_offset;
};

#define perf_debug(...) do {                                    \
   static GLuint msg_id = 0;                                    \
   if (INTEL_DEBUG(DEBUG_PERF))                                 \
      dbg_printf(__VA_ARGS__);                                  \
   if (brw->perf_debug)                                         \
      _mesa_gl_debugf(&brw->ctx, &msg_id,                       \
                      MESA_DEBUG_SOURCE_API,                    \
                      MESA_DEBUG_TYPE_PERFORMANCE,              \
                      MESA_DEBUG_SEVERITY_MEDIUM,               \
                      __VA_ARGS__);                             \
} while(0)

#define WARN_ONCE(cond, fmt...) do {                            \
   if (unlikely(cond)) {                                        \
      static bool _warned = false;                              \
      static GLuint msg_id = 0;                                 \
      if (!_warned) {                                           \
         fprintf(stderr, "WARNING: ");                          \
         fprintf(stderr, fmt);                                  \
         _warned = true;                                        \
                                                                \
         _mesa_gl_debugf(ctx, &msg_id,                          \
                         MESA_DEBUG_SOURCE_API,                 \
                         MESA_DEBUG_TYPE_OTHER,                 \
                         MESA_DEBUG_SEVERITY_HIGH, fmt);        \
      }                                                         \
   }                                                            \
} while (0)

/* Considered adding a member to this struct to document which flags
 * an update might raise so that ordering of the state atoms can be
 * checked or derived at runtime.  Dropped the idea in favor of having
 * a debug mode where the state is monitored for flags which are
 * raised that have already been tested against.
 */
struct brw_tracked_state {
   struct brw_state_flags dirty;
   void (*emit)( struct brw_context *brw );
};

enum shader_time_shader_type {
   ST_NONE,
   ST_VS,
   ST_TCS,
   ST_TES,
   ST_GS,
   ST_FS8,
   ST_FS16,
   ST_FS32,
   ST_CS,
};

struct brw_vertex_buffer {
   /** Buffer object containing the uploaded vertex data */
   struct brw_bo *bo;
   uint32_t offset;
   uint32_t size;
   /** Byte stride between elements in the uploaded array */
   GLuint stride;
   GLuint step_rate;
};
struct brw_vertex_element {
   const struct gl_vertex_format *glformat;

   int buffer;
   bool is_dual_slot;
   /** Offset of the first element within the buffer object */
   unsigned int offset;
};

struct brw_query_object {
   struct gl_query_object Base;

   /** Last query BO associated with this query. */
   struct brw_bo *bo;

   /** Last index in bo with query data for this object. */
   int last_index;

   /** True if we know the batch has been flushed since we ended the query. */
   bool flushed;
};

struct brw_reloc_list {
   struct drm_i915_gem_relocation_entry *relocs;
   int reloc_count;
   int reloc_array_size;
};

struct brw_growing_bo {
   struct brw_bo *bo;
   uint32_t *map;
   struct brw_bo *partial_bo;
   uint32_t *partial_bo_map;
   unsigned partial_bytes;
   enum brw_memory_zone memzone;
};

struct brw_batch {
   /** Current batchbuffer being queued up. */
   struct brw_growing_bo batch;
   /** Current statebuffer being queued up. */
   struct brw_growing_bo state;

   /** Last batchbuffer submitted to the hardware.  Used for glFinish(). */
   struct brw_bo *last_bo;

#ifdef DEBUG
   uint16_t emit, total;
#endif
   uint32_t *map_next;
   uint32_t state_used;

   bool use_shadow_copy;
   bool use_batch_first;
   bool needs_sol_reset;
   bool state_base_address_emitted;
   bool no_wrap;
   bool contains_fence_signal;

   struct brw_reloc_list batch_relocs;
   struct brw_reloc_list state_relocs;
   unsigned int valid_reloc_flags;

   /** The validation list */
   struct drm_i915_gem_exec_object2 *validation_list;
   struct brw_bo **exec_bos;
   int exec_count;
   int exec_array_size;

   /** The amount of aperture space (in bytes) used by all exec_bos */
   uint64_t aperture_space;

   struct {
      uint32_t *map_next;
      int batch_reloc_count;
      int state_reloc_count;
      int exec_count;
   } saved;

   /** Map from batch offset to brw_state_batch data (with DEBUG_BATCH) */
   struct hash_table_u64 *state_batch_sizes;

   struct intel_batch_decode_ctx decoder;

   /** A list of drm_i915_exec_fences to have execbuf signal or wait on */
   struct util_dynarray exec_fences;
};

#define BRW_MAX_XFB_STREAMS 4

struct brw_transform_feedback_counter {
   /**
    * Index of the first entry of this counter within the primitive count BO.
    * An entry is considered to be an N-tuple of 64bit values, where N is the
    * number of vertex streams supported by the platform.
    */
   unsigned bo_start;

   /**
    * Index one past the last entry of this counter within the primitive
    * count BO.
    */
   unsigned bo_end;

   /**
    * Primitive count values accumulated while this counter was active,
    * excluding any entries buffered between \c bo_start and \c bo_end, which
    * haven't been accounted for yet.
    */
   uint64_t accum[BRW_MAX_XFB_STREAMS];
};

static inline void
brw_reset_transform_feedback_counter(
   struct brw_transform_feedback_counter *counter)
{
   counter->bo_start = counter->bo_end;
   memset(&counter->accum, 0, sizeof(counter->accum));
}

struct brw_transform_feedback_object {
   struct gl_transform_feedback_object base;

   /** A buffer to hold SO_WRITE_OFFSET(n) values while paused. */
   struct brw_bo *offset_bo;

   /** If true, SO_WRITE_OFFSET(n) should be reset to zero at next use. */
   bool zero_offsets;

   /** The most recent primitive mode (GL_TRIANGLES/GL_POINTS/GL_LINES). */
   GLenum primitive_mode;

   /**
    * The maximum number of vertices that we can write without overflowing
    * any of the buffers currently being used for transform feedback.
    */
   unsigned max_index;

   struct brw_bo *prim_count_bo;

   /**
    * Count of primitives generated during this transform feedback operation.
    */
   struct brw_transform_feedback_counter counter;

   /**
    * Count of primitives generated during the previous transform feedback
    * operation.  Used to implement DrawTransformFeedback().
    */
   struct brw_transform_feedback_counter previous_counter;

   /**
    * Number of vertices written between last Begin/EndTransformFeedback().
    *
    * Used to implement DrawTransformFeedback().
    */
   uint64_t vertices_written[BRW_MAX_XFB_STREAMS];
   bool vertices_written_valid;
};

/**
 * Data shared between each programmable stage in the pipeline (vs, gs, and
 * wm).
 */
struct brw_stage_state
{
   gl_shader_stage stage;
   struct brw_stage_prog_data *prog_data;

   /**
    * Optional scratch buffer used to store spilled register values and
    * variably-indexed GRF arrays.
    *
    * The contents of this buffer are short-lived so the same memory can be
    * re-used at will for multiple shader programs (executed by the same fixed
    * function).  However reusing a scratch BO for which shader invocations
    * are still in flight with a per-thread scratch slot size other than the
    * original can cause threads with different scratch slot size and FFTID
    * (which may be executed in parallel depending on the shader stage and
    * hardware generation) to map to an overlapping region of the scratch
    * space, which can potentially lead to mutual scratch space corruption.
    * For that reason if you borrow this scratch buffer you should only be
    * using the slot size given by the \c per_thread_scratch member below,
    * unless you're taking additional measures to synchronize thread execution
    * across slot size changes.
    */
   struct brw_bo *scratch_bo;

   /**
    * Scratch slot size allocated for each thread in the buffer object given
    * by \c scratch_bo.
    */
   uint32_t per_thread_scratch;

   /** Offset in the program cache to the program */
   uint32_t prog_offset;

   /** Offset in the batchbuffer to Gfx4-5 pipelined state (VS/WM/GS_STATE). */
   uint32_t state_offset;

   struct brw_bo *push_const_bo; /* NULL if using the batchbuffer */
   uint32_t push_const_offset; /* Offset in the push constant BO or batch */
   int push_const_size; /* in 256-bit register increments */

   /* Binding table: pointers to SURFACE_STATE entries. */
   uint32_t bind_bo_offset;
   uint32_t surf_offset[BRW_MAX_SURFACES];

   /** SAMPLER_STATE count and table offset */
   uint32_t sampler_count;
   uint32_t sampler_offset;

   struct brw_image_param image_param[BRW_MAX_IMAGES];

   /** Need to re-emit 3DSTATE_CONSTANT_XS? */
   bool push_constants_dirty;
};

enum brw_predicate_state {
   /* The first two states are used if we can determine whether to draw
    * without having to look at the values in the query object buffer. This
    * will happen if there is no conditional render in progress, if the query
    * object is already completed or if something else has already added
    * samples to the preliminary result such as via a BLT command.
    */
   BRW_PREDICATE_STATE_RENDER,
   BRW_PREDICATE_STATE_DONT_RENDER,
   /* In this case whether to draw or not depends on the result of an
    * MI_PREDICATE command so the predicate enable bit needs to be checked.
    */
   BRW_PREDICATE_STATE_USE_BIT,
   /* In this case, either MI_PREDICATE doesn't exist or we lack the
    * necessary kernel features to use it.  Stall for the query result.
    */
   BRW_PREDICATE_STATE_STALL_FOR_QUERY,
};

struct shader_times;

struct intel_l3_config;
struct intel_perf;

struct brw_uploader {
   struct brw_bufmgr *bufmgr;
   struct brw_bo *bo;
   void *map;
   uint32_t next_offset;
   unsigned default_size;
};

/**
 * brw_context is derived from gl_context.
 */
struct brw_context
{
   struct gl_context ctx; /**< base class, must be first field */

   struct
   {
      /**
       * Emit an MI_REPORT_PERF_COUNT command packet.
       *
       * This asks the GPU to write a report of the current OA counter values
       * into @bo at the given offset and containing the given @report_id
       * which we can cross-reference when parsing the report (gfx7+ only).
       */
      void (*emit_mi_report_perf_count)(struct brw_context *brw,
                                        struct brw_bo *bo,
                                        uint32_t offset_in_bytes,
                                        uint32_t report_id);

      void (*emit_compute_walker)(struct brw_context *brw);
      void (*emit_raw_pipe_control)(struct brw_context *brw, uint32_t flags,
                                    struct brw_bo *bo, uint32_t offset,
                                    uint64_t imm);
   } vtbl;

   struct brw_bufmgr *bufmgr;

   uint32_t hw_ctx;

   /**
    * BO for post-sync nonzero writes for gfx6 workaround.
    *
    * This buffer also contains a marker + description of the driver. This
    * buffer is added to all execbufs syscalls so that we can identify the
    * driver that generated a hang by looking at the content of the buffer in
    * the error state.
    *
    * Read/write should go at workaround_bo_offset in that buffer to avoid
    * overriding the debug data.
    */
   struct brw_bo *workaround_bo;
   uint32_t workaround_bo_offset;
   uint8_t pipe_controls_since_last_cs_stall;

   /**
    * Set of struct brw_bo * that have been rendered to within this batchbuffer
    * and would need flushing before being used from another cache domain that
    * isn't coherent with it (i.e. the sampler).
    */
   struct hash_table *render_cache;

   /**
    * Set of struct brw_bo * that have been used as a depth buffer within this
    * batchbuffer and would need flushing before being used from another cache
    * domain that isn't coherent with it (i.e. the sampler).
    */
   struct set *depth_cache;

   /**
    * Number of resets observed in the system at context creation.
    *
    * This is tracked in the context so that we can determine that another
    * reset has occurred.
    */
   uint32_t reset_count;

   struct brw_batch batch;

   struct brw_uploader upload;

   /**
    * Set if rendering has occurred to the drawable's front buffer.
    *
    * This is used in the DRI2 case to detect that glFlush should also copy
    * the contents of the fake front buffer to the real front buffer.
    */
   bool front_buffer_dirty;

   /**
    * True if the __DRIdrawable's current __DRIimageBufferMask is
    * __DRI_IMAGE_BUFFER_SHARED.
    */
   bool is_shared_buffer_bound;

   /**
    * True if a shared buffer is bound and it has received any rendering since
    * the previous __DRImutableRenderBufferLoaderExtension::displaySharedBuffer().
    */
   bool is_shared_buffer_dirty;

   /** Framerate throttling: @{ */
   struct brw_bo *throttle_batch[2];

   /* Limit the number of outstanding SwapBuffers by waiting for an earlier
    * frame of rendering to complete. This gives a very precise cap to the
    * latency between input and output such that rendering never gets more
    * than a frame behind the user. (With the caveat that we technically are
    * not using the SwapBuffers itself as a barrier but the first batch
    * submitted afterwards, which may be immediately prior to the next
    * SwapBuffers.)
    */
   bool need_swap_throttle;

   /** General throttling, not caught by throttling between SwapBuffers */
   bool need_flush_throttle;
   /** @} */

   GLuint stats_wm;

   /**
    * drirc options:
    * @{
    */
   bool always_flush_batch;
   bool always_flush_cache;
   bool disable_throttling;
   bool precompile;
   bool dual_color_blend_by_location;
   /** @} */

   GLuint primitive; /**< Hardware primitive, such as _3DPRIM_TRILIST. */

   bool object_preemption; /**< Object level preemption enabled. */

   GLenum reduced_primitive;

   /**
    * Set if we're either a debug context or the INTEL_DEBUG=perf environment
    * variable is set, this is the flag indicating to do expensive work that
    * might lead to a perf_debug() call.
    */
   bool perf_debug;

   uint64_t max_gtt_map_object_size;

   bool has_hiz;
   bool has_separate_stencil;
   bool has_swizzling;

   bool can_push_ubos;

   /** Derived stencil states. */
   bool stencil_enabled;
   bool stencil_two_sided;
   bool stencil_write_enabled;
   /** Derived polygon state. */
   bool polygon_front_bit; /**< 0=GL_CCW, 1=GL_CW */

   struct isl_device isl_dev;

   struct blorp_context blorp;

   GLuint NewGLState;
   struct {
      struct brw_state_flags pipelines[BRW_NUM_PIPELINES];
   } state;

   enum brw_pipeline last_pipeline;

   struct brw_cache cache;

   /* Whether a meta-operation is in progress. */
   bool meta_in_progress;

   /* Whether the last depth/stencil packets were both NULL. */
   bool no_depth_or_stencil;

   /* The last PMA stall bits programmed. */
   uint32_t pma_stall_bits;

   /* Whether INTEL_black_render is active. */
   bool frontend_noop;

   struct {
      struct {
         /**
          * Either the value of gl_BaseVertex for indexed draw calls or the
          * value of the argument <first> for non-indexed draw calls for the
          * current _mesa_prim.
          */
         int firstvertex;

         /** The value of gl_BaseInstance for the current _mesa_prim. */
         int gl_baseinstance;
      } params;

      /**
       * Buffer and offset used for GL_ARB_shader_draw_parameters which will
       * point to the indirect buffer for indirect draw calls.
       */
      struct brw_bo *draw_params_bo;
      uint32_t draw_params_offset;

      struct {
         /**
          * The value of gl_DrawID for the current _mesa_prim. This always comes
          * in from it's own vertex buffer since it's not part of the indirect
          * draw parameters.
          */
         int gl_drawid;

         /**
          * Stores if the current _mesa_prim is an indexed or non-indexed draw
          * (~0/0). Useful to calculate gl_BaseVertex as an AND of firstvertex
          * and is_indexed_draw.
          */
         int is_indexed_draw;
      } derived_params;

      /**
       * Buffer and offset used for GL_ARB_shader_draw_parameters which contains
       * parameters that are not present in the indirect buffer. They will go in
       * their own vertex element.
       */
      struct brw_bo *derived_draw_params_bo;
      uint32_t derived_draw_params_offset;

      /**
       * Pointer to the the buffer storing the indirect draw parameters. It
       * currently only stores the number of requested draw calls but more
       * parameters could potentially be added.
       */
      struct brw_bo *draw_params_count_bo;
      uint32_t draw_params_count_offset;

      /**
       * Draw indirect buffer.
       */
      unsigned draw_indirect_stride;
      GLsizeiptr draw_indirect_offset;
      struct gl_buffer_object *draw_indirect_data;
   } draw;

   struct {
      /**
       * For gl_NumWorkGroups: If num_work_groups_bo is non NULL, then it is
       * an indirect call, and num_work_groups_offset is valid. Otherwise,
       * num_work_groups is set based on glDispatchCompute.
       */
      struct brw_bo *num_work_groups_bo;
      GLintptr num_work_groups_offset;
      const GLuint *num_work_groups;
      /**
       * This is only used alongside ARB_compute_variable_group_size when the
       * local work group size is variable, otherwise it's NULL.
       */
      const GLuint *group_size;
   } compute;

   struct {
      struct brw_vertex_element inputs[VERT_ATTRIB_MAX];
      struct brw_vertex_buffer buffers[VERT_ATTRIB_MAX];

      struct brw_vertex_element *enabled[VERT_ATTRIB_MAX];
      GLuint nr_enabled;
      GLuint nr_buffers;

      /* Summary of size and varying of active arrays, so we can check
       * for changes to this state:
       */
      bool index_bounds_valid;
      unsigned int min_index, max_index;

      /* Offset from start of vertex buffer so we can avoid redefining
       * the same VB packed over and over again.
       */
      unsigned int start_vertex_bias;

      /**
       * Certain vertex attribute formats aren't natively handled by the
       * hardware and require special VS code to fix up their values.
       *
       * These bitfields indicate which workarounds are needed.
       */
      uint8_t attrib_wa_flags[VERT_ATTRIB_MAX];

      /* High bits of the last seen vertex buffer address (for workarounds). */
      uint16_t last_bo_high_bits[33];
   } vb;

   struct {
      /**
       * Index buffer for this draw_prims call.
       *
       * Updates are signaled by BRW_NEW_INDICES.
       */
      const struct _mesa_index_buffer *ib;

      /* Updates are signaled by BRW_NEW_INDEX_BUFFER. */
      struct brw_bo *bo;
      uint32_t size;
      unsigned index_size;

      /* Offset to index buffer index to use in CMD_3D_PRIM so that we can
       * avoid re-uploading the IB packet over and over if we're actually
       * referencing the same index buffer.
       */
      unsigned int start_vertex_offset;

      /* High bits of the last seen index buffer address (for workarounds). */
      uint16_t last_bo_high_bits;

      /* Used to understand is GPU state of primitive restart is up to date */
      bool enable_cut_index;
   } ib;

   /* Active vertex program:
    */
   struct gl_program *programs[MESA_SHADER_STAGES];

   /**
    * Number of samples in ctx->DrawBuffer, updated by BRW_NEW_NUM_SAMPLES so
    * that we don't have to reemit that state every time we change FBOs.
    */
   unsigned int num_samples;

   /* BRW_NEW_URB_ALLOCATIONS:
    */
   struct {
      GLuint vsize;  /* vertex size plus header in urb registers */
      GLuint gsize;  /* GS output size in urb registers */
      GLuint hsize;  /* Tessellation control output size in urb registers */
      GLuint dsize;  /* Tessellation evaluation output size in urb registers */
      GLuint csize;  /* constant buffer size in urb registers */
      GLuint sfsize; /* setup data size in urb registers */

      bool constrained;

      GLuint nr_vs_entries;
      GLuint nr_hs_entries;
      GLuint nr_ds_entries;
      GLuint nr_gs_entries;
      GLuint nr_clip_entries;
      GLuint nr_sf_entries;
      GLuint nr_cs_entries;

      GLuint vs_start;
      GLuint hs_start;
      GLuint ds_start;
      GLuint gs_start;
      GLuint clip_start;
      GLuint sf_start;
      GLuint cs_start;
      /**
       * URB size in the current configuration.  The units this is expressed
       * in are somewhat inconsistent, see intel_device_info::urb::size.
       *
       * FINISHME: Represent the URB size consistently in KB on all platforms.
       */
      GLuint size;

      /* True if the most recently sent _3DSTATE_URB message allocated
       * URB space for the GS.
       */
      bool gs_present;

      /* True if the most recently sent _3DSTATE_URB message allocated
       * URB space for the HS and DS.
       */
      bool tess_present;
   } urb;


   /* BRW_NEW_PUSH_CONSTANT_ALLOCATION */
   struct {
      GLuint wm_start;  /**< pos of first wm const in CURBE buffer */
      GLuint wm_size;   /**< number of float[4] consts, multiple of 16 */
      GLuint clip_start;
      GLuint clip_size;
      GLuint vs_start;
      GLuint vs_size;
      GLuint total_size;

      /**
       * Pointer to the (intel_upload.c-generated) BO containing the uniforms
       * for upload to the CURBE.
       */
      struct brw_bo *curbe_bo;
      /** Offset within curbe_bo of space for current curbe entry */
      GLuint curbe_offset;
   } curbe;

   /**
    * Layout of vertex data exiting the geometry portion of the pipleine.
    * This comes from the last enabled shader stage (GS, DS, or VS).
    *
    * BRW_NEW_VUE_MAP_GEOM_OUT is flagged when the VUE map changes.
    */
   struct brw_vue_map vue_map_geom_out;

   struct {
      struct brw_stage_state base;
   } vs;

   struct {
      struct brw_stage_state base;
   } tcs;

   struct {
      struct brw_stage_state base;
   } tes;

   struct {
      struct brw_stage_state base;

      /**
       * True if the 3DSTATE_GS command most recently emitted to the 3D
       * pipeline enabled the GS; false otherwise.
       */
      bool enabled;
   } gs;

   struct {
      struct brw_ff_gs_prog_data *prog_data;

      bool prog_active;
      /** Offset in the program cache to the CLIP program pre-gfx6 */
      uint32_t prog_offset;
      uint32_t state_offset;

      uint32_t bind_bo_offset;
      /**
       * Surface offsets for the binding table. We only need surfaces to
       * implement transform feedback so BRW_MAX_SOL_BINDINGS is all that we
       * need in this case.
       */
      uint32_t surf_offset[BRW_MAX_SOL_BINDINGS];
   } ff_gs;

   struct {
      struct brw_clip_prog_data *prog_data;

      /** Offset in the program cache to the CLIP program pre-gfx6 */
      uint32_t prog_offset;

      /* Offset in the batch to the CLIP state on pre-gfx6. */
      uint32_t state_offset;

      /* As of gfx6, this is the offset in the batch to the CLIP VP,
       * instead of vp_bo.
       */
      uint32_t vp_offset;

      /**
       * The number of viewports to use.  If gl_ViewportIndex is written,
       * we can have up to ctx->Const.MaxViewports viewports.  If not,
       * the viewport index is always 0, so we can only emit one.
       */
      uint8_t viewport_count;
   } clip;


   struct {
      struct brw_sf_prog_data *prog_data;

      /** Offset in the program cache to the CLIP program pre-gfx6 */
      uint32_t prog_offset;
      uint32_t state_offset;
      uint32_t vp_offset;
   } sf;

   struct {
      struct brw_stage_state base;

      /**
       * Buffer object used in place of multisampled null render targets on
       * Gfx6.  See brw_emit_null_surface_state().
       */
      struct brw_bo *multisampled_null_render_target_bo;

      float offset_clamp;
   } wm;

   struct {
      struct brw_stage_state base;
   } cs;

   struct {
      uint32_t state_offset;
      uint32_t blend_state_offset;
      uint32_t depth_stencil_state_offset;
      uint32_t vp_offset;
   } cc;

   struct {
      struct brw_query_object *obj;
      bool begin_emitted;
   } query;

   struct {
      enum brw_predicate_state state;
      bool supported;
   } predicate;

   struct intel_perf_context *perf_ctx;

   int num_atoms[BRW_NUM_PIPELINES];
   const struct brw_tracked_state render_atoms[76];
   const struct brw_tracked_state compute_atoms[11];

   const enum isl_format *mesa_to_isl_render_format;
   const bool *mesa_format_supports_render;

   /* PrimitiveRestart */
   struct {
      bool in_progress;
      bool enable_cut_index;
      unsigned restart_index;
   } prim_restart;

   /** Computed depth/stencil/hiz state from the current attached
    * renderbuffers, valid only during the drawing state upload loop after
    * brw_workaround_depthstencil_alignment().
    */
   struct {
      /* Inter-tile (page-aligned) byte offsets. */
      uint32_t depth_offset;
      /* Intra-tile x,y offsets for drawing to combined depth-stencil. Only
       * used for Gen < 6.
       */
      uint32_t tile_x, tile_y;
   } depthstencil;

   uint32_t num_instances;
   int basevertex;
   int baseinstance;

   struct {
      const struct intel_l3_config *config;
   } l3;

   struct {
      struct brw_bo *bo;
      const char **names;
      int *ids;
      enum shader_time_shader_type *types;
      struct shader_times *cumulative;
      int num_entries;
      int max_entries;
      double report_time;
   } shader_time;

   struct brw_fast_clear_state *fast_clear_state;

   /* Array of aux usages to use for drawing.  Aux usage for render targets is
    * a bit more complex than simply calling a single function so we need some
    * way of passing it form brw_draw.c to surface state setup.
    */
   enum isl_aux_usage draw_aux_usage[MAX_DRAW_BUFFERS];

   enum gfx9_astc5x5_wa_tex_type gfx9_astc5x5_wa_tex_mask;

   /** Last rendering scale argument provided to brw_emit_hashing_mode(). */
   unsigned current_hash_scale;

   __DRIcontext *driContext;
   struct brw_screen *screen;
   void *mem_ctx;
};

/* brw_clear.c */
extern void brw_init_clear_functions(struct dd_function_table *functions);

/*======================================================================
 * brw_context.c
 */
extern const char *const brw_vendor_string;

extern const char *
brw_get_renderer_string(const struct brw_screen *screen);

enum {
   DRI_CONF_BO_REUSE_DISABLED,
   DRI_CONF_BO_REUSE_ALL
};

void brw_update_renderbuffers(__DRIcontext *context,
                                __DRIdrawable *drawable);
void brw_prepare_render(struct brw_context *brw);

void gfx9_apply_single_tex_astc5x5_wa(struct brw_context *brw,
                                      mesa_format format,
                                      enum isl_aux_usage aux_usage);

void brw_predraw_resolve_inputs(struct brw_context *brw, bool rendering,
                                bool *draw_aux_buffer_disabled);

void brw_resolve_for_dri2_flush(struct brw_context *brw,
                                  __DRIdrawable *drawable);

GLboolean brw_create_context(gl_api api,
                             const struct gl_config *mesaVis,
                             __DRIcontext *driContextPriv,
                             const struct __DriverContextConfig *ctx_config,
                             unsigned *error,
                             void *sharedContextPrivate);

/*======================================================================
 * brw_misc_state.c
 */
void brw_workaround_depthstencil_alignment(struct brw_context *brw,
                                           GLbitfield clear_mask);
void brw_emit_hashing_mode(struct brw_context *brw, unsigned width,
                           unsigned height, unsigned scale);

/* brw_object_purgeable.c */
void brw_init_object_purgeable_functions(struct dd_function_table *functions);

/*======================================================================
 * brw_queryobj.c
 */
void brw_init_common_queryobj_functions(struct dd_function_table *functions);
void gfx4_init_queryobj_functions(struct dd_function_table *functions);
void brw_emit_query_begin(struct brw_context *brw);
void brw_emit_query_end(struct brw_context *brw);
void brw_query_counter(struct gl_context *ctx, struct gl_query_object *q);
bool brw_is_query_pipelined(struct brw_query_object *query);
uint64_t brw_raw_timestamp_delta(struct brw_context *brw,
                                 uint64_t time0, uint64_t time1);

/** gfx6_queryobj.c */
void gfx6_init_queryobj_functions(struct dd_function_table *functions);
void brw_write_timestamp(struct brw_context *brw, struct brw_bo *bo, int idx);
void brw_write_depth_count(struct brw_context *brw, struct brw_bo *bo, int idx);

/** hsw_queryobj.c */
void hsw_overflow_result_to_gpr0(struct brw_context *brw,
                                 struct brw_query_object *query,
                                 int count);
void hsw_init_queryobj_functions(struct dd_function_table *functions);

/** brw_conditional_render.c */
void brw_init_conditional_render_functions(struct dd_function_table *functions);
bool brw_check_conditional_render(struct brw_context *brw);

/** brw_batch.c */
void brw_load_register_mem(struct brw_context *brw,
                           uint32_t reg,
                           struct brw_bo *bo,
                           uint32_t offset);
void brw_load_register_mem64(struct brw_context *brw,
                             uint32_t reg,
                             struct brw_bo *bo,
                             uint32_t offset);
void brw_store_register_mem32(struct brw_context *brw,
                              struct brw_bo *bo, uint32_t reg, uint32_t offset);
void brw_store_register_mem64(struct brw_context *brw,
                              struct brw_bo *bo, uint32_t reg, uint32_t offset);
void brw_load_register_imm32(struct brw_context *brw,
                             uint32_t reg, uint32_t imm);
void brw_load_register_imm64(struct brw_context *brw,
                             uint32_t reg, uint64_t imm);
void brw_load_register_reg(struct brw_context *brw, uint32_t dst,
                           uint32_t src);
void brw_load_register_reg64(struct brw_context *brw, uint32_t dst,
                             uint32_t src);
void brw_store_data_imm32(struct brw_context *brw, struct brw_bo *bo,
                          uint32_t offset, uint32_t imm);
void brw_store_data_imm64(struct brw_context *brw, struct brw_bo *bo,
                          uint32_t offset, uint64_t imm);

/*======================================================================
 * intel_tex_validate.c
 */
void brw_validate_textures( struct brw_context *brw );


/*======================================================================
 * brw_program.c
 */
void brw_init_frag_prog_functions(struct dd_function_table *functions);

void brw_get_scratch_bo(struct brw_context *brw,
                        struct brw_bo **scratch_bo, int size);
void brw_alloc_stage_scratch(struct brw_context *brw,
                             struct brw_stage_state *stage_state,
                             unsigned per_thread_size);
void brw_init_shader_time(struct brw_context *brw);
int brw_get_shader_time_index(struct brw_context *brw,
                              struct gl_program *prog,
                              enum shader_time_shader_type type,
                              bool is_glsl_sh);
void brw_collect_and_report_shader_time(struct brw_context *brw);
void brw_destroy_shader_time(struct brw_context *brw);

/* brw_urb.c
 */
void brw_calculate_urb_fence(struct brw_context *brw, unsigned csize,
                             unsigned vsize, unsigned sfsize);
void brw_upload_urb_fence(struct brw_context *brw);

/* brw_curbe.c
 */
void brw_upload_cs_urb_state(struct brw_context *brw);

/* brw_vs.c */
gl_clip_plane *brw_select_clip_planes(struct gl_context *ctx);

/* brw_draw_upload.c */
unsigned brw_get_vertex_surface_type(struct brw_context *brw,
                                     const struct gl_vertex_format *glformat);

static inline unsigned
brw_get_index_type(unsigned index_size)
{
   /* The hw needs 0x00, 0x01, and 0x02 for ubyte, ushort, and uint,
    * respectively.
    */
   return index_size >> 1;
}

void brw_prepare_vertices(struct brw_context *brw);

/* brw_wm_surface_state.c */
void brw_update_buffer_texture_surface(struct gl_context *ctx,
                                       unsigned unit,
                                       uint32_t *surf_offset);
void
brw_update_sol_surface(struct brw_context *brw,
                       struct gl_buffer_object *buffer_obj,
                       uint32_t *out_offset, unsigned num_vector_components,
                       unsigned stride_dwords, unsigned offset_dwords);
void brw_upload_ubo_surfaces(struct brw_context *brw, struct gl_program *prog,
                             struct brw_stage_state *stage_state,
                             struct brw_stage_prog_data *prog_data);
void brw_upload_image_surfaces(struct brw_context *brw,
                               const struct gl_program *prog,
                               struct brw_stage_state *stage_state,
                               struct brw_stage_prog_data *prog_data);

/* brw_surface_formats.c */
void brw_screen_init_surface_formats(struct brw_screen *screen);
void brw_init_surface_formats(struct brw_context *brw);
bool brw_render_target_supported(struct brw_context *brw,
                                 struct gl_renderbuffer *rb);
uint32_t brw_depth_format(struct brw_context *brw, mesa_format format);

/* brw_performance_query.c */
void brw_init_performance_queries(struct brw_context *brw);

/* intel_extensions.c */
extern void brw_init_extensions(struct gl_context *ctx);

/* intel_state.c */
extern int brw_translate_shadow_compare_func(GLenum func);
extern int brw_translate_compare_func(GLenum func);
extern int brw_translate_stencil_op(GLenum op);

/* brw_sync.c */
void brw_init_syncobj_functions(struct dd_function_table *functions);

/* gfx6_sol.c */
struct gl_transform_feedback_object *
brw_new_transform_feedback(struct gl_context *ctx, GLuint name);
void
brw_delete_transform_feedback(struct gl_context *ctx,
                              struct gl_transform_feedback_object *obj);
void
brw_begin_transform_feedback(struct gl_context *ctx, GLenum mode,
                             struct gl_transform_feedback_object *obj);
void
brw_end_transform_feedback(struct gl_context *ctx,
                           struct gl_transform_feedback_object *obj);
void
brw_pause_transform_feedback(struct gl_context *ctx,
                             struct gl_transform_feedback_object *obj);
void
brw_resume_transform_feedback(struct gl_context *ctx,
                              struct gl_transform_feedback_object *obj);
void
brw_save_primitives_written_counters(struct brw_context *brw,
                                     struct brw_transform_feedback_object *obj);
GLsizei
brw_get_transform_feedback_vertex_count(struct gl_context *ctx,
                                        struct gl_transform_feedback_object *obj,
                                        GLuint stream);

/* gfx7_sol_state.c */
void
gfx7_begin_transform_feedback(struct gl_context *ctx, GLenum mode,
                              struct gl_transform_feedback_object *obj);
void
gfx7_end_transform_feedback(struct gl_context *ctx,
                            struct gl_transform_feedback_object *obj);
void
gfx7_pause_transform_feedback(struct gl_context *ctx,
                              struct gl_transform_feedback_object *obj);
void
gfx7_resume_transform_feedback(struct gl_context *ctx,
                               struct gl_transform_feedback_object *obj);

/* hsw_sol.c */
void
hsw_begin_transform_feedback(struct gl_context *ctx, GLenum mode,
                             struct gl_transform_feedback_object *obj);
void
hsw_end_transform_feedback(struct gl_context *ctx,
                           struct gl_transform_feedback_object *obj);
void
hsw_pause_transform_feedback(struct gl_context *ctx,
                             struct gl_transform_feedback_object *obj);
void
hsw_resume_transform_feedback(struct gl_context *ctx,
                              struct gl_transform_feedback_object *obj);

/* brw_blorp_blit.cpp */
GLbitfield
brw_blorp_framebuffer(struct brw_context *brw,
                      struct gl_framebuffer *readFb,
                      struct gl_framebuffer *drawFb,
                      GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                      GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                      GLbitfield mask, GLenum filter);

bool
brw_blorp_copytexsubimage(struct brw_context *brw,
                          struct gl_renderbuffer *src_rb,
                          struct gl_texture_image *dst_image,
                          int slice,
                          int srcX0, int srcY0,
                          int dstX0, int dstY0,
                          int width, int height);

/* brw_generate_mipmap.c */
void brw_generate_mipmap(struct gl_context *ctx, GLenum target,
                         struct gl_texture_object *tex_obj);

void
gfx6_get_sample_position(struct gl_context *ctx,
                         struct gl_framebuffer *fb,
                         GLuint index,
                         GLfloat *result);

/* gfx8_multisample_state.c */
void gfx8_emit_3dstate_sample_pattern(struct brw_context *brw);

/* gfx7_l3_state.c */
void brw_emit_l3_state(struct brw_context *brw);

/* gfx7_urb.c */
void
gfx7_emit_push_constant_state(struct brw_context *brw, unsigned vs_size,
                              unsigned hs_size, unsigned ds_size,
                              unsigned gs_size, unsigned fs_size);

void
gfx6_upload_urb(struct brw_context *brw, unsigned vs_size,
                bool gs_present, unsigned gs_size);
void
gfx7_upload_urb(struct brw_context *brw, unsigned vs_size,
                bool gs_present, bool tess_present);

/* brw_reset.c */
extern GLenum
brw_get_graphics_reset_status(struct gl_context *ctx);
void
brw_check_for_reset(struct brw_context *brw);

/* brw_compute.c */
extern void
brw_init_compute_functions(struct dd_function_table *functions);

/* brw_program_binary.c */
extern void
brw_program_binary_init(unsigned device_id);
extern void
brw_get_program_binary_driver_sha1(struct gl_context *ctx, uint8_t *sha1);
void brw_serialize_program_binary(struct gl_context *ctx,
                                  struct gl_shader_program *sh_prog,
                                  struct gl_program *prog);
extern void
brw_deserialize_program_binary(struct gl_context *ctx,
                               struct gl_shader_program *shProg,
                               struct gl_program *prog);
void
brw_program_serialize_nir(struct gl_context *ctx, struct gl_program *prog);
void
brw_program_deserialize_driver_blob(struct gl_context *ctx,
                                    struct gl_program *prog,
                                    gl_shader_stage stage);

/*======================================================================
 * Inline conversion functions.  These are better-typed than the
 * macros used previously:
 */
static inline struct brw_context *
brw_context( struct gl_context *ctx )
{
   return (struct brw_context *)ctx;
}

static inline struct brw_program *
brw_program(struct gl_program *p)
{
   return (struct brw_program *) p;
}

static inline const struct brw_program *
brw_program_const(const struct gl_program *p)
{
   return (const struct brw_program *) p;
}

static inline bool
brw_depth_writes_enabled(const struct brw_context *brw)
{
   const struct gl_context *ctx = &brw->ctx;

   /* We consider depth writes disabled if the depth function is GL_EQUAL,
    * because it would just overwrite the existing depth value with itself.
    *
    * These bonus depth writes not only use bandwidth, but they also can
    * prevent early depth processing.  For example, if the pixel shader
    * discards, the hardware must invoke the to determine whether or not
    * to do the depth write.  If writes are disabled, we may still be able
    * to do the depth test before the shader, and skip the shader execution.
    *
    * The Broadwell 3DSTATE_WM_DEPTH_STENCIL documentation also contains
    * a programming note saying to disable depth writes for EQUAL.
    */
   return ctx->Depth.Test && ctx->Depth.Mask && ctx->Depth.Func != GL_EQUAL;
}

void
brw_emit_depthbuffer(struct brw_context *brw);

uint32_t get_hw_prim_for_gl_prim(int mode);

void
gfx6_upload_push_constants(struct brw_context *brw,
                           const struct gl_program *prog,
                           const struct brw_stage_prog_data *prog_data,
                           struct brw_stage_state *stage_state);

bool
gfx9_use_linear_1d_layout(const struct brw_context *brw,
                          const struct brw_mipmap_tree *mt);

/* brw_queryformat.c */
void brw_query_internal_format(struct gl_context *ctx, GLenum target,
                               GLenum internalFormat, GLenum pname,
                               GLint *params);

#ifdef __cplusplus
}
#endif

#endif
