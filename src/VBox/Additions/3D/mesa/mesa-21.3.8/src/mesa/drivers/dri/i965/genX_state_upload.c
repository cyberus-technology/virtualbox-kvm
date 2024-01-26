/*
 * Copyright © 2017 Intel Corporation
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

#include <assert.h>

#include "main/samplerobj.h"

#include "dev/intel_device_info.h"
#include "common/intel_sample_positions.h"
#include "genxml/gen_macros.h"
#include "common/intel_guardband.h"

#include "main/bufferobj.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/macros.h"
#include "main/state.h"

#include "genX_boilerplate.h"

#include "brw_context.h"
#include "brw_cs.h"
#include "brw_draw.h"
#include "brw_multisample_state.h"
#include "brw_state.h"
#include "brw_wm.h"
#include "brw_util.h"

#include "brw_batch.h"
#include "brw_buffer_objects.h"
#include "brw_fbo.h"

#include "main/enums.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/glformats.h"
#include "main/shaderapi.h"
#include "main/stencil.h"
#include "main/transformfeedback.h"
#include "main/varray.h"
#include "main/viewport.h"
#include "util/half_float.h"

#if GFX_VER == 4
static struct brw_address
KSP(struct brw_context *brw, uint32_t offset)
{
   return ro_bo(brw->cache.bo, offset);
}
#else
static uint32_t
KSP(UNUSED struct brw_context *brw, uint32_t offset)
{
   return offset;
}
#endif

#if GFX_VER >= 7
static void
emit_lrm(struct brw_context *brw, uint32_t reg, struct brw_address addr)
{
   brw_batch_emit(brw, GENX(MI_LOAD_REGISTER_MEM), lrm) {
      lrm.RegisterAddress  = reg;
      lrm.MemoryAddress    = addr;
   }
}
#endif

#if GFX_VER == 7
static void
emit_lri(struct brw_context *brw, uint32_t reg, uint32_t imm)
{
   brw_batch_emit(brw, GENX(MI_LOAD_REGISTER_IMM), lri) {
      lri.RegisterOffset   = reg;
      lri.DataDWord        = imm;
   }
}
#endif

/**
 * Polygon stipple packet
 */
static void
genX(upload_polygon_stipple)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   /* _NEW_POLYGON */
   if (!ctx->Polygon.StippleFlag)
      return;

   brw_batch_emit(brw, GENX(3DSTATE_POLY_STIPPLE_PATTERN), poly) {
      /* Polygon stipple is provided in OpenGL order, i.e. bottom
       * row first.  If we're rendering to a window (i.e. the
       * default frame buffer object, 0), then we need to invert
       * it to match our pixel layout.  But if we're rendering
       * to a FBO (i.e. any named frame buffer object), we *don't*
       * need to invert - we already match the layout.
       */
      if (ctx->DrawBuffer->FlipY) {
         for (unsigned i = 0; i < 32; i++)
            poly.PatternRow[i] = ctx->PolygonStipple[31 - i]; /* invert */
      } else {
         for (unsigned i = 0; i < 32; i++)
            poly.PatternRow[i] = ctx->PolygonStipple[i];
      }
   }
}

static const struct brw_tracked_state genX(polygon_stipple) = {
   .dirty = {
      .mesa = _NEW_POLYGON |
              _NEW_POLYGONSTIPPLE,
      .brw = BRW_NEW_CONTEXT,
   },
   .emit = genX(upload_polygon_stipple),
};

/**
 * Polygon stipple offset packet
 */
static void
genX(upload_polygon_stipple_offset)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   /* _NEW_POLYGON */
   if (!ctx->Polygon.StippleFlag)
      return;

   brw_batch_emit(brw, GENX(3DSTATE_POLY_STIPPLE_OFFSET), poly) {
      /* _NEW_BUFFERS
       *
       * If we're drawing to a system window we have to invert the Y axis
       * in order to match the OpenGL pixel coordinate system, and our
       * offset must be matched to the window position.  If we're drawing
       * to a user-created FBO then our native pixel coordinate system
       * works just fine, and there's no window system to worry about.
       */
      if (ctx->DrawBuffer->FlipY) {
         poly.PolygonStippleYOffset =
            (32 - (_mesa_geometric_height(ctx->DrawBuffer) & 31)) & 31;
      }
   }
}

static const struct brw_tracked_state genX(polygon_stipple_offset) = {
   .dirty = {
      .mesa = _NEW_BUFFERS |
              _NEW_POLYGON,
      .brw = BRW_NEW_CONTEXT,
   },
   .emit = genX(upload_polygon_stipple_offset),
};

/**
 * Line stipple packet
 */
static void
genX(upload_line_stipple)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   if (!ctx->Line.StippleFlag)
      return;

   brw_batch_emit(brw, GENX(3DSTATE_LINE_STIPPLE), line) {
      line.LineStipplePattern = ctx->Line.StipplePattern;

      line.LineStippleInverseRepeatCount = 1.0f / ctx->Line.StippleFactor;
      line.LineStippleRepeatCount = ctx->Line.StippleFactor;
   }
}

static const struct brw_tracked_state genX(line_stipple) = {
   .dirty = {
      .mesa = _NEW_LINE,
      .brw = BRW_NEW_CONTEXT,
   },
   .emit = genX(upload_line_stipple),
};

/* Constant single cliprect for framebuffer object or DRI2 drawing */
static void
genX(upload_drawing_rect)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   const struct gl_framebuffer *fb = ctx->DrawBuffer;
   const unsigned int fb_width = _mesa_geometric_width(fb);
   const unsigned int fb_height = _mesa_geometric_height(fb);

   brw_batch_emit(brw, GENX(3DSTATE_DRAWING_RECTANGLE), rect) {
      rect.ClippedDrawingRectangleXMax = fb_width - 1;
      rect.ClippedDrawingRectangleYMax = fb_height - 1;
   }
}

static const struct brw_tracked_state genX(drawing_rect) = {
   .dirty = {
      .mesa = _NEW_BUFFERS,
      .brw = BRW_NEW_BLORP |
             BRW_NEW_CONTEXT,
   },
   .emit = genX(upload_drawing_rect),
};

static uint32_t *
genX(emit_vertex_buffer_state)(struct brw_context *brw,
                               uint32_t *dw,
                               unsigned buffer_nr,
                               struct brw_bo *bo,
                               unsigned start_offset,
                               UNUSED unsigned end_offset,
                               unsigned stride,
                               UNUSED unsigned step_rate)
{
   struct GENX(VERTEX_BUFFER_STATE) buf_state = {
      .VertexBufferIndex = buffer_nr,
      .BufferPitch = stride,

      /* The VF cache designers apparently cut corners, and made the cache
       * only consider the bottom 32 bits of memory addresses.  If you happen
       * to have two vertex buffers which get placed exactly 4 GiB apart and
       * use them in back-to-back draw calls, you can get collisions.  To work
       * around this problem, we restrict vertex buffers to the low 32 bits of
       * the address space.
       */
      .BufferStartingAddress = ro_32_bo(bo, start_offset),
#if GFX_VER >= 8
      .BufferSize = end_offset - start_offset,
#endif

#if GFX_VER >= 7
      .AddressModifyEnable = true,
#endif

#if GFX_VER < 8
      .BufferAccessType = step_rate ? INSTANCEDATA : VERTEXDATA,
      .InstanceDataStepRate = step_rate,
#if GFX_VER >= 5
      .EndAddress = ro_bo(bo, end_offset - 1),
#endif
#endif

#if GFX_VER == 11
      .MOCS = ICL_MOCS_WB,
#elif GFX_VER == 10
      .MOCS = CNL_MOCS_WB,
#elif GFX_VER == 9
      .MOCS = SKL_MOCS_WB,
#elif GFX_VER == 8
      .MOCS = BDW_MOCS_WB,
#elif GFX_VER == 7
      .MOCS = GFX7_MOCS_L3,
#endif
   };

   GENX(VERTEX_BUFFER_STATE_pack)(brw, dw, &buf_state);
   return dw + GENX(VERTEX_BUFFER_STATE_length);
}

UNUSED static bool
is_passthru_format(uint32_t format)
{
   switch (format) {
   case ISL_FORMAT_R64_PASSTHRU:
   case ISL_FORMAT_R64G64_PASSTHRU:
   case ISL_FORMAT_R64G64B64_PASSTHRU:
   case ISL_FORMAT_R64G64B64A64_PASSTHRU:
      return true;
   default:
      return false;
   }
}

UNUSED static int
uploads_needed(uint32_t format,
               bool is_dual_slot)
{
   if (!is_passthru_format(format))
      return 1;

   if (is_dual_slot)
      return 2;

   switch (format) {
   case ISL_FORMAT_R64_PASSTHRU:
   case ISL_FORMAT_R64G64_PASSTHRU:
      return 1;
   case ISL_FORMAT_R64G64B64_PASSTHRU:
   case ISL_FORMAT_R64G64B64A64_PASSTHRU:
      return 2;
   default:
      unreachable("not reached");
   }
}

/*
 * Returns the format that we are finally going to use when upload a vertex
 * element. It will only change if we are using *64*PASSTHRU formats, as for
 * gen < 8 they need to be splitted on two *32*FLOAT formats.
 *
 * @upload points in which upload we are. Valid values are [0,1]
 */
static uint32_t
downsize_format_if_needed(uint32_t format,
                          int upload)
{
   assert(upload == 0 || upload == 1);

   if (!is_passthru_format(format))
      return format;

   /* ISL_FORMAT_R64_PASSTHRU and ISL_FORMAT_R64G64_PASSTHRU with an upload ==
    * 1 means that we have been forced to do 2 uploads for a size <= 2. This
    * happens with gen < 8 and dvec3 or dvec4 vertex shader input
    * variables. In those cases, we return ISL_FORMAT_R32_FLOAT as a way of
    * flagging that we want to fill with zeroes this second forced upload.
    */
   switch (format) {
   case ISL_FORMAT_R64_PASSTHRU:
      return upload == 0 ? ISL_FORMAT_R32G32_FLOAT
                         : ISL_FORMAT_R32_FLOAT;
   case ISL_FORMAT_R64G64_PASSTHRU:
      return upload == 0 ? ISL_FORMAT_R32G32B32A32_FLOAT
                         : ISL_FORMAT_R32_FLOAT;
   case ISL_FORMAT_R64G64B64_PASSTHRU:
      return upload == 0 ? ISL_FORMAT_R32G32B32A32_FLOAT
                         : ISL_FORMAT_R32G32_FLOAT;
   case ISL_FORMAT_R64G64B64A64_PASSTHRU:
      return ISL_FORMAT_R32G32B32A32_FLOAT;
   default:
      unreachable("not reached");
   }
}

/*
 * Returns the number of componentes associated with a format that is used on
 * a 64 to 32 format split. See downsize_format()
 */
static int
upload_format_size(uint32_t upload_format)
{
   switch (upload_format) {
   case ISL_FORMAT_R32_FLOAT:

      /* downsized_format has returned this one in order to flag that we are
       * performing a second upload which we want to have filled with
       * zeroes. This happens with gen < 8, a size <= 2, and dvec3 or dvec4
       * vertex shader input variables.
       */

      return 0;
   case ISL_FORMAT_R32G32_FLOAT:
      return 2;
   case ISL_FORMAT_R32G32B32A32_FLOAT:
      return 4;
   default:
      unreachable("not reached");
   }
}

static UNUSED uint16_t
pinned_bo_high_bits(struct brw_bo *bo)
{
   return (bo->kflags & EXEC_OBJECT_PINNED) ? bo->gtt_offset >> 32ull : 0;
}

/* The VF cache designers apparently cut corners, and made the cache key's
 * <VertexBufferIndex, Memory Address> tuple only consider the bottom 32 bits
 * of the address.  If you happen to have two vertex buffers which get placed
 * exactly 4 GiB apart and use them in back-to-back draw calls, you can get
 * collisions.  (These collisions can happen within a single batch.)
 *
 * In the soft-pin world, we'd like to assign addresses up front, and never
 * move buffers.  So, we need to do a VF cache invalidate if the buffer for
 * a particular VB slot has different [48:32] address bits than the last one.
 *
 * In the relocation world, we have no idea what the addresses will be, so
 * we can't apply this workaround.  Instead, we tell the kernel to move it
 * to the low 4GB regardless.
 *
 * This HW issue is gone on Gfx11+.
 */
static void
vf_invalidate_for_vb_48bit_transitions(UNUSED struct brw_context *brw)
{
#if GFX_VER >= 8 && GFX_VER < 11
   bool need_invalidate = false;

   for (unsigned i = 0; i < brw->vb.nr_buffers; i++) {
      uint16_t high_bits = pinned_bo_high_bits(brw->vb.buffers[i].bo);

      if (high_bits != brw->vb.last_bo_high_bits[i]) {
         need_invalidate = true;
         brw->vb.last_bo_high_bits[i] = high_bits;
      }
   }

   if (brw->draw.draw_params_bo) {
      uint16_t high_bits = pinned_bo_high_bits(brw->draw.draw_params_bo);

      if (brw->vb.last_bo_high_bits[brw->vb.nr_buffers] != high_bits) {
         need_invalidate = true;
         brw->vb.last_bo_high_bits[brw->vb.nr_buffers] = high_bits;
      }
   }

   if (brw->draw.derived_draw_params_bo) {
      uint16_t high_bits = pinned_bo_high_bits(brw->draw.derived_draw_params_bo);

      if (brw->vb.last_bo_high_bits[brw->vb.nr_buffers + 1] != high_bits) {
         need_invalidate = true;
         brw->vb.last_bo_high_bits[brw->vb.nr_buffers + 1] = high_bits;
      }
   }

   if (need_invalidate) {
      brw_emit_pipe_control_flush(brw, PIPE_CONTROL_VF_CACHE_INVALIDATE | PIPE_CONTROL_CS_STALL);
   }
#endif
}

static void
vf_invalidate_for_ib_48bit_transition(UNUSED struct brw_context *brw)
{
#if GFX_VER >= 8
   uint16_t high_bits = pinned_bo_high_bits(brw->ib.bo);

   if (high_bits != brw->ib.last_bo_high_bits) {
      brw_emit_pipe_control_flush(brw, PIPE_CONTROL_VF_CACHE_INVALIDATE);
      brw->ib.last_bo_high_bits = high_bits;
   }
#endif
}

static void
genX(emit_vertices)(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   uint32_t *dw;

   brw_prepare_vertices(brw);
   brw_prepare_shader_draw_parameters(brw);

#if GFX_VER < 6
   brw_emit_query_begin(brw);
#endif

   const struct brw_vs_prog_data *vs_prog_data =
      brw_vs_prog_data(brw->vs.base.prog_data);

#if GFX_VER >= 8
   struct gl_context *ctx = &brw->ctx;
   const bool uses_edge_flag = (ctx->Polygon.FrontMode != GL_FILL ||
                                ctx->Polygon.BackMode != GL_FILL);

   if (vs_prog_data->uses_vertexid || vs_prog_data->uses_instanceid) {
      unsigned vue = brw->vb.nr_enabled;

      /* The element for the edge flags must always be last, so we have to
       * insert the SGVS before it in that case.
       */
      if (uses_edge_flag) {
         assert(vue > 0);
         vue--;
      }

      WARN_ONCE(vue >= 33,
                "Trying to insert VID/IID past 33rd vertex element, "
                "need to reorder the vertex attrbutes.");

      brw_batch_emit(brw, GENX(3DSTATE_VF_SGVS), vfs) {
         if (vs_prog_data->uses_vertexid) {
            vfs.VertexIDEnable = true;
            vfs.VertexIDComponentNumber = 2;
            vfs.VertexIDElementOffset = vue;
         }

         if (vs_prog_data->uses_instanceid) {
            vfs.InstanceIDEnable = true;
            vfs.InstanceIDComponentNumber = 3;
            vfs.InstanceIDElementOffset = vue;
         }
      }

      brw_batch_emit(brw, GENX(3DSTATE_VF_INSTANCING), vfi) {
         vfi.InstancingEnable = true;
         vfi.VertexElementIndex = vue;
      }
   } else {
      brw_batch_emit(brw, GENX(3DSTATE_VF_SGVS), vfs);
   }
#endif

   const bool uses_draw_params =
      vs_prog_data->uses_firstvertex ||
      vs_prog_data->uses_baseinstance;

   const bool uses_derived_draw_params =
      vs_prog_data->uses_drawid ||
      vs_prog_data->uses_is_indexed_draw;

   const bool needs_sgvs_element = (uses_draw_params ||
                                    vs_prog_data->uses_instanceid ||
                                    vs_prog_data->uses_vertexid);

   unsigned nr_elements =
      brw->vb.nr_enabled + needs_sgvs_element + uses_derived_draw_params;

#if GFX_VER < 8
   /* If any of the formats of vb.enabled needs more that one upload, we need
    * to add it to nr_elements
    */
   for (unsigned i = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];
      uint32_t format = brw_get_vertex_surface_type(brw, input->glformat);

      if (uploads_needed(format, input->is_dual_slot) > 1)
         nr_elements++;
   }
#endif

   /* If the VS doesn't read any inputs (calculating vertex position from
    * a state variable for some reason, for example), emit a single pad
    * VERTEX_ELEMENT struct and bail.
    *
    * The stale VB state stays in place, but they don't do anything unless
    * a VE loads from them.
    */
   if (nr_elements == 0) {
      dw = brw_batch_emitn(brw, GENX(3DSTATE_VERTEX_ELEMENTS),
                           1 + GENX(VERTEX_ELEMENT_STATE_length));
      struct GENX(VERTEX_ELEMENT_STATE) elem = {
         .Valid = true,
         .SourceElementFormat = ISL_FORMAT_R32G32B32A32_FLOAT,
         .Component0Control = VFCOMP_STORE_0,
         .Component1Control = VFCOMP_STORE_0,
         .Component2Control = VFCOMP_STORE_0,
         .Component3Control = VFCOMP_STORE_1_FP,
      };
      GENX(VERTEX_ELEMENT_STATE_pack)(brw, dw, &elem);
      return;
   }

   /* Now emit 3DSTATE_VERTEX_BUFFERS and 3DSTATE_VERTEX_ELEMENTS packets. */
   const unsigned nr_buffers = brw->vb.nr_buffers +
      uses_draw_params + uses_derived_draw_params;

   vf_invalidate_for_vb_48bit_transitions(brw);

   if (nr_buffers) {
      assert(nr_buffers <= (GFX_VER >= 6 ? 33 : 17));

      dw = brw_batch_emitn(brw, GENX(3DSTATE_VERTEX_BUFFERS),
                           1 + GENX(VERTEX_BUFFER_STATE_length) * nr_buffers);

      for (unsigned i = 0; i < brw->vb.nr_buffers; i++) {
         const struct brw_vertex_buffer *buffer = &brw->vb.buffers[i];
         /* Prior to Haswell and Bay Trail we have to use 4-component formats
          * to fake 3-component ones.  In particular, we do this for
          * half-float and 8 and 16-bit integer formats.  This means that the
          * vertex element may poke over the end of the buffer by 2 bytes.
          */
         const unsigned padding =
            (GFX_VERx10 < 75 && !devinfo->is_baytrail) * 2;
         const unsigned end = buffer->offset + buffer->size + padding;
         dw = genX(emit_vertex_buffer_state)(brw, dw, i, buffer->bo,
                                             buffer->offset,
                                             end,
                                             buffer->stride,
                                             buffer->step_rate);
      }

      if (uses_draw_params) {
         dw = genX(emit_vertex_buffer_state)(brw, dw, brw->vb.nr_buffers,
                                             brw->draw.draw_params_bo,
                                             brw->draw.draw_params_offset,
                                             brw->draw.draw_params_bo->size,
                                             0 /* stride */,
                                             0 /* step rate */);
      }

      if (uses_derived_draw_params) {
         dw = genX(emit_vertex_buffer_state)(brw, dw, brw->vb.nr_buffers + 1,
                                             brw->draw.derived_draw_params_bo,
                                             brw->draw.derived_draw_params_offset,
                                             brw->draw.derived_draw_params_bo->size,
                                             0 /* stride */,
                                             0 /* step rate */);
      }
   }

   /* The hardware allows one more VERTEX_ELEMENTS than VERTEX_BUFFERS,
    * presumably for VertexID/InstanceID.
    */
#if GFX_VER >= 6
   assert(nr_elements <= 34);
   const struct brw_vertex_element *gfx6_edgeflag_input = NULL;
#else
   assert(nr_elements <= 18);
#endif

   dw = brw_batch_emitn(brw, GENX(3DSTATE_VERTEX_ELEMENTS),
                        1 + GENX(VERTEX_ELEMENT_STATE_length) * nr_elements);
   unsigned i;
   for (i = 0; i < brw->vb.nr_enabled; i++) {
      const struct brw_vertex_element *input = brw->vb.enabled[i];
      const struct gl_vertex_format *glformat = input->glformat;
      uint32_t format = brw_get_vertex_surface_type(brw, glformat);
      uint32_t comp0 = VFCOMP_STORE_SRC;
      uint32_t comp1 = VFCOMP_STORE_SRC;
      uint32_t comp2 = VFCOMP_STORE_SRC;
      uint32_t comp3 = VFCOMP_STORE_SRC;
      const unsigned num_uploads = GFX_VER < 8 ?
         uploads_needed(format, input->is_dual_slot) : 1;

#if GFX_VER >= 8
      /* From the BDW PRM, Volume 2d, page 588 (VERTEX_ELEMENT_STATE):
       * "Any SourceElementFormat of *64*_PASSTHRU cannot be used with an
       * element which has edge flag enabled."
       */
      assert(!(is_passthru_format(format) && uses_edge_flag));
#endif

      /* The gfx4 driver expects edgeflag to come in as a float, and passes
       * that float on to the tests in the clipper.  Mesa's current vertex
       * attribute value for EdgeFlag is stored as a float, which works out.
       * glEdgeFlagPointer, on the other hand, gives us an unnormalized
       * integer ubyte.  Just rewrite that to convert to a float.
       *
       * Gfx6+ passes edgeflag as sideband along with the vertex, instead
       * of in the VUE.  We have to upload it sideband as the last vertex
       * element according to the B-Spec.
       */
#if GFX_VER >= 6
      if (input == &brw->vb.inputs[VERT_ATTRIB_EDGEFLAG]) {
         gfx6_edgeflag_input = input;
         continue;
      }
#endif

      for (unsigned c = 0; c < num_uploads; c++) {
         const uint32_t upload_format = GFX_VER >= 8 ? format :
            downsize_format_if_needed(format, c);
         /* If we need more that one upload, the offset stride would be 128
          * bits (16 bytes), as for previous uploads we are using the full
          * entry. */
         const unsigned offset = input->offset + c * 16;

         const int size = (GFX_VER < 8 && is_passthru_format(format)) ?
            upload_format_size(upload_format) : glformat->Size;

         switch (size) {
            case 0: comp0 = VFCOMP_STORE_0; FALLTHROUGH;
            case 1: comp1 = VFCOMP_STORE_0; FALLTHROUGH;
            case 2: comp2 = VFCOMP_STORE_0; FALLTHROUGH;
            case 3:
               if (GFX_VER >= 8 && glformat->Doubles) {
                  comp3 = VFCOMP_STORE_0;
               } else if (glformat->Integer) {
                  comp3 = VFCOMP_STORE_1_INT;
               } else {
                  comp3 = VFCOMP_STORE_1_FP;
               }

               break;
         }

#if GFX_VER >= 8
         /* From the BDW PRM, Volume 2d, page 586 (VERTEX_ELEMENT_STATE):
          *
          *     "When SourceElementFormat is set to one of the *64*_PASSTHRU
          *     formats, 64-bit components are stored in the URB without any
          *     conversion. In this case, vertex elements must be written as 128
          *     or 256 bits, with VFCOMP_STORE_0 being used to pad the output as
          *     required. E.g., if R64_PASSTHRU is used to copy a 64-bit Red
          *     component into the URB, Component 1 must be specified as
          *     VFCOMP_STORE_0 (with Components 2,3 set to VFCOMP_NOSTORE) in
          *     order to output a 128-bit vertex element, or Components 1-3 must
          *     be specified as VFCOMP_STORE_0 in order to output a 256-bit vertex
          *     element. Likewise, use of R64G64B64_PASSTHRU requires Component 3
          *     to be specified as VFCOMP_STORE_0 in order to output a 256-bit
          *     vertex element."
          */
         if (glformat->Doubles && !input->is_dual_slot) {
            /* Store vertex elements which correspond to double and dvec2 vertex
             * shader inputs as 128-bit vertex elements, instead of 256-bits.
             */
            comp2 = VFCOMP_NOSTORE;
            comp3 = VFCOMP_NOSTORE;
         }
#endif

         struct GENX(VERTEX_ELEMENT_STATE) elem_state = {
            .VertexBufferIndex = input->buffer,
            .Valid = true,
            .SourceElementFormat = upload_format,
            .SourceElementOffset = offset,
            .Component0Control = comp0,
            .Component1Control = comp1,
            .Component2Control = comp2,
            .Component3Control = comp3,
#if GFX_VER < 5
            .DestinationElementOffset = i * 4,
#endif
         };

         GENX(VERTEX_ELEMENT_STATE_pack)(brw, dw, &elem_state);
         dw += GENX(VERTEX_ELEMENT_STATE_length);
      }
   }

   if (needs_sgvs_element) {
      struct GENX(VERTEX_ELEMENT_STATE) elem_state = {
         .Valid = true,
         .Component0Control = VFCOMP_STORE_0,
         .Component1Control = VFCOMP_STORE_0,
         .Component2Control = VFCOMP_STORE_0,
         .Component3Control = VFCOMP_STORE_0,
#if GFX_VER < 5
         .DestinationElementOffset = i * 4,
#endif
      };

#if GFX_VER >= 8
      if (uses_draw_params) {
         elem_state.VertexBufferIndex = brw->vb.nr_buffers;
         elem_state.SourceElementFormat = ISL_FORMAT_R32G32_UINT;
         elem_state.Component0Control = VFCOMP_STORE_SRC;
         elem_state.Component1Control = VFCOMP_STORE_SRC;
      }
#else
      elem_state.VertexBufferIndex = brw->vb.nr_buffers;
      elem_state.SourceElementFormat = ISL_FORMAT_R32G32_UINT;
      if (uses_draw_params) {
         elem_state.Component0Control = VFCOMP_STORE_SRC;
         elem_state.Component1Control = VFCOMP_STORE_SRC;
      }

      if (vs_prog_data->uses_vertexid)
         elem_state.Component2Control = VFCOMP_STORE_VID;

      if (vs_prog_data->uses_instanceid)
         elem_state.Component3Control = VFCOMP_STORE_IID;
#endif

      GENX(VERTEX_ELEMENT_STATE_pack)(brw, dw, &elem_state);
      dw += GENX(VERTEX_ELEMENT_STATE_length);
   }

   if (uses_derived_draw_params) {
      struct GENX(VERTEX_ELEMENT_STATE) elem_state = {
         .Valid = true,
         .VertexBufferIndex = brw->vb.nr_buffers + 1,
         .SourceElementFormat = ISL_FORMAT_R32G32_UINT,
         .Component0Control = VFCOMP_STORE_SRC,
         .Component1Control = VFCOMP_STORE_SRC,
         .Component2Control = VFCOMP_STORE_0,
         .Component3Control = VFCOMP_STORE_0,
#if GFX_VER < 5
         .DestinationElementOffset = i * 4,
#endif
      };

      GENX(VERTEX_ELEMENT_STATE_pack)(brw, dw, &elem_state);
      dw += GENX(VERTEX_ELEMENT_STATE_length);
   }

#if GFX_VER >= 6
   if (gfx6_edgeflag_input) {
      const struct gl_vertex_format *glformat = gfx6_edgeflag_input->glformat;
      const uint32_t format = brw_get_vertex_surface_type(brw, glformat);

      struct GENX(VERTEX_ELEMENT_STATE) elem_state = {
         .Valid = true,
         .VertexBufferIndex = gfx6_edgeflag_input->buffer,
         .EdgeFlagEnable = true,
         .SourceElementFormat = format,
         .SourceElementOffset = gfx6_edgeflag_input->offset,
         .Component0Control = VFCOMP_STORE_SRC,
         .Component1Control = VFCOMP_STORE_0,
         .Component2Control = VFCOMP_STORE_0,
         .Component3Control = VFCOMP_STORE_0,
      };

      GENX(VERTEX_ELEMENT_STATE_pack)(brw, dw, &elem_state);
      dw += GENX(VERTEX_ELEMENT_STATE_length);
   }
#endif

#if GFX_VER >= 8
   for (unsigned i = 0, j = 0; i < brw->vb.nr_enabled; i++) {
      const struct brw_vertex_element *input = brw->vb.enabled[i];
      const struct brw_vertex_buffer *buffer = &brw->vb.buffers[input->buffer];
      unsigned element_index;

      /* The edge flag element is reordered to be the last one in the code
       * above so we need to compensate for that in the element indices used
       * below.
       */
      if (input == gfx6_edgeflag_input)
         element_index = nr_elements - 1;
      else
         element_index = j++;

      brw_batch_emit(brw, GENX(3DSTATE_VF_INSTANCING), vfi) {
         vfi.VertexElementIndex = element_index;
         vfi.InstancingEnable = buffer->step_rate != 0;
         vfi.InstanceDataStepRate = buffer->step_rate;
      }
   }

   if (vs_prog_data->uses_drawid) {
      const unsigned element = brw->vb.nr_enabled + needs_sgvs_element;

      brw_batch_emit(brw, GENX(3DSTATE_VF_INSTANCING), vfi) {
         vfi.VertexElementIndex = element;
      }
   }
#endif
}

static const struct brw_tracked_state genX(vertices) = {
   .dirty = {
      .mesa = _NEW_POLYGON,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_VERTEX_PROGRAM |
             BRW_NEW_VERTICES |
             BRW_NEW_VS_PROG_DATA,
   },
   .emit = genX(emit_vertices),
};

static void
genX(emit_index_buffer)(struct brw_context *brw)
{
   const struct _mesa_index_buffer *index_buffer = brw->ib.ib;

   if (index_buffer == NULL)
      return;

   vf_invalidate_for_ib_48bit_transition(brw);

   brw_batch_emit(brw, GENX(3DSTATE_INDEX_BUFFER), ib) {
#if GFX_VERx10 < 75
      assert(brw->ib.enable_cut_index == brw->prim_restart.enable_cut_index);
      ib.CutIndexEnable = brw->ib.enable_cut_index;
#endif
      ib.IndexFormat = brw_get_index_type(1 << index_buffer->index_size_shift);

      /* The VF cache designers apparently cut corners, and made the cache
       * only consider the bottom 32 bits of memory addresses.  If you happen
       * to have two index buffers which get placed exactly 4 GiB apart and
       * use them in back-to-back draw calls, you can get collisions.  To work
       * around this problem, we restrict index buffers to the low 32 bits of
       * the address space.
       */
      ib.BufferStartingAddress = ro_32_bo(brw->ib.bo, 0);
#if GFX_VER >= 8
      ib.MOCS = GFX_VER >= 9 ? SKL_MOCS_WB : BDW_MOCS_WB;
      ib.BufferSize = brw->ib.size;
#else
      ib.BufferEndingAddress = ro_bo(brw->ib.bo, brw->ib.size - 1);
#endif
   }
}

static const struct brw_tracked_state genX(index_buffer) = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_INDEX_BUFFER,
   },
   .emit = genX(emit_index_buffer),
};

#if GFX_VERx10 >= 75
static void
genX(upload_cut_index)(struct brw_context *brw)
{
   brw_batch_emit(brw, GENX(3DSTATE_VF), vf) {
      if (brw->prim_restart.enable_cut_index && brw->ib.ib) {
         vf.IndexedDrawCutIndexEnable = true;
         vf.CutIndex = brw->prim_restart.restart_index;
      }
   }
}

const struct brw_tracked_state genX(cut_index) = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM,
      .brw   = BRW_NEW_INDEX_BUFFER,
   },
   .emit = genX(upload_cut_index),
};
#endif

static void
genX(upload_vf_statistics)(struct brw_context *brw)
{
   brw_batch_emit(brw, GENX(3DSTATE_VF_STATISTICS), vf) {
      vf.StatisticsEnable = true;
   }
}

const struct brw_tracked_state genX(vf_statistics) = {
   .dirty = {
      .mesa  = 0,
      .brw   = BRW_NEW_BLORP | BRW_NEW_CONTEXT,
   },
   .emit = genX(upload_vf_statistics),
};

#if GFX_VER >= 6
/**
 * Determine the appropriate attribute override value to store into the
 * 3DSTATE_SF structure for a given fragment shader attribute.  The attribute
 * override value contains two pieces of information: the location of the
 * attribute in the VUE (relative to urb_entry_read_offset, see below), and a
 * flag indicating whether to "swizzle" the attribute based on the direction
 * the triangle is facing.
 *
 * If an attribute is "swizzled", then the given VUE location is used for
 * front-facing triangles, and the VUE location that immediately follows is
 * used for back-facing triangles.  We use this to implement the mapping from
 * gl_FrontColor/gl_BackColor to gl_Color.
 *
 * urb_entry_read_offset is the offset into the VUE at which the SF unit is
 * being instructed to begin reading attribute data.  It can be set to a
 * nonzero value to prevent the SF unit from wasting time reading elements of
 * the VUE that are not needed by the fragment shader.  It is measured in
 * 256-bit increments.
 */
static void
genX(get_attr_override)(struct GENX(SF_OUTPUT_ATTRIBUTE_DETAIL) *attr,
                        const struct brw_vue_map *vue_map,
                        int urb_entry_read_offset, int fs_attr,
                        bool two_side_color, uint32_t *max_source_attr)
{
   /* Find the VUE slot for this attribute. */
   int slot = vue_map->varying_to_slot[fs_attr];

   /* Viewport and Layer are stored in the VUE header.  We need to override
    * them to zero if earlier stages didn't write them, as GL requires that
    * they read back as zero when not explicitly set.
    */
   if (fs_attr == VARYING_SLOT_VIEWPORT || fs_attr == VARYING_SLOT_LAYER) {
      attr->ComponentOverrideX = true;
      attr->ComponentOverrideW = true;
      attr->ConstantSource = CONST_0000;

      if (!(vue_map->slots_valid & VARYING_BIT_LAYER))
         attr->ComponentOverrideY = true;
      if (!(vue_map->slots_valid & VARYING_BIT_VIEWPORT))
         attr->ComponentOverrideZ = true;

      return;
   }

   /* If there was only a back color written but not front, use back
    * as the color instead of undefined
    */
   if (slot == -1 && fs_attr == VARYING_SLOT_COL0)
      slot = vue_map->varying_to_slot[VARYING_SLOT_BFC0];
   if (slot == -1 && fs_attr == VARYING_SLOT_COL1)
      slot = vue_map->varying_to_slot[VARYING_SLOT_BFC1];

   if (slot == -1) {
      /* This attribute does not exist in the VUE--that means that the vertex
       * shader did not write to it.  This means that either:
       *
       * (a) This attribute is a texture coordinate, and it is going to be
       * replaced with point coordinates (as a consequence of a call to
       * glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE)), so the
       * hardware will ignore whatever attribute override we supply.
       *
       * (b) This attribute is read by the fragment shader but not written by
       * the vertex shader, so its value is undefined.  Therefore the
       * attribute override we supply doesn't matter.
       *
       * (c) This attribute is gl_PrimitiveID, and it wasn't written by the
       * previous shader stage.
       *
       * Note that we don't have to worry about the cases where the attribute
       * is gl_PointCoord or is undergoing point sprite coordinate
       * replacement, because in those cases, this function isn't called.
       *
       * In case (c), we need to program the attribute overrides so that the
       * primitive ID will be stored in this slot.  In every other case, the
       * attribute override we supply doesn't matter.  So just go ahead and
       * program primitive ID in every case.
       */
      attr->ComponentOverrideW = true;
      attr->ComponentOverrideX = true;
      attr->ComponentOverrideY = true;
      attr->ComponentOverrideZ = true;
      attr->ConstantSource = PRIM_ID;
      return;
   }

   /* Compute the location of the attribute relative to urb_entry_read_offset.
    * Each increment of urb_entry_read_offset represents a 256-bit value, so
    * it counts for two 128-bit VUE slots.
    */
   int source_attr = slot - 2 * urb_entry_read_offset;
   assert(source_attr >= 0 && source_attr < 32);

   /* If we are doing two-sided color, and the VUE slot following this one
    * represents a back-facing color, then we need to instruct the SF unit to
    * do back-facing swizzling.
    */
   bool swizzling = two_side_color &&
      ((vue_map->slot_to_varying[slot] == VARYING_SLOT_COL0 &&
        vue_map->slot_to_varying[slot+1] == VARYING_SLOT_BFC0) ||
       (vue_map->slot_to_varying[slot] == VARYING_SLOT_COL1 &&
        vue_map->slot_to_varying[slot+1] == VARYING_SLOT_BFC1));

   /* Update max_source_attr.  If swizzling, the SF will read this slot + 1. */
   if (*max_source_attr < source_attr + swizzling)
      *max_source_attr = source_attr + swizzling;

   attr->SourceAttribute = source_attr;
   if (swizzling)
      attr->SwizzleSelect = INPUTATTR_FACING;
}


static void
genX(calculate_attr_overrides)(const struct brw_context *brw,
                               struct GENX(SF_OUTPUT_ATTRIBUTE_DETAIL) *attr_overrides,
                               uint32_t *point_sprite_enables,
                               uint32_t *urb_entry_read_length,
                               uint32_t *urb_entry_read_offset)
{
   const struct gl_context *ctx = &brw->ctx;

   /* _NEW_POINT */
   const struct gl_point_attrib *point = &ctx->Point;

   /* BRW_NEW_FRAGMENT_PROGRAM */
   const struct gl_program *fp = brw->programs[MESA_SHADER_FRAGMENT];

   /* BRW_NEW_FS_PROG_DATA */
   const struct brw_wm_prog_data *wm_prog_data =
      brw_wm_prog_data(brw->wm.base.prog_data);
   uint32_t max_source_attr = 0;

   *point_sprite_enables = 0;

   int first_slot =
      brw_compute_first_urb_slot_required(fp->info.inputs_read,
                                          &brw->vue_map_geom_out);

   /* Each URB offset packs two varying slots */
   assert(first_slot % 2 == 0);
   *urb_entry_read_offset = first_slot / 2;

   /* From the Ivybridge PRM, Vol 2 Part 1, 3DSTATE_SBE,
    * description of dw10 Point Sprite Texture Coordinate Enable:
    *
    * "This field must be programmed to zero when non-point primitives
    * are rendered."
    *
    * The SandyBridge PRM doesn't explicitly say that point sprite enables
    * must be programmed to zero when rendering non-point primitives, but
    * the IvyBridge PRM does, and if we don't, we get garbage.
    *
    * This is not required on Haswell, as the hardware ignores this state
    * when drawing non-points -- although we do still need to be careful to
    * correctly set the attr overrides.
    *
    * _NEW_POLYGON
    * BRW_NEW_PRIMITIVE | BRW_NEW_GS_PROG_DATA | BRW_NEW_TES_PROG_DATA
    */
   bool drawing_points = brw_is_drawing_points(brw);

   for (uint8_t idx = 0; idx < wm_prog_data->urb_setup_attribs_count; idx++) {
      uint8_t attr = wm_prog_data->urb_setup_attribs[idx];
      int input_index = wm_prog_data->urb_setup[attr];

      assert(0 <= input_index);

      /* _NEW_POINT */
      bool point_sprite = false;
      if (drawing_points) {
         if (point->PointSprite &&
             (attr >= VARYING_SLOT_TEX0 && attr <= VARYING_SLOT_TEX7) &&
             (point->CoordReplace & (1u << (attr - VARYING_SLOT_TEX0)))) {
            point_sprite = true;
         }

         if (attr == VARYING_SLOT_PNTC)
            point_sprite = true;

         if (point_sprite)
            *point_sprite_enables |= (1 << input_index);
      }

      /* BRW_NEW_VUE_MAP_GEOM_OUT | _NEW_LIGHT | _NEW_PROGRAM */
      struct GENX(SF_OUTPUT_ATTRIBUTE_DETAIL) attribute = { 0 };

      if (!point_sprite) {
         genX(get_attr_override)(&attribute,
                                 &brw->vue_map_geom_out,
                                 *urb_entry_read_offset, attr,
                                 _mesa_vertex_program_two_side_enabled(ctx),
                                 &max_source_attr);
      }

      /* The hardware can only do the overrides on 16 overrides at a
       * time, and the other up to 16 have to be lined up so that the
       * input index = the output index.  We'll need to do some
       * tweaking to make sure that's the case.
       */
      if (input_index < 16)
         attr_overrides[input_index] = attribute;
      else
         assert(attribute.SourceAttribute == input_index);
   }

   /* From the Sandy Bridge PRM, Volume 2, Part 1, documentation for
    * 3DSTATE_SF DWord 1 bits 15:11, "Vertex URB Entry Read Length":
    *
    * "This field should be set to the minimum length required to read the
    *  maximum source attribute.  The maximum source attribute is indicated
    *  by the maximum value of the enabled Attribute # Source Attribute if
    *  Attribute Swizzle Enable is set, Number of Output Attributes-1 if
    *  enable is not set.
    *  read_length = ceiling((max_source_attr + 1) / 2)
    *
    *  [errata] Corruption/Hang possible if length programmed larger than
    *  recommended"
    *
    * Similar text exists for Ivy Bridge.
    */
   *urb_entry_read_length = DIV_ROUND_UP(max_source_attr + 1, 2);
}
#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER >= 8
typedef struct GENX(3DSTATE_WM_DEPTH_STENCIL) DEPTH_STENCIL_GENXML;
#elif GFX_VER >= 6
typedef struct GENX(DEPTH_STENCIL_STATE)      DEPTH_STENCIL_GENXML;
#else
typedef struct GENX(COLOR_CALC_STATE)         DEPTH_STENCIL_GENXML;
#endif

static inline void
set_depth_stencil_bits(struct brw_context *brw, DEPTH_STENCIL_GENXML *ds)
{
   struct gl_context *ctx = &brw->ctx;

   /* _NEW_BUFFERS */
   struct brw_renderbuffer *depth_irb =
      brw_get_renderbuffer(ctx->DrawBuffer, BUFFER_DEPTH);

   /* _NEW_DEPTH */
   struct gl_depthbuffer_attrib *depth = &ctx->Depth;

   /* _NEW_STENCIL */
   struct gl_stencil_attrib *stencil = &ctx->Stencil;
   const int b = stencil->_BackFace;

   if (depth->Test && depth_irb) {
      ds->DepthTestEnable = true;
      ds->DepthBufferWriteEnable = brw_depth_writes_enabled(brw);
      ds->DepthTestFunction = brw_translate_compare_func(depth->Func);
   }

   if (brw->stencil_enabled) {
      ds->StencilTestEnable = true;
      ds->StencilWriteMask = stencil->WriteMask[0] & 0xff;
      ds->StencilTestMask = stencil->ValueMask[0] & 0xff;

      ds->StencilTestFunction =
         brw_translate_compare_func(stencil->Function[0]);
      ds->StencilFailOp =
         brw_translate_stencil_op(stencil->FailFunc[0]);
      ds->StencilPassDepthPassOp =
         brw_translate_stencil_op(stencil->ZPassFunc[0]);
      ds->StencilPassDepthFailOp =
         brw_translate_stencil_op(stencil->ZFailFunc[0]);

      ds->StencilBufferWriteEnable = brw->stencil_write_enabled;

      if (brw->stencil_two_sided) {
         ds->DoubleSidedStencilEnable = true;
         ds->BackfaceStencilWriteMask = stencil->WriteMask[b] & 0xff;
         ds->BackfaceStencilTestMask = stencil->ValueMask[b] & 0xff;

         ds->BackfaceStencilTestFunction =
            brw_translate_compare_func(stencil->Function[b]);
         ds->BackfaceStencilFailOp =
            brw_translate_stencil_op(stencil->FailFunc[b]);
         ds->BackfaceStencilPassDepthPassOp =
            brw_translate_stencil_op(stencil->ZPassFunc[b]);
         ds->BackfaceStencilPassDepthFailOp =
            brw_translate_stencil_op(stencil->ZFailFunc[b]);
      }

#if GFX_VER <= 5 || GFX_VER >= 9
      ds->StencilReferenceValue = _mesa_get_stencil_ref(ctx, 0);
      ds->BackfaceStencilReferenceValue = _mesa_get_stencil_ref(ctx, b);
#endif
   }
}

#if GFX_VER >= 6
static void
genX(upload_depth_stencil_state)(struct brw_context *brw)
{
#if GFX_VER >= 8
   brw_batch_emit(brw, GENX(3DSTATE_WM_DEPTH_STENCIL), wmds) {
      set_depth_stencil_bits(brw, &wmds);
   }
#else
   uint32_t ds_offset;
   brw_state_emit(brw, GENX(DEPTH_STENCIL_STATE), 64, &ds_offset, ds) {
      set_depth_stencil_bits(brw, &ds);
   }

   /* Now upload a pointer to the indirect state */
#if GFX_VER == 6
   brw_batch_emit(brw, GENX(3DSTATE_CC_STATE_POINTERS), ptr) {
      ptr.PointertoDEPTH_STENCIL_STATE = ds_offset;
      ptr.DEPTH_STENCIL_STATEChange = true;
   }
#else
   brw_batch_emit(brw, GENX(3DSTATE_DEPTH_STENCIL_STATE_POINTERS), ptr) {
      ptr.PointertoDEPTH_STENCIL_STATE = ds_offset;
   }
#endif
#endif
}

static const struct brw_tracked_state genX(depth_stencil_state) = {
   .dirty = {
      .mesa = _NEW_BUFFERS |
              _NEW_DEPTH |
              _NEW_STENCIL,
      .brw  = BRW_NEW_BLORP |
              (GFX_VER >= 8 ? BRW_NEW_CONTEXT
                            : BRW_NEW_BATCH |
                              BRW_NEW_STATE_BASE_ADDRESS),
   },
   .emit = genX(upload_depth_stencil_state),
};
#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER <= 5

static void
genX(upload_clip_state)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   ctx->NewDriverState |= BRW_NEW_GFX4_UNIT_STATE;
   brw_state_emit(brw, GENX(CLIP_STATE), 32, &brw->clip.state_offset, clip) {
      clip.KernelStartPointer = KSP(brw, brw->clip.prog_offset);
      clip.GRFRegisterCount =
         DIV_ROUND_UP(brw->clip.prog_data->total_grf, 16) - 1;
      clip.FloatingPointMode = FLOATING_POINT_MODE_Alternate;
      clip.SingleProgramFlow = true;
      clip.VertexURBEntryReadLength = brw->clip.prog_data->urb_read_length;
      clip.ConstantURBEntryReadLength = brw->clip.prog_data->curb_read_length;

      /* BRW_NEW_PUSH_CONSTANT_ALLOCATION */
      clip.ConstantURBEntryReadOffset = brw->curbe.clip_start * 2;
      clip.DispatchGRFStartRegisterForURBData = 1;
      clip.VertexURBEntryReadOffset = 0;

      /* BRW_NEW_URB_FENCE */
      clip.NumberofURBEntries = brw->urb.nr_clip_entries;
      clip.URBEntryAllocationSize = brw->urb.vsize - 1;

      if (brw->urb.nr_clip_entries >= 10) {
         /* Half of the URB entries go to each thread, and it has to be an
          * even number.
          */
         assert(brw->urb.nr_clip_entries % 2 == 0);

         /* Although up to 16 concurrent Clip threads are allowed on Ironlake,
          * only 2 threads can output VUEs at a time.
          */
         clip.MaximumNumberofThreads = (GFX_VER == 5 ? 16 : 2) - 1;
      } else {
         assert(brw->urb.nr_clip_entries >= 5);
         clip.MaximumNumberofThreads = 1 - 1;
      }

      clip.VertexPositionSpace = VPOS_NDCSPACE;
      clip.UserClipFlagsMustClipEnable = true;
      clip.GuardbandClipTestEnable = true;

      clip.ClipperViewportStatePointer =
         ro_bo(brw->batch.state.bo, brw->clip.vp_offset);

      clip.ScreenSpaceViewportXMin = -1;
      clip.ScreenSpaceViewportXMax = 1;
      clip.ScreenSpaceViewportYMin = -1;
      clip.ScreenSpaceViewportYMax = 1;

      clip.ViewportXYClipTestEnable = true;
      clip.ViewportZClipTestEnable = !(ctx->Transform.DepthClampNear &&
                                       ctx->Transform.DepthClampFar);

      /* _NEW_TRANSFORM */
      if (GFX_VER == 5 || GFX_VERx10 == 45) {
         clip.UserClipDistanceClipTestEnableBitmask =
            ctx->Transform.ClipPlanesEnabled;
      } else {
         /* Up to 6 actual clip flags, plus the 7th for the negative RHW
          * workaround.
          */
         clip.UserClipDistanceClipTestEnableBitmask =
            (ctx->Transform.ClipPlanesEnabled & 0x3f) | 0x40;
      }

      if (ctx->Transform.ClipDepthMode == GL_ZERO_TO_ONE)
         clip.APIMode = APIMODE_D3D;
      else
         clip.APIMode = APIMODE_OGL;

      clip.GuardbandClipTestEnable = true;

      clip.ClipMode = brw->clip.prog_data->clip_mode;

#if GFX_VERx10 == 45
      clip.NegativeWClipTestEnable = true;
#endif
   }
}

const struct brw_tracked_state genX(clip_state) = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM |
               _NEW_VIEWPORT,
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               BRW_NEW_CLIP_PROG_DATA |
               BRW_NEW_PUSH_CONSTANT_ALLOCATION |
               BRW_NEW_PROGRAM_CACHE |
               BRW_NEW_URB_FENCE,
   },
   .emit = genX(upload_clip_state),
};

#else

static void
genX(upload_clip_state)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   /* _NEW_BUFFERS */
   struct gl_framebuffer *fb = ctx->DrawBuffer;

   /* BRW_NEW_FS_PROG_DATA */
   struct brw_wm_prog_data *wm_prog_data =
      brw_wm_prog_data(brw->wm.base.prog_data);

   brw_batch_emit(brw, GENX(3DSTATE_CLIP), clip) {
      clip.StatisticsEnable = !brw->meta_in_progress;

      if (wm_prog_data->barycentric_interp_modes &
          BRW_BARYCENTRIC_NONPERSPECTIVE_BITS)
         clip.NonPerspectiveBarycentricEnable = true;

#if GFX_VER >= 7
      clip.EarlyCullEnable = true;
#endif

#if GFX_VER == 7
      clip.FrontWinding = brw->polygon_front_bit != fb->FlipY;

      if (ctx->Polygon.CullFlag) {
         switch (ctx->Polygon.CullFaceMode) {
         case GL_FRONT:
            clip.CullMode = CULLMODE_FRONT;
            break;
         case GL_BACK:
            clip.CullMode = CULLMODE_BACK;
            break;
         case GL_FRONT_AND_BACK:
            clip.CullMode = CULLMODE_BOTH;
            break;
         default:
            unreachable("Should not get here: invalid CullFlag");
         }
      } else {
         clip.CullMode = CULLMODE_NONE;
      }
#endif

#if GFX_VER < 8
      clip.UserClipDistanceCullTestEnableBitmask =
         brw_vue_prog_data(brw->vs.base.prog_data)->cull_distance_mask;

      clip.ViewportZClipTestEnable = !(ctx->Transform.DepthClampNear &&
                                       ctx->Transform.DepthClampFar);
#endif

      /* _NEW_LIGHT */
      if (ctx->Light.ProvokingVertex == GL_FIRST_VERTEX_CONVENTION) {
         clip.TriangleStripListProvokingVertexSelect = 0;
         clip.TriangleFanProvokingVertexSelect = 1;
         clip.LineStripListProvokingVertexSelect = 0;
      } else {
         clip.TriangleStripListProvokingVertexSelect = 2;
         clip.TriangleFanProvokingVertexSelect = 2;
         clip.LineStripListProvokingVertexSelect = 1;
      }

      /* _NEW_TRANSFORM */
      clip.UserClipDistanceClipTestEnableBitmask =
         ctx->Transform.ClipPlanesEnabled;

#if GFX_VER >= 8
      clip.ForceUserClipDistanceClipTestEnableBitmask = true;
#endif

      if (ctx->Transform.ClipDepthMode == GL_ZERO_TO_ONE)
         clip.APIMode = APIMODE_D3D;
      else
         clip.APIMode = APIMODE_OGL;

      clip.GuardbandClipTestEnable = true;

      /* BRW_NEW_VIEWPORT_COUNT */
      const unsigned viewport_count = brw->clip.viewport_count;

      if (ctx->RasterDiscard) {
         clip.ClipMode = CLIPMODE_REJECT_ALL;
#if GFX_VER == 6
         perf_debug("Rasterizer discard is currently implemented via the "
                    "clipper; having the GS not write primitives would "
                    "likely be faster.\n");
#endif
      } else {
         clip.ClipMode = CLIPMODE_NORMAL;
      }

      clip.ClipEnable = true;

      /* _NEW_POLYGON,
       * BRW_NEW_GEOMETRY_PROGRAM | BRW_NEW_TES_PROG_DATA | BRW_NEW_PRIMITIVE
       */
      if (!brw_is_drawing_points(brw) && !brw_is_drawing_lines(brw))
         clip.ViewportXYClipTestEnable = true;

      clip.MinimumPointWidth = 0.125;
      clip.MaximumPointWidth = 255.875;
      clip.MaximumVPIndex = viewport_count - 1;
      if (_mesa_geometric_layers(fb) == 0)
         clip.ForceZeroRTAIndexEnable = true;
   }
}

static const struct brw_tracked_state genX(clip_state) = {
   .dirty = {
      .mesa  = _NEW_BUFFERS |
               _NEW_LIGHT |
               _NEW_POLYGON |
               _NEW_TRANSFORM,
      .brw   = BRW_NEW_BLORP |
               BRW_NEW_CONTEXT |
               BRW_NEW_FS_PROG_DATA |
               BRW_NEW_GS_PROG_DATA |
               BRW_NEW_VS_PROG_DATA |
               BRW_NEW_META_IN_PROGRESS |
               BRW_NEW_PRIMITIVE |
               BRW_NEW_RASTERIZER_DISCARD |
               BRW_NEW_TES_PROG_DATA |
               BRW_NEW_VIEWPORT_COUNT,
   },
   .emit = genX(upload_clip_state),
};
#endif

/* ---------------------------------------------------------------------- */

static void
genX(upload_sf)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   float point_size;

#if GFX_VER <= 7
   /* _NEW_BUFFERS */
   bool flip_y = ctx->DrawBuffer->FlipY;
   UNUSED const bool multisampled_fbo =
      _mesa_geometric_samples(ctx->DrawBuffer) > 1;
#endif

#if GFX_VER < 6
   const struct brw_sf_prog_data *sf_prog_data = brw->sf.prog_data;

   ctx->NewDriverState |= BRW_NEW_GFX4_UNIT_STATE;

   brw_state_emit(brw, GENX(SF_STATE), 64, &brw->sf.state_offset, sf) {
      sf.KernelStartPointer = KSP(brw, brw->sf.prog_offset);
      sf.FloatingPointMode = FLOATING_POINT_MODE_Alternate;
      sf.GRFRegisterCount = DIV_ROUND_UP(sf_prog_data->total_grf, 16) - 1;
      sf.DispatchGRFStartRegisterForURBData = 3;
      sf.VertexURBEntryReadOffset = BRW_SF_URB_ENTRY_READ_OFFSET;
      sf.VertexURBEntryReadLength = sf_prog_data->urb_read_length;
      sf.NumberofURBEntries = brw->urb.nr_sf_entries;
      sf.URBEntryAllocationSize = brw->urb.sfsize - 1;

      /* STATE_PREFETCH command description describes this state as being
       * something loaded through the GPE (L2 ISC), so it's INSTRUCTION
       * domain.
       */
      sf.SetupViewportStateOffset =
         ro_bo(brw->batch.state.bo, brw->sf.vp_offset);

      sf.PointRasterizationRule = RASTRULE_UPPER_RIGHT;

      /* sf.ConstantURBEntryReadLength = stage_prog_data->curb_read_length; */
      /* sf.ConstantURBEntryReadOffset = brw->curbe.vs_start * 2; */

      sf.MaximumNumberofThreads =
         MIN2(GFX_VER == 5 ? 48 : 24, brw->urb.nr_sf_entries) - 1;

      sf.SpritePointEnable = ctx->Point.PointSprite;

      sf.DestinationOriginHorizontalBias = 0.5;
      sf.DestinationOriginVerticalBias = 0.5;
#else
   brw_batch_emit(brw, GENX(3DSTATE_SF), sf) {
      sf.StatisticsEnable = true;
#endif
      sf.ViewportTransformEnable = true;

#if GFX_VER == 7
      /* _NEW_BUFFERS */
      sf.DepthBufferSurfaceFormat = brw_depthbuffer_format(brw);
#endif

#if GFX_VER <= 7
      /* _NEW_POLYGON */
      sf.FrontWinding = brw->polygon_front_bit != flip_y;
#if GFX_VER >= 6
      sf.GlobalDepthOffsetEnableSolid = ctx->Polygon.OffsetFill;
      sf.GlobalDepthOffsetEnableWireframe = ctx->Polygon.OffsetLine;
      sf.GlobalDepthOffsetEnablePoint = ctx->Polygon.OffsetPoint;

      switch (ctx->Polygon.FrontMode) {
         case GL_FILL:
            sf.FrontFaceFillMode = FILL_MODE_SOLID;
            break;
         case GL_LINE:
            sf.FrontFaceFillMode = FILL_MODE_WIREFRAME;
            break;
         case GL_POINT:
            sf.FrontFaceFillMode = FILL_MODE_POINT;
            break;
         default:
            unreachable("not reached");
      }

      switch (ctx->Polygon.BackMode) {
         case GL_FILL:
            sf.BackFaceFillMode = FILL_MODE_SOLID;
            break;
         case GL_LINE:
            sf.BackFaceFillMode = FILL_MODE_WIREFRAME;
            break;
         case GL_POINT:
            sf.BackFaceFillMode = FILL_MODE_POINT;
            break;
         default:
            unreachable("not reached");
      }

      if (multisampled_fbo && ctx->Multisample.Enabled)
         sf.MultisampleRasterizationMode = MSRASTMODE_ON_PATTERN;

      sf.GlobalDepthOffsetConstant = ctx->Polygon.OffsetUnits * 2;
      sf.GlobalDepthOffsetScale = ctx->Polygon.OffsetFactor;
      sf.GlobalDepthOffsetClamp = ctx->Polygon.OffsetClamp;
#endif

      sf.ScissorRectangleEnable = true;

      if (ctx->Polygon.CullFlag) {
         switch (ctx->Polygon.CullFaceMode) {
            case GL_FRONT:
               sf.CullMode = CULLMODE_FRONT;
               break;
            case GL_BACK:
               sf.CullMode = CULLMODE_BACK;
               break;
            case GL_FRONT_AND_BACK:
               sf.CullMode = CULLMODE_BOTH;
               break;
            default:
               unreachable("not reached");
         }
      } else {
         sf.CullMode = CULLMODE_NONE;
      }

#if GFX_VERx10 == 75
      sf.LineStippleEnable = ctx->Line.StippleFlag;
#endif

#endif

      /* _NEW_LINE */
#if GFX_VER == 8
      const struct intel_device_info *devinfo = &brw->screen->devinfo;

      if (devinfo->is_cherryview)
         sf.CHVLineWidth = brw_get_line_width(brw);
      else
         sf.LineWidth = brw_get_line_width(brw);
#else
      sf.LineWidth = brw_get_line_width(brw);
#endif

      if (ctx->Line.SmoothFlag) {
         sf.LineEndCapAntialiasingRegionWidth = _10pixels;
#if GFX_VER <= 7
         sf.AntialiasingEnable = true;
#endif
      }

      /* _NEW_POINT - Clamp to ARB_point_parameters user limits */
      point_size = CLAMP(ctx->Point.Size, ctx->Point.MinSize, ctx->Point.MaxSize);
      /* Clamp to the hardware limits */
      sf.PointWidth = CLAMP(point_size, 0.125f, 255.875f);

      /* _NEW_PROGRAM | _NEW_POINT, BRW_NEW_VUE_MAP_GEOM_OUT */
      if (use_state_point_size(brw))
         sf.PointWidthSource = State;

#if GFX_VER >= 8
      /* _NEW_POINT | _NEW_MULTISAMPLE */
      if ((ctx->Point.SmoothFlag || _mesa_is_multisample_enabled(ctx)) &&
          !ctx->Point.PointSprite)
         sf.SmoothPointEnable = true;
#endif

#if GFX_VER == 10
      /* _NEW_BUFFERS
       * Smooth Point Enable bit MUST not be set when NUM_MULTISAMPLES > 1.
       */
      const bool multisampled_fbo =
         _mesa_geometric_samples(ctx->DrawBuffer) > 1;
      if (multisampled_fbo)
         sf.SmoothPointEnable = false;
#endif

#if GFX_VERx10 >= 45
      sf.AALineDistanceMode = AALINEDISTANCE_TRUE;
#endif

      /* _NEW_LIGHT */
      if (ctx->Light.ProvokingVertex != GL_FIRST_VERTEX_CONVENTION) {
         sf.TriangleStripListProvokingVertexSelect = 2;
         sf.TriangleFanProvokingVertexSelect = 2;
         sf.LineStripListProvokingVertexSelect = 1;
      } else {
         sf.TriangleFanProvokingVertexSelect = 1;
      }

#if GFX_VER == 6
      /* BRW_NEW_FS_PROG_DATA */
      const struct brw_wm_prog_data *wm_prog_data =
         brw_wm_prog_data(brw->wm.base.prog_data);

      sf.AttributeSwizzleEnable = true;
      sf.NumberofSFOutputAttributes = wm_prog_data->num_varying_inputs;

      /*
       * Window coordinates in an FBO are inverted, which means point
       * sprite origin must be inverted, too.
       */
      if ((ctx->Point.SpriteOrigin == GL_LOWER_LEFT) == flip_y) {
         sf.PointSpriteTextureCoordinateOrigin = LOWERLEFT;
      } else {
         sf.PointSpriteTextureCoordinateOrigin = UPPERLEFT;
      }

      /* BRW_NEW_VUE_MAP_GEOM_OUT | BRW_NEW_FRAGMENT_PROGRAM |
       * _NEW_POINT | _NEW_LIGHT | _NEW_PROGRAM | BRW_NEW_FS_PROG_DATA
       */
      uint32_t urb_entry_read_length;
      uint32_t urb_entry_read_offset;
      uint32_t point_sprite_enables;
      genX(calculate_attr_overrides)(brw, sf.Attribute, &point_sprite_enables,
                                     &urb_entry_read_length,
                                     &urb_entry_read_offset);
      sf.VertexURBEntryReadLength = urb_entry_read_length;
      sf.VertexURBEntryReadOffset = urb_entry_read_offset;
      sf.PointSpriteTextureCoordinateEnable = point_sprite_enables;
      sf.ConstantInterpolationEnable = wm_prog_data->flat_inputs;
#endif
   }
}

static const struct brw_tracked_state genX(sf_state) = {
   .dirty = {
      .mesa  = _NEW_LIGHT |
               _NEW_LINE |
               _NEW_POINT |
               _NEW_PROGRAM |
               (GFX_VER >= 6 ? _NEW_MULTISAMPLE : 0) |
               (GFX_VER <= 7 ? _NEW_BUFFERS | _NEW_POLYGON : 0) |
               (GFX_VER == 10 ? _NEW_BUFFERS : 0),
      .brw   = BRW_NEW_BLORP |
               BRW_NEW_VUE_MAP_GEOM_OUT |
               (GFX_VER <= 5 ? BRW_NEW_BATCH |
                               BRW_NEW_PROGRAM_CACHE |
                               BRW_NEW_SF_PROG_DATA |
                               BRW_NEW_SF_VP |
                               BRW_NEW_URB_FENCE
                             : 0) |
               (GFX_VER >= 6 ? BRW_NEW_CONTEXT : 0) |
               (GFX_VER >= 6 && GFX_VER <= 7 ?
                               BRW_NEW_GS_PROG_DATA |
                               BRW_NEW_PRIMITIVE |
                               BRW_NEW_TES_PROG_DATA
                             : 0) |
               (GFX_VER == 6 ? BRW_NEW_FS_PROG_DATA |
                               BRW_NEW_FRAGMENT_PROGRAM
                             : 0),
   },
   .emit = genX(upload_sf),
};

/* ---------------------------------------------------------------------- */

static bool
brw_color_buffer_write_enabled(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   const struct gl_program *fp = brw->programs[MESA_SHADER_FRAGMENT];
   unsigned i;

   /* _NEW_BUFFERS */
   for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
      struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[i];
      uint64_t outputs_written = fp->info.outputs_written;

      /* _NEW_COLOR */
      if (rb && (outputs_written & BITFIELD64_BIT(FRAG_RESULT_COLOR) ||
                 outputs_written & BITFIELD64_BIT(FRAG_RESULT_DATA0 + i)) &&
          GET_COLORMASK(ctx->Color.ColorMask, i)) {
         return true;
      }
   }

   return false;
}

static void
genX(upload_wm)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   /* BRW_NEW_FS_PROG_DATA */
   const struct brw_wm_prog_data *wm_prog_data =
      brw_wm_prog_data(brw->wm.base.prog_data);

   UNUSED bool writes_depth =
      wm_prog_data->computed_depth_mode != BRW_PSCDEPTH_OFF;
   UNUSED struct brw_stage_state *stage_state = &brw->wm.base;
   UNUSED const struct intel_device_info *devinfo = &brw->screen->devinfo;

#if GFX_VER == 6
   /* We can't fold this into gfx6_upload_wm_push_constants(), because
    * according to the SNB PRM, vol 2 part 1 section 7.2.2
    * (3DSTATE_CONSTANT_PS [DevSNB]):
    *
    *     "[DevSNB]: This packet must be followed by WM_STATE."
    */
   brw_batch_emit(brw, GENX(3DSTATE_CONSTANT_PS), wmcp) {
      if (wm_prog_data->base.nr_params != 0) {
         wmcp.Buffer0Valid = true;
         /* Pointer to the WM constant buffer.  Covered by the set of
          * state flags from gfx6_upload_wm_push_constants.
          */
         wmcp.ConstantBody.PointertoConstantBuffer0 = stage_state->push_const_offset;
         wmcp.ConstantBody.ConstantBuffer0ReadLength = stage_state->push_const_size - 1;
      }
   }
#endif

#if GFX_VER >= 6
   brw_batch_emit(brw, GENX(3DSTATE_WM), wm) {
#else
   ctx->NewDriverState |= BRW_NEW_GFX4_UNIT_STATE;
   brw_state_emit(brw, GENX(WM_STATE), 64, &stage_state->state_offset, wm) {
#endif

#if GFX_VER <= 6
      wm._8PixelDispatchEnable = wm_prog_data->dispatch_8;
      wm._16PixelDispatchEnable = wm_prog_data->dispatch_16;
      wm._32PixelDispatchEnable = wm_prog_data->dispatch_32;
#endif

#if GFX_VER == 4
      /* On gfx4, we only have one shader kernel */
      if (brw_wm_state_has_ksp(wm, 0)) {
         assert(brw_wm_prog_data_prog_offset(wm_prog_data, wm, 0) == 0);
         wm.KernelStartPointer0 = KSP(brw, stage_state->prog_offset);
         wm.GRFRegisterCount0 = brw_wm_prog_data_reg_blocks(wm_prog_data, wm, 0);
         wm.DispatchGRFStartRegisterForConstantSetupData0 =
            brw_wm_prog_data_dispatch_grf_start_reg(wm_prog_data, wm, 0);
      }
#elif GFX_VER == 5
      /* On gfx5, we have multiple shader kernels but only one GRF start
       * register for all kernels
       */
      wm.KernelStartPointer0 = stage_state->prog_offset +
                               brw_wm_prog_data_prog_offset(wm_prog_data, wm, 0);
      wm.KernelStartPointer1 = stage_state->prog_offset +
                               brw_wm_prog_data_prog_offset(wm_prog_data, wm, 1);
      wm.KernelStartPointer2 = stage_state->prog_offset +
                               brw_wm_prog_data_prog_offset(wm_prog_data, wm, 2);

      wm.GRFRegisterCount0 = brw_wm_prog_data_reg_blocks(wm_prog_data, wm, 0);
      wm.GRFRegisterCount1 = brw_wm_prog_data_reg_blocks(wm_prog_data, wm, 1);
      wm.GRFRegisterCount2 = brw_wm_prog_data_reg_blocks(wm_prog_data, wm, 2);

      wm.DispatchGRFStartRegisterForConstantSetupData0 =
         wm_prog_data->base.dispatch_grf_start_reg;

      /* Dispatch GRF Start should be the same for all shaders on gfx5 */
      if (brw_wm_state_has_ksp(wm, 1)) {
         assert(wm_prog_data->base.dispatch_grf_start_reg ==
                brw_wm_prog_data_dispatch_grf_start_reg(wm_prog_data, wm, 1));
      }
      if (brw_wm_state_has_ksp(wm, 2)) {
         assert(wm_prog_data->base.dispatch_grf_start_reg ==
                brw_wm_prog_data_dispatch_grf_start_reg(wm_prog_data, wm, 2));
      }
#elif GFX_VER == 6
      /* On gfx6, we have multiple shader kernels and we no longer specify a
       * register count for each one.
       */
      wm.KernelStartPointer0 = stage_state->prog_offset +
                               brw_wm_prog_data_prog_offset(wm_prog_data, wm, 0);
      wm.KernelStartPointer1 = stage_state->prog_offset +
                               brw_wm_prog_data_prog_offset(wm_prog_data, wm, 1);
      wm.KernelStartPointer2 = stage_state->prog_offset +
                               brw_wm_prog_data_prog_offset(wm_prog_data, wm, 2);

      wm.DispatchGRFStartRegisterForConstantSetupData0 =
         brw_wm_prog_data_dispatch_grf_start_reg(wm_prog_data, wm, 0);
      wm.DispatchGRFStartRegisterForConstantSetupData1 =
         brw_wm_prog_data_dispatch_grf_start_reg(wm_prog_data, wm, 1);
      wm.DispatchGRFStartRegisterForConstantSetupData2 =
         brw_wm_prog_data_dispatch_grf_start_reg(wm_prog_data, wm, 2);
#endif

#if GFX_VER <= 5
      wm.ConstantURBEntryReadLength = wm_prog_data->base.curb_read_length;
      /* BRW_NEW_PUSH_CONSTANT_ALLOCATION */
      wm.ConstantURBEntryReadOffset = brw->curbe.wm_start * 2;
      wm.SetupURBEntryReadLength = wm_prog_data->num_varying_inputs * 2;
      wm.SetupURBEntryReadOffset = 0;
      wm.EarlyDepthTestEnable = true;
#endif

#if GFX_VER >= 6
      wm.LineAntialiasingRegionWidth = _10pixels;
      wm.LineEndCapAntialiasingRegionWidth = _05pixels;

      wm.PointRasterizationRule = RASTRULE_UPPER_RIGHT;
      wm.BarycentricInterpolationMode = wm_prog_data->barycentric_interp_modes;
#else
      if (stage_state->sampler_count)
         wm.SamplerStatePointer =
            ro_bo(brw->batch.state.bo, stage_state->sampler_offset);

      wm.LineAntialiasingRegionWidth = _05pixels;
      wm.LineEndCapAntialiasingRegionWidth = _10pixels;

      /* _NEW_POLYGON */
      if (ctx->Polygon.OffsetFill) {
         wm.GlobalDepthOffsetEnable = true;
         /* Something weird going on with legacy_global_depth_bias,
          * offset_constant, scaling and MRD.  This value passes glean
          * but gives some odd results elsewere (eg. the
          * quad-offset-units test).
          */
         wm.GlobalDepthOffsetConstant = ctx->Polygon.OffsetUnits * 2;

         /* This is the only value that passes glean:
         */
         wm.GlobalDepthOffsetScale = ctx->Polygon.OffsetFactor;
      }

      wm.DepthCoefficientURBReadOffset = 1;
#endif

      /* BRW_NEW_STATS_WM */
      wm.StatisticsEnable = GFX_VER >= 6 || brw->stats_wm;

#if GFX_VER < 7
      if (wm_prog_data->base.use_alt_mode)
         wm.FloatingPointMode = FLOATING_POINT_MODE_Alternate;

      wm.SamplerCount = GFX_VER == 5 ?
         0 : DIV_ROUND_UP(stage_state->sampler_count, 4);

      wm.BindingTableEntryCount =
         wm_prog_data->base.binding_table.size_bytes / 4;
      wm.MaximumNumberofThreads = devinfo->max_wm_threads - 1;

#if GFX_VER == 6
      wm.DualSourceBlendEnable =
         wm_prog_data->dual_src_blend && (ctx->Color.BlendEnabled & 1) &&
         ctx->Color._BlendUsesDualSrc & 0x1;
      wm.oMaskPresenttoRenderTarget = wm_prog_data->uses_omask;
      wm.NumberofSFOutputAttributes = wm_prog_data->num_varying_inputs;

      /* From the SNB PRM, volume 2 part 1, page 281:
       * "If the PS kernel does not need the Position XY Offsets
       * to compute a Position XY value, then this field should be
       * programmed to POSOFFSET_NONE."
       *
       * "SW Recommendation: If the PS kernel needs the Position Offsets
       * to compute a Position XY value, this field should match Position
       * ZW Interpolation Mode to ensure a consistent position.xyzw
       * computation."
       * We only require XY sample offsets. So, this recommendation doesn't
       * look useful at the moment. We might need this in future.
       */
      if (wm_prog_data->uses_pos_offset)
         wm.PositionXYOffsetSelect = POSOFFSET_SAMPLE;
      else
         wm.PositionXYOffsetSelect = POSOFFSET_NONE;
#endif

      if (wm_prog_data->base.total_scratch) {
         wm.ScratchSpaceBasePointer = rw_32_bo(stage_state->scratch_bo, 0);
         wm.PerThreadScratchSpace =
            ffs(stage_state->per_thread_scratch) - 11;
      }

      wm.PixelShaderComputedDepth = writes_depth;
#endif

      /* _NEW_LINE */
      wm.LineStippleEnable = ctx->Line.StippleFlag;

      /* _NEW_POLYGON */
      wm.PolygonStippleEnable = ctx->Polygon.StippleFlag;

#if GFX_VER < 8

#if GFX_VER >= 6
      wm.PixelShaderUsesSourceW = wm_prog_data->uses_src_w;

      /* _NEW_BUFFERS */
      const bool multisampled_fbo = _mesa_geometric_samples(ctx->DrawBuffer) > 1;

      if (multisampled_fbo) {
         /* _NEW_MULTISAMPLE */
         if (ctx->Multisample.Enabled)
            wm.MultisampleRasterizationMode = MSRASTMODE_ON_PATTERN;
         else
            wm.MultisampleRasterizationMode = MSRASTMODE_OFF_PIXEL;

         if (wm_prog_data->persample_dispatch)
            wm.MultisampleDispatchMode = MSDISPMODE_PERSAMPLE;
         else
            wm.MultisampleDispatchMode = MSDISPMODE_PERPIXEL;
      } else {
         wm.MultisampleRasterizationMode = MSRASTMODE_OFF_PIXEL;
         wm.MultisampleDispatchMode = MSDISPMODE_PERSAMPLE;
      }
#endif
      wm.PixelShaderUsesSourceDepth = wm_prog_data->uses_src_depth;
      if (wm_prog_data->uses_kill ||
          _mesa_is_alpha_test_enabled(ctx) ||
          _mesa_is_alpha_to_coverage_enabled(ctx) ||
          (GFX_VER >= 6 && wm_prog_data->uses_omask)) {
         wm.PixelShaderKillsPixel = true;
      }

      /* _NEW_BUFFERS | _NEW_COLOR */
      if (brw_color_buffer_write_enabled(brw) || writes_depth ||
          wm.PixelShaderKillsPixel ||
          (GFX_VER >= 6 && wm_prog_data->has_side_effects)) {
         wm.ThreadDispatchEnable = true;
      }

#if GFX_VER >= 7
      wm.PixelShaderComputedDepthMode = wm_prog_data->computed_depth_mode;
      wm.PixelShaderUsesInputCoverageMask = wm_prog_data->uses_sample_mask;
#endif

      /* The "UAV access enable" bits are unnecessary on HSW because they only
       * seem to have an effect on the HW-assisted coherency mechanism which we
       * don't need, and the rasterization-related UAV_ONLY flag and the
       * DISPATCH_ENABLE bit can be set independently from it.
       * C.f. gfx8_upload_ps_extra().
       *
       * BRW_NEW_FRAGMENT_PROGRAM | BRW_NEW_FS_PROG_DATA | _NEW_BUFFERS |
       * _NEW_COLOR
       */
#if GFX_VERx10 == 75
      if (!(brw_color_buffer_write_enabled(brw) || writes_depth) &&
          wm_prog_data->has_side_effects)
         wm.PSUAVonly = ON;
#endif
#endif

#if GFX_VER >= 7
      /* BRW_NEW_FS_PROG_DATA */
      if (wm_prog_data->early_fragment_tests)
         wm.EarlyDepthStencilControl = EDSC_PREPS;
      else if (wm_prog_data->has_side_effects)
         wm.EarlyDepthStencilControl = EDSC_PSEXEC;
#endif
   }

#if GFX_VER <= 5
   if (brw->wm.offset_clamp != ctx->Polygon.OffsetClamp) {
      brw_batch_emit(brw, GENX(3DSTATE_GLOBAL_DEPTH_OFFSET_CLAMP), clamp) {
         clamp.GlobalDepthOffsetClamp = ctx->Polygon.OffsetClamp;
      }

      brw->wm.offset_clamp = ctx->Polygon.OffsetClamp;
   }
#endif
}

static const struct brw_tracked_state genX(wm_state) = {
   .dirty = {
      .mesa  = _NEW_LINE |
               _NEW_POLYGON |
               (GFX_VER < 8 ? _NEW_BUFFERS |
                              _NEW_COLOR :
                              0) |
               (GFX_VER == 6 ? _NEW_PROGRAM_CONSTANTS : 0) |
               (GFX_VER < 6 ? _NEW_POLYGONSTIPPLE : 0) |
               (GFX_VER < 8 && GFX_VER >= 6 ? _NEW_MULTISAMPLE : 0),
      .brw   = BRW_NEW_BLORP |
               BRW_NEW_FS_PROG_DATA |
               (GFX_VER < 6 ? BRW_NEW_PUSH_CONSTANT_ALLOCATION |
                              BRW_NEW_FRAGMENT_PROGRAM |
                              BRW_NEW_PROGRAM_CACHE |
                              BRW_NEW_SAMPLER_STATE_TABLE |
                              BRW_NEW_STATS_WM
                            : 0) |
               (GFX_VER < 7 ? BRW_NEW_BATCH : BRW_NEW_CONTEXT),
   },
   .emit = genX(upload_wm),
};

/* ---------------------------------------------------------------------- */

/* We restrict scratch buffers to the bottom 32 bits of the address space
 * by using rw_32_bo().
 *
 * General State Base Address is a bit broken.  If the address + size as
 * seen by STATE_BASE_ADDRESS overflows 48 bits, the GPU appears to treat
 * all accesses to the buffer as being out of bounds and returns zero.
 */

#define INIT_THREAD_DISPATCH_FIELDS(pkt, prefix) \
   pkt.KernelStartPointer = KSP(brw, stage_state->prog_offset);           \
   /* Wa_1606682166 */                                                    \
   pkt.SamplerCount       =                                               \
      GFX_VER == 11 ?                                                     \
      0 :                                                                 \
      DIV_ROUND_UP(CLAMP(stage_state->sampler_count, 0, 16), 4);          \
   pkt.BindingTableEntryCount =                                           \
      stage_prog_data->binding_table.size_bytes / 4;                      \
   pkt.FloatingPointMode  = stage_prog_data->use_alt_mode;                \
                                                                          \
   if (stage_prog_data->total_scratch) {                                  \
      pkt.ScratchSpaceBasePointer = rw_32_bo(stage_state->scratch_bo, 0); \
      pkt.PerThreadScratchSpace =                                         \
         ffs(stage_state->per_thread_scratch) - 11;                       \
   }                                                                      \
                                                                          \
   pkt.DispatchGRFStartRegisterForURBData =                               \
      stage_prog_data->dispatch_grf_start_reg;                            \
   pkt.prefix##URBEntryReadLength = vue_prog_data->urb_read_length;       \
   pkt.prefix##URBEntryReadOffset = 0;                                    \
                                                                          \
   pkt.StatisticsEnable = true;                                           \
   pkt.Enable           = true;

static void
genX(upload_vs_state)(struct brw_context *brw)
{
   UNUSED struct gl_context *ctx = &brw->ctx;
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct brw_stage_state *stage_state = &brw->vs.base;

   /* BRW_NEW_VS_PROG_DATA */
   const struct brw_vue_prog_data *vue_prog_data =
      brw_vue_prog_data(brw->vs.base.prog_data);
   const struct brw_stage_prog_data *stage_prog_data = &vue_prog_data->base;

   assert(vue_prog_data->dispatch_mode == DISPATCH_MODE_SIMD8 ||
          vue_prog_data->dispatch_mode == DISPATCH_MODE_4X2_DUAL_OBJECT);
   assert(GFX_VER < 11 ||
          vue_prog_data->dispatch_mode == DISPATCH_MODE_SIMD8);

#if GFX_VER == 6
   /* From the BSpec, 3D Pipeline > Geometry > Vertex Shader > State,
    * 3DSTATE_VS, Dword 5.0 "VS Function Enable":
    *
    *   [DevSNB] A pipeline flush must be programmed prior to a 3DSTATE_VS
    *   command that causes the VS Function Enable to toggle. Pipeline
    *   flush can be executed by sending a PIPE_CONTROL command with CS
    *   stall bit set and a post sync operation.
    *
    * We've already done such a flush at the start of state upload, so we
    * don't need to do another one here.
    */
   brw_batch_emit(brw, GENX(3DSTATE_CONSTANT_VS), cvs) {
      if (stage_state->push_const_size != 0) {
         cvs.Buffer0Valid = true;
         cvs.ConstantBody.PointertoConstantBuffer0 = stage_state->push_const_offset;
         cvs.ConstantBody.ConstantBuffer0ReadLength = stage_state->push_const_size - 1;
      }
   }
#endif

   if (GFX_VER == 7 && devinfo->is_ivybridge)
      gfx7_emit_vs_workaround_flush(brw);

#if GFX_VER >= 6
   brw_batch_emit(brw, GENX(3DSTATE_VS), vs) {
#else
   ctx->NewDriverState |= BRW_NEW_GFX4_UNIT_STATE;
   brw_state_emit(brw, GENX(VS_STATE), 32, &stage_state->state_offset, vs) {
#endif
      INIT_THREAD_DISPATCH_FIELDS(vs, Vertex);

      vs.MaximumNumberofThreads = devinfo->max_vs_threads - 1;

#if GFX_VER < 6
      vs.GRFRegisterCount = DIV_ROUND_UP(vue_prog_data->total_grf, 16) - 1;
      vs.ConstantURBEntryReadLength = stage_prog_data->curb_read_length;
      vs.ConstantURBEntryReadOffset = brw->curbe.vs_start * 2;

      vs.NumberofURBEntries = brw->urb.nr_vs_entries >> (GFX_VER == 5 ? 2 : 0);
      vs.URBEntryAllocationSize = brw->urb.vsize - 1;

      vs.MaximumNumberofThreads =
         CLAMP(brw->urb.nr_vs_entries / 2, 1, devinfo->max_vs_threads) - 1;

      vs.StatisticsEnable = false;
      vs.SamplerStatePointer =
         ro_bo(brw->batch.state.bo, stage_state->sampler_offset);
#endif

#if GFX_VER == 5
      /* Force single program flow on Ironlake.  We cannot reliably get
       * all applications working without it.  See:
       * https://bugs.freedesktop.org/show_bug.cgi?id=29172
       *
       * The most notable and reliably failing application is the Humus
       * demo "CelShading"
       */
      vs.SingleProgramFlow = true;
      vs.SamplerCount = 0; /* hardware requirement */
#endif

#if GFX_VER >= 8
      vs.SIMD8DispatchEnable =
         vue_prog_data->dispatch_mode == DISPATCH_MODE_SIMD8;

      vs.UserClipDistanceCullTestEnableBitmask =
         vue_prog_data->cull_distance_mask;
#endif
   }

#if GFX_VER == 6
   /* Based on my reading of the simulator, the VS constants don't get
    * pulled into the VS FF unit until an appropriate pipeline flush
    * happens, and instead the 3DSTATE_CONSTANT_VS packet just adds
    * references to them into a little FIFO.  The flushes are common,
    * but don't reliably happen between this and a 3DPRIMITIVE, causing
    * the primitive to use the wrong constants.  Then the FIFO
    * containing the constant setup gets added to again on the next
    * constants change, and eventually when a flush does happen the
    * unit is overwhelmed by constant changes and dies.
    *
    * To avoid this, send a PIPE_CONTROL down the line that will
    * update the unit immediately loading the constants.  The flush
    * type bits here were those set by the STATE_BASE_ADDRESS whose
    * move in a82a43e8d99e1715dd11c9c091b5ab734079b6a6 triggered the
    * bug reports that led to this workaround, and may be more than
    * what is strictly required to avoid the issue.
    */
   brw_emit_pipe_control_flush(brw,
                               PIPE_CONTROL_DEPTH_STALL |
                               PIPE_CONTROL_INSTRUCTION_INVALIDATE |
                               PIPE_CONTROL_STATE_CACHE_INVALIDATE);
#endif
}

static const struct brw_tracked_state genX(vs_state) = {
   .dirty = {
      .mesa  = (GFX_VER == 6 ? (_NEW_PROGRAM_CONSTANTS | _NEW_TRANSFORM) : 0),
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               BRW_NEW_CONTEXT |
               BRW_NEW_VS_PROG_DATA |
               (GFX_VER == 6 ? BRW_NEW_VERTEX_PROGRAM : 0) |
               (GFX_VER <= 5 ? BRW_NEW_PUSH_CONSTANT_ALLOCATION |
                               BRW_NEW_PROGRAM_CACHE |
                               BRW_NEW_SAMPLER_STATE_TABLE |
                               BRW_NEW_URB_FENCE
                             : 0),
   },
   .emit = genX(upload_vs_state),
};

/* ---------------------------------------------------------------------- */

static void
genX(upload_cc_viewport)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   /* BRW_NEW_VIEWPORT_COUNT */
   const unsigned viewport_count = brw->clip.viewport_count;

   struct GENX(CC_VIEWPORT) ccv;
   uint32_t cc_vp_offset;
   uint32_t *cc_map =
      brw_state_batch(brw, 4 * GENX(CC_VIEWPORT_length) * viewport_count,
                      32, &cc_vp_offset);

   for (unsigned i = 0; i < viewport_count; i++) {
      /* _NEW_VIEWPORT | _NEW_TRANSFORM */
      const struct gl_viewport_attrib *vp = &ctx->ViewportArray[i];
      if (ctx->Transform.DepthClampNear && ctx->Transform.DepthClampFar) {
         ccv.MinimumDepth = MIN2(vp->Near, vp->Far);
         ccv.MaximumDepth = MAX2(vp->Near, vp->Far);
      } else if (ctx->Transform.DepthClampNear) {
         ccv.MinimumDepth = MIN2(vp->Near, vp->Far);
         ccv.MaximumDepth = 0.0;
      } else if (ctx->Transform.DepthClampFar) {
         ccv.MinimumDepth = 0.0;
         ccv.MaximumDepth = MAX2(vp->Near, vp->Far);
      } else {
         ccv.MinimumDepth = 0.0;
         ccv.MaximumDepth = 1.0;
      }
      GENX(CC_VIEWPORT_pack)(NULL, cc_map, &ccv);
      cc_map += GENX(CC_VIEWPORT_length);
   }

#if GFX_VER >= 7
   brw_batch_emit(brw, GENX(3DSTATE_VIEWPORT_STATE_POINTERS_CC), ptr) {
      ptr.CCViewportPointer = cc_vp_offset;
   }
#elif GFX_VER == 6
   brw_batch_emit(brw, GENX(3DSTATE_VIEWPORT_STATE_POINTERS), vp) {
      vp.CCViewportStateChange = 1;
      vp.PointertoCC_VIEWPORT = cc_vp_offset;
   }
#else
   brw->cc.vp_offset = cc_vp_offset;
   ctx->NewDriverState |= BRW_NEW_CC_VP;
#endif
}

const struct brw_tracked_state genX(cc_vp) = {
   .dirty = {
      .mesa = _NEW_TRANSFORM |
              _NEW_VIEWPORT,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_VIEWPORT_COUNT,
   },
   .emit = genX(upload_cc_viewport)
};

/* ---------------------------------------------------------------------- */

static void
set_scissor_bits(const struct gl_context *ctx, int i,
                 bool flip_y, unsigned fb_width, unsigned fb_height,
                 struct GENX(SCISSOR_RECT) *sc)
{
   int bbox[4];

   bbox[0] = MAX2(ctx->ViewportArray[i].X, 0);
   bbox[1] = MIN2(bbox[0] + ctx->ViewportArray[i].Width, fb_width);
   bbox[2] = CLAMP(ctx->ViewportArray[i].Y, 0, fb_height);
   bbox[3] = MIN2(bbox[2] + ctx->ViewportArray[i].Height, fb_height);
   _mesa_intersect_scissor_bounding_box(ctx, i, bbox);

   if (bbox[0] == bbox[1] || bbox[2] == bbox[3]) {
      /* If the scissor was out of bounds and got clamped to 0 width/height
       * at the bounds, the subtraction of 1 from maximums could produce a
       * negative number and thus not clip anything.  Instead, just provide
       * a min > max scissor inside the bounds, which produces the expected
       * no rendering.
       */
      sc->ScissorRectangleXMin = 1;
      sc->ScissorRectangleXMax = 0;
      sc->ScissorRectangleYMin = 1;
      sc->ScissorRectangleYMax = 0;
   } else if (!flip_y) {
      /* texmemory: Y=0=bottom */
      sc->ScissorRectangleXMin = bbox[0];
      sc->ScissorRectangleXMax = bbox[1] - 1;
      sc->ScissorRectangleYMin = bbox[2];
      sc->ScissorRectangleYMax = bbox[3] - 1;
   } else {
      /* memory: Y=0=top */
      sc->ScissorRectangleXMin = bbox[0];
      sc->ScissorRectangleXMax = bbox[1] - 1;
      sc->ScissorRectangleYMin = fb_height - bbox[3];
      sc->ScissorRectangleYMax = fb_height - bbox[2] - 1;
   }
}

#if GFX_VER >= 6
static void
genX(upload_scissor_state)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   const bool flip_y = ctx->DrawBuffer->FlipY;
   struct GENX(SCISSOR_RECT) scissor;
   uint32_t scissor_state_offset;
   const unsigned int fb_width = _mesa_geometric_width(ctx->DrawBuffer);
   const unsigned int fb_height = _mesa_geometric_height(ctx->DrawBuffer);
   uint32_t *scissor_map;

   /* BRW_NEW_VIEWPORT_COUNT */
   const unsigned viewport_count = brw->clip.viewport_count;
   /* Wa_1409725701:
    *    "The viewport-specific state used by the SF unit (SCISSOR_RECT) is
    *    stored as an array of up to 16 elements. The location of first
    *    element of the array, as specified by Pointer to SCISSOR_RECT, should
    *    be aligned to a 64-byte boundary.
    */
   const unsigned alignment = 64;
   scissor_map = brw_state_batch(
      brw, GENX(SCISSOR_RECT_length) * sizeof(uint32_t) * viewport_count,
      alignment, &scissor_state_offset);

   /* _NEW_SCISSOR | _NEW_BUFFERS | _NEW_VIEWPORT */

   /* The scissor only needs to handle the intersection of drawable and
    * scissor rect.  Clipping to the boundaries of static shared buffers
    * for front/back/depth is covered by looping over cliprects in brw_draw.c.
    *
    * Note that the hardware's coordinates are inclusive, while Mesa's min is
    * inclusive but max is exclusive.
    */
   for (unsigned i = 0; i < viewport_count; i++) {
      set_scissor_bits(ctx, i, flip_y, fb_width, fb_height, &scissor);
      GENX(SCISSOR_RECT_pack)(
         NULL, scissor_map + i * GENX(SCISSOR_RECT_length), &scissor);
   }

   brw_batch_emit(brw, GENX(3DSTATE_SCISSOR_STATE_POINTERS), ptr) {
      ptr.ScissorRectPointer = scissor_state_offset;
   }
}

static const struct brw_tracked_state genX(scissor_state) = {
   .dirty = {
      .mesa = _NEW_BUFFERS |
              _NEW_SCISSOR |
              _NEW_VIEWPORT,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_VIEWPORT_COUNT,
   },
   .emit = genX(upload_scissor_state),
};
#endif

/* ---------------------------------------------------------------------- */

static void
genX(upload_sf_clip_viewport)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   float y_scale, y_bias;

   /* BRW_NEW_VIEWPORT_COUNT */
   const unsigned viewport_count = brw->clip.viewport_count;

   /* _NEW_BUFFERS */
   const bool flip_y = ctx->DrawBuffer->FlipY;
   const uint32_t fb_width = (float)_mesa_geometric_width(ctx->DrawBuffer);
   const uint32_t fb_height = (float)_mesa_geometric_height(ctx->DrawBuffer);

#if GFX_VER >= 7
#define clv sfv
   struct GENX(SF_CLIP_VIEWPORT) sfv;
   uint32_t sf_clip_vp_offset;
   uint32_t *sf_clip_map =
      brw_state_batch(brw, GENX(SF_CLIP_VIEWPORT_length) * 4 * viewport_count,
                      64, &sf_clip_vp_offset);
#else
   struct GENX(SF_VIEWPORT) sfv;
   struct GENX(CLIP_VIEWPORT) clv;
   uint32_t sf_vp_offset, clip_vp_offset;
   uint32_t *sf_map =
      brw_state_batch(brw, GENX(SF_VIEWPORT_length) * 4 * viewport_count,
                      32, &sf_vp_offset);
   uint32_t *clip_map =
      brw_state_batch(brw, GENX(CLIP_VIEWPORT_length) * 4 * viewport_count,
                      32, &clip_vp_offset);
#endif

   /* _NEW_BUFFERS */
   if (flip_y) {
      y_scale = -1.0;
      y_bias = (float)fb_height;
   } else {
      y_scale = 1.0;
      y_bias = 0;
   }

   for (unsigned i = 0; i < brw->clip.viewport_count; i++) {
      /* _NEW_VIEWPORT: Guardband Clipping */
      float scale[3], translate[3], gb_xmin, gb_xmax, gb_ymin, gb_ymax;
      _mesa_get_viewport_xform(ctx, i, scale, translate);

      sfv.ViewportMatrixElementm00 = scale[0];
      sfv.ViewportMatrixElementm11 = scale[1] * y_scale,
      sfv.ViewportMatrixElementm22 = scale[2],
      sfv.ViewportMatrixElementm30 = translate[0],
      sfv.ViewportMatrixElementm31 = translate[1] * y_scale + y_bias,
      sfv.ViewportMatrixElementm32 = translate[2],
      intel_calculate_guardband_size(fb_width, fb_height,
                                     sfv.ViewportMatrixElementm00,
                                     sfv.ViewportMatrixElementm11,
                                     sfv.ViewportMatrixElementm30,
                                     sfv.ViewportMatrixElementm31,
                                     &gb_xmin, &gb_xmax, &gb_ymin, &gb_ymax);


      clv.XMinClipGuardband = gb_xmin;
      clv.XMaxClipGuardband = gb_xmax;
      clv.YMinClipGuardband = gb_ymin;
      clv.YMaxClipGuardband = gb_ymax;

#if GFX_VER < 6
      set_scissor_bits(ctx, i, flip_y, fb_width, fb_height,
                       &sfv.ScissorRectangle);
#elif GFX_VER >= 8
      /* _NEW_VIEWPORT | _NEW_BUFFERS: Screen Space Viewport
       * The hardware will take the intersection of the drawing rectangle,
       * scissor rectangle, and the viewport extents.  However, emitting
       * 3DSTATE_DRAWING_RECTANGLE is expensive since it requires a full
       * pipeline stall so we're better off just being a little more clever
       * with our viewport so we can emit it once at context creation time.
       */
      const float viewport_Xmin = MAX2(ctx->ViewportArray[i].X, 0);
      const float viewport_Ymin = MAX2(ctx->ViewportArray[i].Y, 0);
      const float viewport_Xmax =
         MIN2(ctx->ViewportArray[i].X + ctx->ViewportArray[i].Width, fb_width);
      const float viewport_Ymax =
         MIN2(ctx->ViewportArray[i].Y + ctx->ViewportArray[i].Height, fb_height);

      if (flip_y) {
         sfv.XMinViewPort = viewport_Xmin;
         sfv.XMaxViewPort = viewport_Xmax - 1;
         sfv.YMinViewPort = fb_height - viewport_Ymax;
         sfv.YMaxViewPort = fb_height - viewport_Ymin - 1;
      } else {
         sfv.XMinViewPort = viewport_Xmin;
         sfv.XMaxViewPort = viewport_Xmax - 1;
         sfv.YMinViewPort = viewport_Ymin;
         sfv.YMaxViewPort = viewport_Ymax - 1;
      }
#endif

#if GFX_VER >= 7
      GENX(SF_CLIP_VIEWPORT_pack)(NULL, sf_clip_map, &sfv);
      sf_clip_map += GENX(SF_CLIP_VIEWPORT_length);
#else
      GENX(SF_VIEWPORT_pack)(NULL, sf_map, &sfv);
      GENX(CLIP_VIEWPORT_pack)(NULL, clip_map, &clv);
      sf_map += GENX(SF_VIEWPORT_length);
      clip_map += GENX(CLIP_VIEWPORT_length);
#endif
   }

#if GFX_VER >= 7
   brw_batch_emit(brw, GENX(3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP), ptr) {
      ptr.SFClipViewportPointer = sf_clip_vp_offset;
   }
#elif GFX_VER == 6
   brw_batch_emit(brw, GENX(3DSTATE_VIEWPORT_STATE_POINTERS), vp) {
      vp.SFViewportStateChange = 1;
      vp.CLIPViewportStateChange = 1;
      vp.PointertoCLIP_VIEWPORT = clip_vp_offset;
      vp.PointertoSF_VIEWPORT = sf_vp_offset;
   }
#else
   brw->sf.vp_offset = sf_vp_offset;
   brw->clip.vp_offset = clip_vp_offset;
   brw->ctx.NewDriverState |= BRW_NEW_SF_VP | BRW_NEW_CLIP_VP;
#endif
}

static const struct brw_tracked_state genX(sf_clip_viewport) = {
   .dirty = {
      .mesa = _NEW_BUFFERS |
              _NEW_VIEWPORT |
              (GFX_VER <= 5 ? _NEW_SCISSOR : 0),
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_VIEWPORT_COUNT,
   },
   .emit = genX(upload_sf_clip_viewport),
};

/* ---------------------------------------------------------------------- */

static void
genX(upload_gs_state)(struct brw_context *brw)
{
   UNUSED struct gl_context *ctx = &brw->ctx;
   UNUSED const struct intel_device_info *devinfo = &brw->screen->devinfo;
   const struct brw_stage_state *stage_state = &brw->gs.base;
   const struct gl_program *gs_prog = brw->programs[MESA_SHADER_GEOMETRY];
   /* BRW_NEW_GEOMETRY_PROGRAM */
   bool active = GFX_VER >= 6 && gs_prog;

   /* BRW_NEW_GS_PROG_DATA */
   struct brw_stage_prog_data *stage_prog_data = stage_state->prog_data;
   UNUSED const struct brw_vue_prog_data *vue_prog_data =
      brw_vue_prog_data(stage_prog_data);
#if GFX_VER >= 7
   const struct brw_gs_prog_data *gs_prog_data =
      brw_gs_prog_data(stage_prog_data);
#endif

#if GFX_VER == 6
   brw_batch_emit(brw, GENX(3DSTATE_CONSTANT_GS), cgs) {
      if (active && stage_state->push_const_size != 0) {
         cgs.Buffer0Valid = true;
         cgs.ConstantBody.PointertoConstantBuffer0 = stage_state->push_const_offset;
         cgs.ConstantBody.ConstantBuffer0ReadLength = stage_state->push_const_size - 1;
      }
   }
#endif

#if GFX_VERx10 == 70
   /**
    * From Graphics BSpec: 3D-Media-GPGPU Engine > 3D Pipeline Stages >
    * Geometry > Geometry Shader > State:
    *
    *     "Note: Because of corruption in IVB:GT2, software needs to flush the
    *     whole fixed function pipeline when the GS enable changes value in
    *     the 3DSTATE_GS."
    *
    * The hardware architects have clarified that in this context "flush the
    * whole fixed function pipeline" means to emit a PIPE_CONTROL with the "CS
    * Stall" bit set.
    */
   if (devinfo->gt == 2 && brw->gs.enabled != active)
      gfx7_emit_cs_stall_flush(brw);
#endif

#if GFX_VER >= 6
   brw_batch_emit(brw, GENX(3DSTATE_GS), gs) {
#else
   ctx->NewDriverState |= BRW_NEW_GFX4_UNIT_STATE;
   brw_state_emit(brw, GENX(GS_STATE), 32, &brw->ff_gs.state_offset, gs) {
#endif

#if GFX_VER >= 6
      if (active) {
         INIT_THREAD_DISPATCH_FIELDS(gs, Vertex);

#if GFX_VER >= 7
         gs.OutputVertexSize = gs_prog_data->output_vertex_size_hwords * 2 - 1;
         gs.OutputTopology = gs_prog_data->output_topology;
         gs.ControlDataHeaderSize =
            gs_prog_data->control_data_header_size_hwords;

         gs.InstanceControl = gs_prog_data->invocations - 1;
         gs.DispatchMode = vue_prog_data->dispatch_mode;

         gs.IncludePrimitiveID = gs_prog_data->include_primitive_id;

         gs.ControlDataFormat = gs_prog_data->control_data_format;
#endif

         /* Note: the meaning of the GFX7_GS_REORDER_TRAILING bit changes between
          * Ivy Bridge and Haswell.
          *
          * On Ivy Bridge, setting this bit causes the vertices of a triangle
          * strip to be delivered to the geometry shader in an order that does
          * not strictly follow the OpenGL spec, but preserves triangle
          * orientation.  For example, if the vertices are (1, 2, 3, 4, 5), then
          * the geometry shader sees triangles:
          *
          * (1, 2, 3), (2, 4, 3), (3, 4, 5)
          *
          * (Clearing the bit is even worse, because it fails to preserve
          * orientation).
          *
          * Triangle strips with adjacency always ordered in a way that preserves
          * triangle orientation but does not strictly follow the OpenGL spec,
          * regardless of the setting of this bit.
          *
          * On Haswell, both triangle strips and triangle strips with adjacency
          * are always ordered in a way that preserves triangle orientation.
          * Setting this bit causes the ordering to strictly follow the OpenGL
          * spec.
          *
          * So in either case we want to set the bit.  Unfortunately on Ivy
          * Bridge this will get the order close to correct but not perfect.
          */
         gs.ReorderMode = TRAILING;
         gs.MaximumNumberofThreads =
            GFX_VER == 8 ? (devinfo->max_gs_threads / 2 - 1)
                         : (devinfo->max_gs_threads - 1);

#if GFX_VER < 7
         gs.SOStatisticsEnable = true;
         if (gs_prog->info.has_transform_feedback_varyings)
            gs.SVBIPayloadEnable = _mesa_is_xfb_active_and_unpaused(ctx);

         /* GFX6_GS_SPF_MODE and GFX6_GS_VECTOR_MASK_ENABLE are enabled as it
          * was previously done for gfx6.
          *
          * TODO: test with both disabled to see if the HW is behaving
          * as expected, like in gfx7.
          */
         gs.SingleProgramFlow = true;
         gs.VectorMaskEnable = true;
#endif

#if GFX_VER >= 8
         gs.ExpectedVertexCount = gs_prog_data->vertices_in;

         if (gs_prog_data->static_vertex_count != -1) {
            gs.StaticOutput = true;
            gs.StaticOutputVertexCount = gs_prog_data->static_vertex_count;
         }
         gs.IncludeVertexHandles = vue_prog_data->include_vue_handles;

         gs.UserClipDistanceCullTestEnableBitmask =
            vue_prog_data->cull_distance_mask;

         const int urb_entry_write_offset = 1;
         const uint32_t urb_entry_output_length =
            DIV_ROUND_UP(vue_prog_data->vue_map.num_slots, 2) -
            urb_entry_write_offset;

         gs.VertexURBEntryOutputReadOffset = urb_entry_write_offset;
         gs.VertexURBEntryOutputLength = MAX2(urb_entry_output_length, 1);
#endif
      }
#endif

#if GFX_VER <= 6
      if (!active && brw->ff_gs.prog_active) {
         /* In gfx6, transform feedback for the VS stage is done with an
          * ad-hoc GS program. This function provides the needed 3DSTATE_GS
          * for this.
          */
         gs.KernelStartPointer = KSP(brw, brw->ff_gs.prog_offset);
         gs.SingleProgramFlow = true;
         gs.DispatchGRFStartRegisterForURBData = GFX_VER == 6 ? 2 : 1;
         gs.VertexURBEntryReadLength = brw->ff_gs.prog_data->urb_read_length;

#if GFX_VER <= 5
         gs.GRFRegisterCount =
            DIV_ROUND_UP(brw->ff_gs.prog_data->total_grf, 16) - 1;
         /* BRW_NEW_URB_FENCE */
         gs.NumberofURBEntries = brw->urb.nr_gs_entries;
         gs.URBEntryAllocationSize = brw->urb.vsize - 1;
         gs.MaximumNumberofThreads = brw->urb.nr_gs_entries >= 8 ? 1 : 0;
         gs.FloatingPointMode = FLOATING_POINT_MODE_Alternate;
#else
         gs.Enable = true;
         gs.VectorMaskEnable = true;
         gs.SVBIPayloadEnable = true;
         gs.SVBIPostIncrementEnable = true;
         gs.SVBIPostIncrementValue =
            brw->ff_gs.prog_data->svbi_postincrement_value;
         gs.SOStatisticsEnable = true;
         gs.MaximumNumberofThreads = devinfo->max_gs_threads - 1;
#endif
      }
#endif
      if (!active && !brw->ff_gs.prog_active) {
#if GFX_VER < 8
         gs.DispatchGRFStartRegisterForURBData = 1;
#if GFX_VER >= 7
         gs.IncludeVertexHandles = true;
#endif
#endif
      }

#if GFX_VER >= 6
      gs.StatisticsEnable = true;
#endif
#if GFX_VER == 5 || GFX_VER == 6
      gs.RenderingEnabled = true;
#endif
#if GFX_VER <= 5
      gs.MaximumVPIndex = brw->clip.viewport_count - 1;
#endif
   }

#if GFX_VER == 6
   brw->gs.enabled = active;
#endif
}

static const struct brw_tracked_state genX(gs_state) = {
   .dirty = {
      .mesa  = (GFX_VER == 6 ? _NEW_PROGRAM_CONSTANTS : 0),
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               (GFX_VER <= 5 ? BRW_NEW_PUSH_CONSTANT_ALLOCATION |
                               BRW_NEW_PROGRAM_CACHE |
                               BRW_NEW_URB_FENCE |
                               BRW_NEW_VIEWPORT_COUNT
                             : 0) |
               (GFX_VER >= 6 ? BRW_NEW_CONTEXT |
                               BRW_NEW_GEOMETRY_PROGRAM |
                               BRW_NEW_GS_PROG_DATA
                             : 0) |
               (GFX_VER < 7 ? BRW_NEW_FF_GS_PROG_DATA : 0),
   },
   .emit = genX(upload_gs_state),
};

/* ---------------------------------------------------------------------- */

UNUSED static GLenum
fix_dual_blend_alpha_to_one(GLenum function)
{
   switch (function) {
   case GL_SRC1_ALPHA:
      return GL_ONE;

   case GL_ONE_MINUS_SRC1_ALPHA:
      return GL_ZERO;
   }

   return function;
}

#define blend_factor(x) brw_translate_blend_factor(x)
#define blend_eqn(x) brw_translate_blend_equation(x)

/**
 * Modify blend function to force destination alpha to 1.0
 *
 * If \c function specifies a blend function that uses destination alpha,
 * replace it with a function that hard-wires destination alpha to 1.0.  This
 * is used when rendering to xRGB targets.
 */
static GLenum
brw_fix_xRGB_alpha(GLenum function)
{
   switch (function) {
   case GL_DST_ALPHA:
      return GL_ONE;

   case GL_ONE_MINUS_DST_ALPHA:
   case GL_SRC_ALPHA_SATURATE:
      return GL_ZERO;
   }

   return function;
}

#if GFX_VER >= 6
typedef struct GENX(BLEND_STATE_ENTRY) BLEND_ENTRY_GENXML;
#else
typedef struct GENX(COLOR_CALC_STATE) BLEND_ENTRY_GENXML;
#endif

UNUSED static bool
set_blend_entry_bits(struct brw_context *brw, BLEND_ENTRY_GENXML *entry, int i,
                     bool alpha_to_one)
{
   struct gl_context *ctx = &brw->ctx;

   /* _NEW_BUFFERS */
   const struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[i];

   bool independent_alpha_blend = false;

   /* Used for implementing the following bit of GL_EXT_texture_integer:
    * "Per-fragment operations that require floating-point color
    *  components, including multisample alpha operations, alpha test,
    *  blending, and dithering, have no effect when the corresponding
    *  colors are written to an integer color buffer."
    */
   const bool integer = ctx->DrawBuffer->_IntegerBuffers & (0x1 << i);

   const unsigned blend_enabled = GFX_VER >= 6 ?
      ctx->Color.BlendEnabled & (1 << i) : ctx->Color.BlendEnabled;

   /* _NEW_COLOR */
   if (ctx->Color.ColorLogicOpEnabled) {
      GLenum rb_type = rb ? _mesa_get_format_datatype(rb->Format)
         : GL_UNSIGNED_NORMALIZED;
      WARN_ONCE(ctx->Color.LogicOp != GL_COPY &&
                rb_type != GL_UNSIGNED_NORMALIZED &&
                rb_type != GL_FLOAT, "Ignoring %s logic op on %s "
                "renderbuffer\n",
                _mesa_enum_to_string(ctx->Color.LogicOp),
                _mesa_enum_to_string(rb_type));
      if (GFX_VER >= 8 || rb_type == GL_UNSIGNED_NORMALIZED) {
         entry->LogicOpEnable = true;
         entry->LogicOpFunction = ctx->Color._LogicOp;
      }
   } else if (blend_enabled &&
              ctx->Color._AdvancedBlendMode == BLEND_NONE
              && (GFX_VER <= 5 || !integer)) {
      GLenum eqRGB = ctx->Color.Blend[i].EquationRGB;
      GLenum eqA = ctx->Color.Blend[i].EquationA;
      GLenum srcRGB = ctx->Color.Blend[i].SrcRGB;
      GLenum dstRGB = ctx->Color.Blend[i].DstRGB;
      GLenum srcA = ctx->Color.Blend[i].SrcA;
      GLenum dstA = ctx->Color.Blend[i].DstA;

      if (eqRGB == GL_MIN || eqRGB == GL_MAX)
         srcRGB = dstRGB = GL_ONE;

      if (eqA == GL_MIN || eqA == GL_MAX)
         srcA = dstA = GL_ONE;

      /* Due to hardware limitations, the destination may have information
       * in an alpha channel even when the format specifies no alpha
       * channel. In order to avoid getting any incorrect blending due to
       * that alpha channel, coerce the blend factors to values that will
       * not read the alpha channel, but will instead use the correct
       * implicit value for alpha.
       */
      if (rb && !_mesa_base_format_has_channel(rb->_BaseFormat,
                                               GL_TEXTURE_ALPHA_TYPE)) {
         srcRGB = brw_fix_xRGB_alpha(srcRGB);
         srcA = brw_fix_xRGB_alpha(srcA);
         dstRGB = brw_fix_xRGB_alpha(dstRGB);
         dstA = brw_fix_xRGB_alpha(dstA);
      }

      /* From the BLEND_STATE docs, DWord 0, Bit 29 (AlphaToOne Enable):
       * "If Dual Source Blending is enabled, this bit must be disabled."
       *
       * We override SRC1_ALPHA to ONE and ONE_MINUS_SRC1_ALPHA to ZERO,
       * and leave it enabled anyway.
       */
      if (GFX_VER >= 6 && ctx->Color._BlendUsesDualSrc & (1 << i) && alpha_to_one) {
         srcRGB = fix_dual_blend_alpha_to_one(srcRGB);
         srcA = fix_dual_blend_alpha_to_one(srcA);
         dstRGB = fix_dual_blend_alpha_to_one(dstRGB);
         dstA = fix_dual_blend_alpha_to_one(dstA);
      }

      /* BRW_NEW_FS_PROG_DATA */
      const struct brw_wm_prog_data *wm_prog_data =
         brw_wm_prog_data(brw->wm.base.prog_data);

      /* The Dual Source Blending documentation says:
       *
       * "If SRC1 is included in a src/dst blend factor and
       * a DualSource RT Write message is not used, results
       * are UNDEFINED. (This reflects the same restriction in DX APIs,
       * where undefined results are produced if “o1” is not written
       * by a PS – there are no default values defined).
       * If SRC1 is not included in a src/dst blend factor,
       * dual source blending must be disabled."
       *
       * There is no way to gracefully fix this undefined situation
       * so we just disable the blending to prevent possible issues.
       */
      entry->ColorBufferBlendEnable =
         !(ctx->Color._BlendUsesDualSrc & 0x1) || wm_prog_data->dual_src_blend;

      entry->DestinationBlendFactor = blend_factor(dstRGB);
      entry->SourceBlendFactor = blend_factor(srcRGB);
      entry->DestinationAlphaBlendFactor = blend_factor(dstA);
      entry->SourceAlphaBlendFactor = blend_factor(srcA);
      entry->ColorBlendFunction = blend_eqn(eqRGB);
      entry->AlphaBlendFunction = blend_eqn(eqA);

      if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB)
         independent_alpha_blend = true;
   }

   return independent_alpha_blend;
}

#if GFX_VER >= 6
static void
genX(upload_blend_state)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   int size;

   /* We need at least one BLEND_STATE written, because we might do
    * thread dispatch even if _NumColorDrawBuffers is 0 (for example
    * for computed depth or alpha test), which will do an FB write
    * with render target 0, which will reference BLEND_STATE[0] for
    * alpha test enable.
    */
   int nr_draw_buffers = ctx->DrawBuffer->_NumColorDrawBuffers;
   if (nr_draw_buffers == 0 && ctx->Color.AlphaEnabled)
      nr_draw_buffers = 1;

   size = GENX(BLEND_STATE_ENTRY_length) * 4 * nr_draw_buffers;
#if GFX_VER >= 8
   size += GENX(BLEND_STATE_length) * 4;
#endif

   uint32_t *blend_map;
   blend_map = brw_state_batch(brw, size, 64, &brw->cc.blend_state_offset);

#if GFX_VER >= 8
   struct GENX(BLEND_STATE) blend = { 0 };
   {
#else
   for (int i = 0; i < nr_draw_buffers; i++) {
      struct GENX(BLEND_STATE_ENTRY) entry = { 0 };
#define blend entry
#endif
      /* OpenGL specification 3.3 (page 196), section 4.1.3 says:
       * "If drawbuffer zero is not NONE and the buffer it references has an
       * integer format, the SAMPLE_ALPHA_TO_COVERAGE and SAMPLE_ALPHA_TO_ONE
       * operations are skipped."
       */
      if (!(ctx->DrawBuffer->_IntegerBuffers & 0x1)) {
         /* _NEW_MULTISAMPLE */
         if (_mesa_is_multisample_enabled(ctx)) {
            if (ctx->Multisample.SampleAlphaToCoverage) {
               blend.AlphaToCoverageEnable = true;
               blend.AlphaToCoverageDitherEnable = GFX_VER >= 7;
            }
            if (ctx->Multisample.SampleAlphaToOne)
               blend.AlphaToOneEnable = true;
         }

         /* _NEW_COLOR */
         if (ctx->Color.AlphaEnabled) {
            blend.AlphaTestEnable = true;
            blend.AlphaTestFunction =
               brw_translate_compare_func(ctx->Color.AlphaFunc);
         }

         if (ctx->Color.DitherFlag) {
            blend.ColorDitherEnable = true;
         }
      }

#if GFX_VER >= 8
      for (int i = 0; i < nr_draw_buffers; i++) {
         struct GENX(BLEND_STATE_ENTRY) entry = { 0 };
#else
      {
#endif
         blend.IndependentAlphaBlendEnable =
            set_blend_entry_bits(brw, &entry, i, blend.AlphaToOneEnable) ||
            blend.IndependentAlphaBlendEnable;

         /* See section 8.1.6 "Pre-Blend Color Clamping" of the
          * SandyBridge PRM Volume 2 Part 1 for HW requirements.
          *
          * We do our ARB_color_buffer_float CLAMP_FRAGMENT_COLOR
          * clamping in the fragment shader.  For its clamping of
          * blending, the spec says:
          *
          *     "RESOLVED: For fixed-point color buffers, the inputs and
          *      the result of the blending equation are clamped.  For
          *      floating-point color buffers, no clamping occurs."
          *
          * So, generally, we want clamping to the render target's range.
          * And, good news, the hardware tables for both pre- and
          * post-blend color clamping are either ignored, or any are
          * allowed, or clamping is required but RT range clamping is a
          * valid option.
          */
         entry.PreBlendColorClampEnable = true;
         entry.PostBlendColorClampEnable = true;
         entry.ColorClampRange = COLORCLAMP_RTFORMAT;

         entry.WriteDisableRed   = !GET_COLORMASK_BIT(ctx->Color.ColorMask, i, 0);
         entry.WriteDisableGreen = !GET_COLORMASK_BIT(ctx->Color.ColorMask, i, 1);
         entry.WriteDisableBlue  = !GET_COLORMASK_BIT(ctx->Color.ColorMask, i, 2);
         entry.WriteDisableAlpha = !GET_COLORMASK_BIT(ctx->Color.ColorMask, i, 3);

#if GFX_VER >= 8
         GENX(BLEND_STATE_ENTRY_pack)(NULL, &blend_map[1 + i * 2], &entry);
#else
         GENX(BLEND_STATE_ENTRY_pack)(NULL, &blend_map[i * 2], &entry);
#endif
      }
   }

#if GFX_VER >= 8
   GENX(BLEND_STATE_pack)(NULL, blend_map, &blend);
#endif

#if GFX_VER < 7
   brw_batch_emit(brw, GENX(3DSTATE_CC_STATE_POINTERS), ptr) {
      ptr.PointertoBLEND_STATE = brw->cc.blend_state_offset;
      ptr.BLEND_STATEChange = true;
   }
#else
   brw_batch_emit(brw, GENX(3DSTATE_BLEND_STATE_POINTERS), ptr) {
      ptr.BlendStatePointer = brw->cc.blend_state_offset;
#if GFX_VER >= 8
      ptr.BlendStatePointerValid = true;
#endif
   }
#endif
}

UNUSED static const struct brw_tracked_state genX(blend_state) = {
   .dirty = {
      .mesa = _NEW_BUFFERS |
              _NEW_COLOR |
              _NEW_MULTISAMPLE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_FS_PROG_DATA |
             BRW_NEW_STATE_BASE_ADDRESS,
   },
   .emit = genX(upload_blend_state),
};
#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER >= 7
UNUSED static const uint32_t push_constant_opcodes[] = {
   [MESA_SHADER_VERTEX]                      = 21,
   [MESA_SHADER_TESS_CTRL]                   = 25, /* HS */
   [MESA_SHADER_TESS_EVAL]                   = 26, /* DS */
   [MESA_SHADER_GEOMETRY]                    = 22,
   [MESA_SHADER_FRAGMENT]                    = 23,
   [MESA_SHADER_COMPUTE]                     = 0,
};

static void
genX(upload_push_constant_packets)(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;

   UNUSED uint32_t mocs = GFX_VER < 8 ? GFX7_MOCS_L3 : 0;

   struct brw_stage_state *stage_states[] = {
      &brw->vs.base,
      &brw->tcs.base,
      &brw->tes.base,
      &brw->gs.base,
      &brw->wm.base,
   };

   if (GFX_VERx10 == 70 && !devinfo->is_baytrail &&
       stage_states[MESA_SHADER_VERTEX]->push_constants_dirty)
      gfx7_emit_vs_workaround_flush(brw);

   for (int stage = 0; stage <= MESA_SHADER_FRAGMENT; stage++) {
      struct brw_stage_state *stage_state = stage_states[stage];
      UNUSED struct gl_program *prog = ctx->_Shader->CurrentProgram[stage];

      if (!stage_state->push_constants_dirty)
         continue;

      brw_batch_emit(brw, GENX(3DSTATE_CONSTANT_VS), pkt) {
         pkt._3DCommandSubOpcode = push_constant_opcodes[stage];
         if (stage_state->prog_data) {
#if GFX_VERx10 >= 75
            /* The Skylake PRM contains the following restriction:
             *
             *    "The driver must ensure The following case does not occur
             *     without a flush to the 3D engine: 3DSTATE_CONSTANT_* with
             *     buffer 3 read length equal to zero committed followed by a
             *     3DSTATE_CONSTANT_* with buffer 0 read length not equal to
             *     zero committed."
             *
             * To avoid this, we program the buffers in the highest slots.
             * This way, slot 0 is only used if slot 3 is also used.
             */
            int n = 3;

            for (int i = 3; i >= 0; i--) {
               const struct brw_ubo_range *range =
                  &stage_state->prog_data->ubo_ranges[i];

               if (range->length == 0)
                  continue;

               const struct gl_uniform_block *block =
                  prog->sh.UniformBlocks[range->block];
               const struct gl_buffer_binding *binding =
                  &ctx->UniformBufferBindings[block->Binding];

               if (!binding->BufferObject) {
                  static unsigned msg_id = 0;
                  _mesa_gl_debugf(ctx, &msg_id, MESA_DEBUG_SOURCE_API,
                                  MESA_DEBUG_TYPE_UNDEFINED,
                                  MESA_DEBUG_SEVERITY_HIGH,
                                  "UBO %d unbound, %s shader uniform data "
                                  "will be undefined.",
                                  range->block,
                                  _mesa_shader_stage_to_string(stage));
                  continue;
               }

               assert(binding->Offset % 32 == 0);

               struct brw_bo *bo = brw_bufferobj_buffer(brw,
                  brw_buffer_object(binding->BufferObject),
                  binding->Offset, range->length * 32, false);

               pkt.ConstantBody.ReadLength[n] = range->length;
               pkt.ConstantBody.Buffer[n] =
                  ro_bo(bo, range->start * 32 + binding->Offset);
               n--;
            }

            if (stage_state->push_const_size > 0) {
               assert(n >= 0);
               pkt.ConstantBody.ReadLength[n] = stage_state->push_const_size;
               pkt.ConstantBody.Buffer[n] =
                  ro_bo(stage_state->push_const_bo,
                        stage_state->push_const_offset);
            }
#else
            pkt.ConstantBody.ReadLength[0] = stage_state->push_const_size;
            pkt.ConstantBody.Buffer[0].offset =
               stage_state->push_const_offset | mocs;
#endif
         }
      }

      stage_state->push_constants_dirty = false;
      brw->ctx.NewDriverState |= GFX_VER >= 9 ? BRW_NEW_SURFACES : 0;
   }
}

const struct brw_tracked_state genX(push_constant_packets) = {
   .dirty = {
      .mesa  = 0,
      .brw   = BRW_NEW_DRAW_CALL,
   },
   .emit = genX(upload_push_constant_packets),
};
#endif

#if GFX_VER >= 6
static void
genX(upload_vs_push_constants)(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->vs.base;

   /* BRW_NEW_VERTEX_PROGRAM */
   const struct gl_program *vp = brw->programs[MESA_SHADER_VERTEX];
   /* BRW_NEW_VS_PROG_DATA */
   const struct brw_stage_prog_data *prog_data = brw->vs.base.prog_data;

   gfx6_upload_push_constants(brw, vp, prog_data, stage_state);
}

static const struct brw_tracked_state genX(vs_push_constants) = {
   .dirty = {
      .mesa  = _NEW_PROGRAM_CONSTANTS |
               _NEW_TRANSFORM,
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               BRW_NEW_VERTEX_PROGRAM |
               BRW_NEW_VS_PROG_DATA,
   },
   .emit = genX(upload_vs_push_constants),
};

static void
genX(upload_gs_push_constants)(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->gs.base;

   /* BRW_NEW_GEOMETRY_PROGRAM */
   const struct gl_program *gp = brw->programs[MESA_SHADER_GEOMETRY];

   /* BRW_NEW_GS_PROG_DATA */
   struct brw_stage_prog_data *prog_data = brw->gs.base.prog_data;

   gfx6_upload_push_constants(brw, gp, prog_data, stage_state);
}

static const struct brw_tracked_state genX(gs_push_constants) = {
   .dirty = {
      .mesa  = _NEW_PROGRAM_CONSTANTS |
               _NEW_TRANSFORM,
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               BRW_NEW_GEOMETRY_PROGRAM |
               BRW_NEW_GS_PROG_DATA,
   },
   .emit = genX(upload_gs_push_constants),
};

static void
genX(upload_wm_push_constants)(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->wm.base;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   const struct gl_program *fp = brw->programs[MESA_SHADER_FRAGMENT];
   /* BRW_NEW_FS_PROG_DATA */
   const struct brw_stage_prog_data *prog_data = brw->wm.base.prog_data;

   gfx6_upload_push_constants(brw, fp, prog_data, stage_state);
}

static const struct brw_tracked_state genX(wm_push_constants) = {
   .dirty = {
      .mesa  = _NEW_PROGRAM_CONSTANTS,
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               BRW_NEW_FRAGMENT_PROGRAM |
               BRW_NEW_FS_PROG_DATA,
   },
   .emit = genX(upload_wm_push_constants),
};
#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER >= 6
static unsigned
genX(determine_sample_mask)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   float coverage = 1.0f;
   float coverage_invert = false;
   unsigned sample_mask = ~0u;

   /* BRW_NEW_NUM_SAMPLES */
   unsigned num_samples = brw->num_samples;

   if (_mesa_is_multisample_enabled(ctx)) {
      if (ctx->Multisample.SampleCoverage) {
         coverage = ctx->Multisample.SampleCoverageValue;
         coverage_invert = ctx->Multisample.SampleCoverageInvert;
      }
      if (ctx->Multisample.SampleMask) {
         sample_mask = ctx->Multisample.SampleMaskValue;
      }
   }

   if (num_samples > 1) {
      int coverage_int = (int) (num_samples * coverage + 0.5f);
      uint32_t coverage_bits = (1 << coverage_int) - 1;
      if (coverage_invert)
         coverage_bits ^= (1 << num_samples) - 1;
      return coverage_bits & sample_mask;
   } else {
      return 1;
   }
}

static void
genX(emit_3dstate_multisample2)(struct brw_context *brw,
                                unsigned num_samples)
{
   unsigned log2_samples = ffs(num_samples) - 1;

   brw_batch_emit(brw, GENX(3DSTATE_MULTISAMPLE), multi) {
      multi.PixelLocation = CENTER;
      multi.NumberofMultisamples = log2_samples;
#if GFX_VER == 6
      INTEL_SAMPLE_POS_4X(multi.Sample);
#elif GFX_VER == 7
      switch (num_samples) {
      case 1:
         INTEL_SAMPLE_POS_1X(multi.Sample);
         break;
      case 2:
         INTEL_SAMPLE_POS_2X(multi.Sample);
         break;
      case 4:
         INTEL_SAMPLE_POS_4X(multi.Sample);
         break;
      case 8:
         INTEL_SAMPLE_POS_8X(multi.Sample);
         break;
      default:
         break;
      }
#endif
   }
}

static void
genX(upload_multisample_state)(struct brw_context *brw)
{
   assert(brw->num_samples > 0 && brw->num_samples <= 16);

   genX(emit_3dstate_multisample2)(brw, brw->num_samples);

   brw_batch_emit(brw, GENX(3DSTATE_SAMPLE_MASK), sm) {
      sm.SampleMask = genX(determine_sample_mask)(brw);
   }
}

static const struct brw_tracked_state genX(multisample_state) = {
   .dirty = {
      .mesa = _NEW_MULTISAMPLE |
              (GFX_VER == 10 ? _NEW_BUFFERS : 0),
      .brw = BRW_NEW_BLORP |
             BRW_NEW_CONTEXT |
             BRW_NEW_NUM_SAMPLES,
   },
   .emit = genX(upload_multisample_state)
};
#endif

/* ---------------------------------------------------------------------- */

static void
genX(upload_color_calc_state)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   brw_state_emit(brw, GENX(COLOR_CALC_STATE), 64, &brw->cc.state_offset, cc) {
#if GFX_VER <= 5
      cc.IndependentAlphaBlendEnable =
         set_blend_entry_bits(brw, &cc, 0, false);
      set_depth_stencil_bits(brw, &cc);

      if (ctx->Color.AlphaEnabled &&
          ctx->DrawBuffer->_NumColorDrawBuffers <= 1) {
         cc.AlphaTestEnable = true;
         cc.AlphaTestFunction =
            brw_translate_compare_func(ctx->Color.AlphaFunc);
      }

      cc.ColorDitherEnable = ctx->Color.DitherFlag;

      cc.StatisticsEnable = brw->stats_wm;

      cc.CCViewportStatePointer =
         ro_bo(brw->batch.state.bo, brw->cc.vp_offset);
#else
      /* _NEW_COLOR */
      cc.BlendConstantColorRed = ctx->Color.BlendColorUnclamped[0];
      cc.BlendConstantColorGreen = ctx->Color.BlendColorUnclamped[1];
      cc.BlendConstantColorBlue = ctx->Color.BlendColorUnclamped[2];
      cc.BlendConstantColorAlpha = ctx->Color.BlendColorUnclamped[3];

#if GFX_VER < 9
      /* _NEW_STENCIL */
      cc.StencilReferenceValue = _mesa_get_stencil_ref(ctx, 0);
      cc.BackfaceStencilReferenceValue =
         _mesa_get_stencil_ref(ctx, ctx->Stencil._BackFace);
#endif

#endif

      /* _NEW_COLOR */
      UNCLAMPED_FLOAT_TO_UBYTE(cc.AlphaReferenceValueAsUNORM8,
                               ctx->Color.AlphaRef);
   }

#if GFX_VER >= 6
   brw_batch_emit(brw, GENX(3DSTATE_CC_STATE_POINTERS), ptr) {
      ptr.ColorCalcStatePointer = brw->cc.state_offset;
#if GFX_VER != 7
      ptr.ColorCalcStatePointerValid = true;
#endif
   }
#else
   brw->ctx.NewDriverState |= BRW_NEW_GFX4_UNIT_STATE;
#endif
}

UNUSED static const struct brw_tracked_state genX(color_calc_state) = {
   .dirty = {
      .mesa = _NEW_COLOR |
              _NEW_STENCIL |
              (GFX_VER <= 5 ? _NEW_BUFFERS |
                              _NEW_DEPTH
                            : 0),
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             (GFX_VER <= 5 ? BRW_NEW_CC_VP |
                             BRW_NEW_STATS_WM
                           : BRW_NEW_CC_STATE |
                             BRW_NEW_STATE_BASE_ADDRESS),
   },
   .emit = genX(upload_color_calc_state),
};


/* ---------------------------------------------------------------------- */

#if GFX_VERx10 == 75
static void
genX(upload_color_calc_and_blend_state)(struct brw_context *brw)
{
   genX(upload_blend_state)(brw);
   genX(upload_color_calc_state)(brw);
}

/* On Haswell when BLEND_STATE is emitted CC_STATE should also be re-emitted,
 * this workarounds the flickering shadows in several games.
 */
static const struct brw_tracked_state genX(cc_and_blend_state) = {
   .dirty = {
      .mesa = _NEW_BUFFERS |
              _NEW_COLOR |
              _NEW_STENCIL |
              _NEW_MULTISAMPLE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_CC_STATE |
             BRW_NEW_FS_PROG_DATA |
             BRW_NEW_STATE_BASE_ADDRESS,
   },
   .emit = genX(upload_color_calc_and_blend_state),
};
#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER >= 7
static void
genX(upload_sbe)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   UNUSED const struct gl_program *fp = brw->programs[MESA_SHADER_FRAGMENT];
   /* BRW_NEW_FS_PROG_DATA */
   const struct brw_wm_prog_data *wm_prog_data =
      brw_wm_prog_data(brw->wm.base.prog_data);
#if GFX_VER >= 8
   struct GENX(SF_OUTPUT_ATTRIBUTE_DETAIL) attr_overrides[16] = { { 0 } };
#else
#define attr_overrides sbe.Attribute
#endif
   uint32_t urb_entry_read_length;
   uint32_t urb_entry_read_offset;
   uint32_t point_sprite_enables;

   brw_batch_emit(brw, GENX(3DSTATE_SBE), sbe) {
      sbe.AttributeSwizzleEnable = true;
      sbe.NumberofSFOutputAttributes = wm_prog_data->num_varying_inputs;

      /* _NEW_BUFFERS */
      bool flip_y = ctx->DrawBuffer->FlipY;

      /* _NEW_POINT
       *
       * Window coordinates in an FBO are inverted, which means point
       * sprite origin must be inverted.
       */
      if ((ctx->Point.SpriteOrigin == GL_LOWER_LEFT) == flip_y)
         sbe.PointSpriteTextureCoordinateOrigin = LOWERLEFT;
      else
         sbe.PointSpriteTextureCoordinateOrigin = UPPERLEFT;

      /* _NEW_POINT | _NEW_LIGHT | _NEW_PROGRAM,
       * BRW_NEW_FS_PROG_DATA | BRW_NEW_FRAGMENT_PROGRAM |
       * BRW_NEW_GS_PROG_DATA | BRW_NEW_PRIMITIVE | BRW_NEW_TES_PROG_DATA |
       * BRW_NEW_VUE_MAP_GEOM_OUT
       */
      genX(calculate_attr_overrides)(brw,
                                     attr_overrides,
                                     &point_sprite_enables,
                                     &urb_entry_read_length,
                                     &urb_entry_read_offset);

      /* Typically, the URB entry read length and offset should be programmed
       * in 3DSTATE_VS and 3DSTATE_GS; SBE inherits it from the last active
       * stage which produces geometry.  However, we don't know the proper
       * value until we call calculate_attr_overrides().
       *
       * To fit with our existing code, we override the inherited values and
       * specify it here directly, as we did on previous generations.
       */
      sbe.VertexURBEntryReadLength = urb_entry_read_length;
      sbe.VertexURBEntryReadOffset = urb_entry_read_offset;
      sbe.PointSpriteTextureCoordinateEnable = point_sprite_enables;
      sbe.ConstantInterpolationEnable = wm_prog_data->flat_inputs;

#if GFX_VER >= 8
      sbe.ForceVertexURBEntryReadLength = true;
      sbe.ForceVertexURBEntryReadOffset = true;
#endif

#if GFX_VER >= 9
      /* prepare the active component dwords */
      for (int i = 0; i < 32; i++)
         sbe.AttributeActiveComponentFormat[i] = ACTIVE_COMPONENT_XYZW;
#endif
   }

#if GFX_VER >= 8
   brw_batch_emit(brw, GENX(3DSTATE_SBE_SWIZ), sbes) {
      for (int i = 0; i < 16; i++)
         sbes.Attribute[i] = attr_overrides[i];
   }
#endif

#undef attr_overrides
}

static const struct brw_tracked_state genX(sbe_state) = {
   .dirty = {
      .mesa  = _NEW_BUFFERS |
               _NEW_LIGHT |
               _NEW_POINT |
               _NEW_POLYGON |
               _NEW_PROGRAM,
      .brw   = BRW_NEW_BLORP |
               BRW_NEW_CONTEXT |
               BRW_NEW_FRAGMENT_PROGRAM |
               BRW_NEW_FS_PROG_DATA |
               BRW_NEW_GS_PROG_DATA |
               BRW_NEW_TES_PROG_DATA |
               BRW_NEW_VUE_MAP_GEOM_OUT |
               (GFX_VER == 7 ? BRW_NEW_PRIMITIVE
                             : 0),
   },
   .emit = genX(upload_sbe),
};
#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER >= 7
/**
 * Outputs the 3DSTATE_SO_DECL_LIST command.
 *
 * The data output is a series of 64-bit entries containing a SO_DECL per
 * stream.  We only have one stream of rendering coming out of the GS unit, so
 * we only emit stream 0 (low 16 bits) SO_DECLs.
 */
static void
genX(upload_3dstate_so_decl_list)(struct brw_context *brw,
                                  const struct brw_vue_map *vue_map)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_TRANSFORM_FEEDBACK */
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;
   const struct gl_transform_feedback_info *linked_xfb_info =
      xfb_obj->program->sh.LinkedTransformFeedback;
   struct GENX(SO_DECL) so_decl[MAX_VERTEX_STREAMS][128];
   int buffer_mask[MAX_VERTEX_STREAMS] = {0, 0, 0, 0};
   int next_offset[MAX_VERTEX_STREAMS] = {0, 0, 0, 0};
   int decls[MAX_VERTEX_STREAMS] = {0, 0, 0, 0};
   int max_decls = 0;
   STATIC_ASSERT(ARRAY_SIZE(so_decl[0]) >= MAX_PROGRAM_OUTPUTS);

   memset(so_decl, 0, sizeof(so_decl));

   /* Construct the list of SO_DECLs to be emitted.  The formatting of the
    * command feels strange -- each dword pair contains a SO_DECL per stream.
    */
   for (unsigned i = 0; i < linked_xfb_info->NumOutputs; i++) {
      const struct gl_transform_feedback_output *output =
         &linked_xfb_info->Outputs[i];
      const int buffer = output->OutputBuffer;
      const int varying = output->OutputRegister;
      const unsigned stream_id = output->StreamId;
      assert(stream_id < MAX_VERTEX_STREAMS);

      buffer_mask[stream_id] |= 1 << buffer;

      assert(vue_map->varying_to_slot[varying] >= 0);

      /* Mesa doesn't store entries for gl_SkipComponents in the Outputs[]
       * array.  Instead, it simply increments DstOffset for the following
       * input by the number of components that should be skipped.
       *
       * Our hardware is unusual in that it requires us to program SO_DECLs
       * for fake "hole" components, rather than simply taking the offset
       * for each real varying.  Each hole can have size 1, 2, 3, or 4; we
       * program as many size = 4 holes as we can, then a final hole to
       * accommodate the final 1, 2, or 3 remaining.
       */
      int skip_components = output->DstOffset - next_offset[buffer];

      while (skip_components > 0) {
         so_decl[stream_id][decls[stream_id]++] = (struct GENX(SO_DECL)) {
            .HoleFlag = 1,
            .OutputBufferSlot = output->OutputBuffer,
            .ComponentMask = (1 << MIN2(skip_components, 4)) - 1,
         };
         skip_components -= 4;
      }

      next_offset[buffer] = output->DstOffset + output->NumComponents;

      so_decl[stream_id][decls[stream_id]++] = (struct GENX(SO_DECL)) {
         .OutputBufferSlot = output->OutputBuffer,
         .RegisterIndex = vue_map->varying_to_slot[varying],
         .ComponentMask =
            ((1 << output->NumComponents) - 1) << output->ComponentOffset,
      };

      if (decls[stream_id] > max_decls)
         max_decls = decls[stream_id];
   }

   uint32_t *dw;
   dw = brw_batch_emitn(brw, GENX(3DSTATE_SO_DECL_LIST), 3 + 2 * max_decls,
                        .StreamtoBufferSelects0 = buffer_mask[0],
                        .StreamtoBufferSelects1 = buffer_mask[1],
                        .StreamtoBufferSelects2 = buffer_mask[2],
                        .StreamtoBufferSelects3 = buffer_mask[3],
                        .NumEntries0 = decls[0],
                        .NumEntries1 = decls[1],
                        .NumEntries2 = decls[2],
                        .NumEntries3 = decls[3]);

   for (int i = 0; i < max_decls; i++) {
      GENX(SO_DECL_ENTRY_pack)(
         brw, dw + 2 + i * 2,
         &(struct GENX(SO_DECL_ENTRY)) {
            .Stream0Decl = so_decl[0][i],
            .Stream1Decl = so_decl[1][i],
            .Stream2Decl = so_decl[2][i],
            .Stream3Decl = so_decl[3][i],
         });
   }
}

static void
genX(upload_3dstate_so_buffers)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_TRANSFORM_FEEDBACK */
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;
#if GFX_VER < 8
   const struct gl_transform_feedback_info *linked_xfb_info =
      xfb_obj->program->sh.LinkedTransformFeedback;
#else
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) xfb_obj;
   uint32_t mocs_wb = GFX_VER >= 9 ? SKL_MOCS_WB : BDW_MOCS_WB;
#endif

   /* Set up the up to 4 output buffers.  These are the ranges defined in the
    * gl_transform_feedback_object.
    */
   for (int i = 0; i < 4; i++) {
      struct brw_buffer_object *bufferobj =
         brw_buffer_object(xfb_obj->Buffers[i]);
      uint32_t start = xfb_obj->Offset[i];
      uint32_t end = ALIGN(start + xfb_obj->Size[i], 4);
      uint32_t const size = end - start;

      if (!bufferobj || !size) {
         brw_batch_emit(brw, GENX(3DSTATE_SO_BUFFER), sob) {
            sob.SOBufferIndex = i;
         }
         continue;
      }

      assert(start % 4 == 0);
      struct brw_bo *bo =
         brw_bufferobj_buffer(brw, bufferobj, start, size, true);
      assert(end <= bo->size);

      brw_batch_emit(brw, GENX(3DSTATE_SO_BUFFER), sob) {
         sob.SOBufferIndex = i;

         sob.SurfaceBaseAddress = rw_bo(bo, start);
#if GFX_VER < 8
         sob.SurfacePitch = linked_xfb_info->Buffers[i].Stride * 4;
         sob.SurfaceEndAddress = rw_bo(bo, end);
#else
         sob.SOBufferEnable = true;
         sob.StreamOffsetWriteEnable = true;
         sob.StreamOutputBufferOffsetAddressEnable = true;
         sob.MOCS = mocs_wb;

         sob.SurfaceSize = MAX2(xfb_obj->Size[i] / 4, 1) - 1;
         sob.StreamOutputBufferOffsetAddress =
            rw_bo(brw_obj->offset_bo, i * sizeof(uint32_t));

         if (brw_obj->zero_offsets) {
            /* Zero out the offset and write that to offset_bo */
            sob.StreamOffset = 0;
         } else {
            /* Use offset_bo as the "Stream Offset." */
            sob.StreamOffset = 0xFFFFFFFF;
         }
#endif
      }
   }

#if GFX_VER >= 8
   brw_obj->zero_offsets = false;
#endif
}

static bool
query_active(struct gl_query_object *q)
{
   return q && q->Active;
}

static void
genX(upload_3dstate_streamout)(struct brw_context *brw, bool active,
                               const struct brw_vue_map *vue_map)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_TRANSFORM_FEEDBACK */
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;

   brw_batch_emit(brw, GENX(3DSTATE_STREAMOUT), sos) {
      if (active) {
         int urb_entry_read_offset = 0;
         int urb_entry_read_length = (vue_map->num_slots + 1) / 2 -
            urb_entry_read_offset;

         sos.SOFunctionEnable = true;
         sos.SOStatisticsEnable = true;

         /* BRW_NEW_RASTERIZER_DISCARD */
         if (ctx->RasterDiscard) {
            if (!query_active(ctx->Query.PrimitivesGenerated[0])) {
               sos.RenderingDisable = true;
            } else {
               perf_debug("Rasterizer discard with a GL_PRIMITIVES_GENERATED "
                          "query active relies on the clipper.\n");
            }
         }

         /* _NEW_LIGHT */
         if (ctx->Light.ProvokingVertex != GL_FIRST_VERTEX_CONVENTION)
            sos.ReorderMode = TRAILING;

#if GFX_VER < 8
         sos.SOBufferEnable0 = xfb_obj->Buffers[0] != NULL;
         sos.SOBufferEnable1 = xfb_obj->Buffers[1] != NULL;
         sos.SOBufferEnable2 = xfb_obj->Buffers[2] != NULL;
         sos.SOBufferEnable3 = xfb_obj->Buffers[3] != NULL;
#else
         const struct gl_transform_feedback_info *linked_xfb_info =
            xfb_obj->program->sh.LinkedTransformFeedback;
         /* Set buffer pitches; 0 means unbound. */
         if (xfb_obj->Buffers[0])
            sos.Buffer0SurfacePitch = linked_xfb_info->Buffers[0].Stride * 4;
         if (xfb_obj->Buffers[1])
            sos.Buffer1SurfacePitch = linked_xfb_info->Buffers[1].Stride * 4;
         if (xfb_obj->Buffers[2])
            sos.Buffer2SurfacePitch = linked_xfb_info->Buffers[2].Stride * 4;
         if (xfb_obj->Buffers[3])
            sos.Buffer3SurfacePitch = linked_xfb_info->Buffers[3].Stride * 4;
#endif

         /* We always read the whole vertex.  This could be reduced at some
          * point by reading less and offsetting the register index in the
          * SO_DECLs.
          */
         sos.Stream0VertexReadOffset = urb_entry_read_offset;
         sos.Stream0VertexReadLength = urb_entry_read_length - 1;
         sos.Stream1VertexReadOffset = urb_entry_read_offset;
         sos.Stream1VertexReadLength = urb_entry_read_length - 1;
         sos.Stream2VertexReadOffset = urb_entry_read_offset;
         sos.Stream2VertexReadLength = urb_entry_read_length - 1;
         sos.Stream3VertexReadOffset = urb_entry_read_offset;
         sos.Stream3VertexReadLength = urb_entry_read_length - 1;
      }
   }
}

static void
genX(upload_sol)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_TRANSFORM_FEEDBACK */
   bool active = _mesa_is_xfb_active_and_unpaused(ctx);

   if (active) {
      genX(upload_3dstate_so_buffers)(brw);

      /* BRW_NEW_VUE_MAP_GEOM_OUT */
      genX(upload_3dstate_so_decl_list)(brw, &brw->vue_map_geom_out);
   }

   /* Finally, set up the SOL stage.  This command must always follow updates to
    * the nonpipelined SOL state (3DSTATE_SO_BUFFER, 3DSTATE_SO_DECL_LIST) or
    * MMIO register updates (current performed by the kernel at each batch
    * emit).
    */
   genX(upload_3dstate_streamout)(brw, active, &brw->vue_map_geom_out);
}

static const struct brw_tracked_state genX(sol_state) = {
   .dirty = {
      .mesa  = _NEW_LIGHT,
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               BRW_NEW_RASTERIZER_DISCARD |
               BRW_NEW_VUE_MAP_GEOM_OUT |
               BRW_NEW_TRANSFORM_FEEDBACK,
   },
   .emit = genX(upload_sol),
};
#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER >= 7
static void
genX(upload_ps)(struct brw_context *brw)
{
   UNUSED const struct gl_context *ctx = &brw->ctx;
   UNUSED const struct intel_device_info *devinfo = &brw->screen->devinfo;

   /* BRW_NEW_FS_PROG_DATA */
   const struct brw_wm_prog_data *prog_data =
      brw_wm_prog_data(brw->wm.base.prog_data);
   const struct brw_stage_state *stage_state = &brw->wm.base;

#if GFX_VER < 8
#endif

   brw_batch_emit(brw, GENX(3DSTATE_PS), ps) {
      /* Initialize the execution mask with VMask.  Otherwise, derivatives are
       * incorrect for subspans where some of the pixels are unlit.  We believe
       * the bit just didn't take effect in previous generations.
       */
      ps.VectorMaskEnable = GFX_VER >= 8;

      /* Wa_1606682166:
       * "Incorrect TDL's SSP address shift in SARB for 16:6 & 18:8 modes.
       * Disable the Sampler state prefetch functionality in the SARB by
       * programming 0xB000[30] to '1'."
       */
      ps.SamplerCount = GFX_VER == 11 ?
         0 : DIV_ROUND_UP(CLAMP(stage_state->sampler_count, 0, 16), 4);

      /* BRW_NEW_FS_PROG_DATA */
      ps.BindingTableEntryCount = prog_data->base.binding_table.size_bytes / 4;

      if (prog_data->base.use_alt_mode)
         ps.FloatingPointMode = Alternate;

      /* Haswell requires the sample mask to be set in this packet as well as
       * in 3DSTATE_SAMPLE_MASK; the values should match.
       */

      /* _NEW_BUFFERS, _NEW_MULTISAMPLE */
#if GFX_VERx10 == 75
      ps.SampleMask = genX(determine_sample_mask(brw));
#endif

      /* 3DSTATE_PS expects the number of threads per PSD, which is always 64
       * for pre Gfx11 and 128 for gfx11+; On gfx11+ If a programmed value is
       * k, it implies 2(k+1) threads. It implicitly scales for different GT
       * levels (which have some # of PSDs).
       *
       * In Gfx8 the format is U8-2 whereas in Gfx9+ it is U9-1.
       */
#if GFX_VER >= 9
      ps.MaximumNumberofThreadsPerPSD = 64 - 1;
#elif GFX_VER >= 8
      ps.MaximumNumberofThreadsPerPSD = 64 - 2;
#else
      ps.MaximumNumberofThreads = devinfo->max_wm_threads - 1;
#endif

      if (prog_data->base.nr_params > 0 ||
          prog_data->base.ubo_ranges[0].length > 0)
         ps.PushConstantEnable = true;

#if GFX_VER < 8
      /* From the IVB PRM, volume 2 part 1, page 287:
       * "This bit is inserted in the PS payload header and made available to
       * the DataPort (either via the message header or via header bypass) to
       * indicate that oMask data (one or two phases) is included in Render
       * Target Write messages. If present, the oMask data is used to mask off
       * samples."
       */
      ps.oMaskPresenttoRenderTarget = prog_data->uses_omask;

      /* The hardware wedges if you have this bit set but don't turn on any
       * dual source blend factors.
       *
       * BRW_NEW_FS_PROG_DATA | _NEW_COLOR
       */
      ps.DualSourceBlendEnable = prog_data->dual_src_blend &&
                                 (ctx->Color.BlendEnabled & 1) &&
                                 ctx->Color._BlendUsesDualSrc & 0x1;

      /* BRW_NEW_FS_PROG_DATA */
      ps.AttributeEnable = (prog_data->num_varying_inputs != 0);
#endif

      /* From the documentation for this packet:
       * "If the PS kernel does not need the Position XY Offsets to
       *  compute a Position Value, then this field should be programmed
       *  to POSOFFSET_NONE."
       *
       * "SW Recommendation: If the PS kernel needs the Position Offsets
       *  to compute a Position XY value, this field should match Position
       *  ZW Interpolation Mode to ensure a consistent position.xyzw
       *  computation."
       *
       * We only require XY sample offsets. So, this recommendation doesn't
       * look useful at the moment. We might need this in future.
       */
      if (prog_data->uses_pos_offset)
         ps.PositionXYOffsetSelect = POSOFFSET_SAMPLE;
      else
         ps.PositionXYOffsetSelect = POSOFFSET_NONE;

      ps._8PixelDispatchEnable = prog_data->dispatch_8;
      ps._16PixelDispatchEnable = prog_data->dispatch_16;
      ps._32PixelDispatchEnable = prog_data->dispatch_32;

      /* From the Sky Lake PRM 3DSTATE_PS::32 Pixel Dispatch Enable:
       *
       *    "When NUM_MULTISAMPLES = 16 or FORCE_SAMPLE_COUNT = 16, SIMD32
       *    Dispatch must not be enabled for PER_PIXEL dispatch mode."
       *
       * Since 16x MSAA is first introduced on SKL, we don't need to apply
       * the workaround on any older hardware.
       *
       * BRW_NEW_NUM_SAMPLES
       */
      if (GFX_VER >= 9 && !prog_data->persample_dispatch &&
          brw->num_samples == 16) {
         assert(ps._8PixelDispatchEnable || ps._16PixelDispatchEnable);
         ps._32PixelDispatchEnable = false;
      }

      ps.DispatchGRFStartRegisterForConstantSetupData0 =
         brw_wm_prog_data_dispatch_grf_start_reg(prog_data, ps, 0);
      ps.DispatchGRFStartRegisterForConstantSetupData1 =
         brw_wm_prog_data_dispatch_grf_start_reg(prog_data, ps, 1);
      ps.DispatchGRFStartRegisterForConstantSetupData2 =
         brw_wm_prog_data_dispatch_grf_start_reg(prog_data, ps, 2);

      ps.KernelStartPointer0 = stage_state->prog_offset +
                               brw_wm_prog_data_prog_offset(prog_data, ps, 0);
      ps.KernelStartPointer1 = stage_state->prog_offset +
                               brw_wm_prog_data_prog_offset(prog_data, ps, 1);
      ps.KernelStartPointer2 = stage_state->prog_offset +
                               brw_wm_prog_data_prog_offset(prog_data, ps, 2);

      if (prog_data->base.total_scratch) {
         ps.ScratchSpaceBasePointer =
            rw_32_bo(stage_state->scratch_bo,
                     ffs(stage_state->per_thread_scratch) - 11);
      }
   }
}

static const struct brw_tracked_state genX(ps_state) = {
   .dirty = {
      .mesa  = _NEW_MULTISAMPLE |
               (GFX_VER < 8 ? _NEW_BUFFERS |
                              _NEW_COLOR
                            : 0),
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               BRW_NEW_FS_PROG_DATA |
               (GFX_VER >= 9 ? BRW_NEW_NUM_SAMPLES : 0),
   },
   .emit = genX(upload_ps),
};
#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER >= 7
static void
genX(upload_hs_state)(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct brw_stage_state *stage_state = &brw->tcs.base;
   struct brw_stage_prog_data *stage_prog_data = stage_state->prog_data;
   const struct brw_vue_prog_data *vue_prog_data =
      brw_vue_prog_data(stage_prog_data);

   /* BRW_NEW_TES_PROG_DATA */
   struct brw_tcs_prog_data *tcs_prog_data =
      brw_tcs_prog_data(stage_prog_data);

   if (!tcs_prog_data) {
      brw_batch_emit(brw, GENX(3DSTATE_HS), hs);
   } else {
      brw_batch_emit(brw, GENX(3DSTATE_HS), hs) {
         INIT_THREAD_DISPATCH_FIELDS(hs, Vertex);

         hs.InstanceCount = tcs_prog_data->instances - 1;
         hs.IncludeVertexHandles = true;

         hs.MaximumNumberofThreads = devinfo->max_tcs_threads - 1;

#if GFX_VER >= 9
         hs.DispatchMode = vue_prog_data->dispatch_mode;
         hs.IncludePrimitiveID = tcs_prog_data->include_primitive_id;
#endif
      }
   }
}

static const struct brw_tracked_state genX(hs_state) = {
   .dirty = {
      .mesa  = 0,
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               BRW_NEW_TCS_PROG_DATA |
               BRW_NEW_TESS_PROGRAMS,
   },
   .emit = genX(upload_hs_state),
};

static void
genX(upload_ds_state)(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   const struct brw_stage_state *stage_state = &brw->tes.base;
   struct brw_stage_prog_data *stage_prog_data = stage_state->prog_data;

   /* BRW_NEW_TES_PROG_DATA */
   const struct brw_tes_prog_data *tes_prog_data =
      brw_tes_prog_data(stage_prog_data);
   const struct brw_vue_prog_data *vue_prog_data =
      brw_vue_prog_data(stage_prog_data);

   if (!tes_prog_data) {
      brw_batch_emit(brw, GENX(3DSTATE_DS), ds);
   } else {
      assert(GFX_VER < 11 ||
             vue_prog_data->dispatch_mode == DISPATCH_MODE_SIMD8);

      brw_batch_emit(brw, GENX(3DSTATE_DS), ds) {
         INIT_THREAD_DISPATCH_FIELDS(ds, Patch);

        ds.MaximumNumberofThreads = devinfo->max_tes_threads - 1;
        ds.ComputeWCoordinateEnable =
           tes_prog_data->domain == BRW_TESS_DOMAIN_TRI;

#if GFX_VER >= 8
        if (vue_prog_data->dispatch_mode == DISPATCH_MODE_SIMD8)
           ds.DispatchMode = DISPATCH_MODE_SIMD8_SINGLE_PATCH;
        ds.UserClipDistanceCullTestEnableBitmask =
            vue_prog_data->cull_distance_mask;
#endif
      }
   }
}

static const struct brw_tracked_state genX(ds_state) = {
   .dirty = {
      .mesa  = 0,
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               BRW_NEW_TESS_PROGRAMS |
               BRW_NEW_TES_PROG_DATA,
   },
   .emit = genX(upload_ds_state),
};

/* ---------------------------------------------------------------------- */

static void
upload_te_state(struct brw_context *brw)
{
   /* BRW_NEW_TESS_PROGRAMS */
   bool active = brw->programs[MESA_SHADER_TESS_EVAL];

   /* BRW_NEW_TES_PROG_DATA */
   const struct brw_tes_prog_data *tes_prog_data =
      brw_tes_prog_data(brw->tes.base.prog_data);

   if (active) {
      brw_batch_emit(brw, GENX(3DSTATE_TE), te) {
         te.Partitioning = tes_prog_data->partitioning;
         te.OutputTopology = tes_prog_data->output_topology;
         te.TEDomain = tes_prog_data->domain;
         te.TEEnable = true;
         te.MaximumTessellationFactorOdd = 63.0;
         te.MaximumTessellationFactorNotOdd = 64.0;
      }
   } else {
      brw_batch_emit(brw, GENX(3DSTATE_TE), te);
   }
}

static const struct brw_tracked_state genX(te_state) = {
   .dirty = {
      .mesa  = 0,
      .brw   = BRW_NEW_BLORP |
               BRW_NEW_CONTEXT |
               BRW_NEW_TES_PROG_DATA |
               BRW_NEW_TESS_PROGRAMS,
   },
   .emit = upload_te_state,
};

/* ---------------------------------------------------------------------- */

static void
genX(upload_tes_push_constants)(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->tes.base;
   /* BRW_NEW_TESS_PROGRAMS */
   const struct gl_program *tep = brw->programs[MESA_SHADER_TESS_EVAL];

   /* BRW_NEW_TES_PROG_DATA */
   const struct brw_stage_prog_data *prog_data = brw->tes.base.prog_data;
   gfx6_upload_push_constants(brw, tep, prog_data, stage_state);
}

static const struct brw_tracked_state genX(tes_push_constants) = {
   .dirty = {
      .mesa  = _NEW_PROGRAM_CONSTANTS,
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               BRW_NEW_TESS_PROGRAMS |
               BRW_NEW_TES_PROG_DATA,
   },
   .emit = genX(upload_tes_push_constants),
};

static void
genX(upload_tcs_push_constants)(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->tcs.base;
   /* BRW_NEW_TESS_PROGRAMS */
   const struct gl_program *tcp = brw->programs[MESA_SHADER_TESS_CTRL];

   /* BRW_NEW_TCS_PROG_DATA */
   const struct brw_stage_prog_data *prog_data = brw->tcs.base.prog_data;

   gfx6_upload_push_constants(brw, tcp, prog_data, stage_state);
}

static const struct brw_tracked_state genX(tcs_push_constants) = {
   .dirty = {
      .mesa  = _NEW_PROGRAM_CONSTANTS,
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               BRW_NEW_DEFAULT_TESS_LEVELS |
               BRW_NEW_TESS_PROGRAMS |
               BRW_NEW_TCS_PROG_DATA,
   },
   .emit = genX(upload_tcs_push_constants),
};

#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER >= 7
static void
genX(upload_cs_push_constants)(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->cs.base;

   /* BRW_NEW_COMPUTE_PROGRAM */
   const struct gl_program *cp = brw->programs[MESA_SHADER_COMPUTE];

   if (cp) {
      /* BRW_NEW_CS_PROG_DATA */
      struct brw_cs_prog_data *cs_prog_data =
         brw_cs_prog_data(brw->cs.base.prog_data);

      _mesa_shader_write_subroutine_indices(&brw->ctx, MESA_SHADER_COMPUTE);
      brw_upload_cs_push_constants(brw, cp, cs_prog_data, stage_state);
   }
}

const struct brw_tracked_state genX(cs_push_constants) = {
   .dirty = {
      .mesa = _NEW_PROGRAM_CONSTANTS,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_COMPUTE_PROGRAM |
             BRW_NEW_CS_PROG_DATA,
   },
   .emit = genX(upload_cs_push_constants),
};

/**
 * Creates a new CS constant buffer reflecting the current CS program's
 * constants, if needed by the CS program.
 */
static void
genX(upload_cs_pull_constants)(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->cs.base;

   /* BRW_NEW_COMPUTE_PROGRAM */
   struct brw_program *cp =
      (struct brw_program *) brw->programs[MESA_SHADER_COMPUTE];

   /* BRW_NEW_CS_PROG_DATA */
   const struct brw_stage_prog_data *prog_data = brw->cs.base.prog_data;

   _mesa_shader_write_subroutine_indices(&brw->ctx, MESA_SHADER_COMPUTE);
   /* _NEW_PROGRAM_CONSTANTS */
   brw_upload_pull_constants(brw, BRW_NEW_SURFACES, &cp->program,
                             stage_state, prog_data);
}

const struct brw_tracked_state genX(cs_pull_constants) = {
   .dirty = {
      .mesa = _NEW_PROGRAM_CONSTANTS,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_COMPUTE_PROGRAM |
             BRW_NEW_CS_PROG_DATA,
   },
   .emit = genX(upload_cs_pull_constants),
};

static void
genX(upload_cs_state)(struct brw_context *brw)
{
   if (!brw->cs.base.prog_data)
      return;

   uint32_t offset;
   uint32_t *desc = (uint32_t*) brw_state_batch(
      brw, GENX(INTERFACE_DESCRIPTOR_DATA_length) * sizeof(uint32_t), 64,
      &offset);

   struct brw_stage_state *stage_state = &brw->cs.base;
   struct brw_stage_prog_data *prog_data = stage_state->prog_data;
   struct brw_cs_prog_data *cs_prog_data = brw_cs_prog_data(prog_data);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   const struct brw_cs_dispatch_info dispatch =
      brw_cs_get_dispatch_info(devinfo, cs_prog_data, brw->compute.group_size);

   if (INTEL_DEBUG(DEBUG_SHADER_TIME)) {
      brw_emit_buffer_surface_state(
         brw, &stage_state->surf_offset[
                 prog_data->binding_table.shader_time_start],
         brw->shader_time.bo, 0, ISL_FORMAT_RAW,
         brw->shader_time.bo->size, 1,
         RELOC_WRITE);
   }

   uint32_t *bind = brw_state_batch(brw, prog_data->binding_table.size_bytes,
                                    32, &stage_state->bind_bo_offset);

   /* The MEDIA_VFE_STATE documentation for Gfx8+ says:
    *
    * "A stalling PIPE_CONTROL is required before MEDIA_VFE_STATE unless
    *  the only bits that are changed are scoreboard related: Scoreboard
    *  Enable, Scoreboard Type, Scoreboard Mask, Scoreboard * Delta. For
    *  these scoreboard related states, a MEDIA_STATE_FLUSH is sufficient."
    *
    * Earlier generations say "MI_FLUSH" instead of "stalling PIPE_CONTROL",
    * but MI_FLUSH isn't really a thing, so we assume they meant PIPE_CONTROL.
    */
   brw_emit_pipe_control_flush(brw, PIPE_CONTROL_CS_STALL);

   brw_batch_emit(brw, GENX(MEDIA_VFE_STATE), vfe) {
      if (prog_data->total_scratch) {
         uint32_t per_thread_scratch_value;

         if (GFX_VER >= 8) {
            /* Broadwell's Per Thread Scratch Space is in the range [0, 11]
             * where 0 = 1k, 1 = 2k, 2 = 4k, ..., 11 = 2M.
             */
            per_thread_scratch_value = ffs(stage_state->per_thread_scratch) - 11;
         } else if (GFX_VERx10 == 75) {
            /* Haswell's Per Thread Scratch Space is in the range [0, 10]
             * where 0 = 2k, 1 = 4k, 2 = 8k, ..., 10 = 2M.
             */
            per_thread_scratch_value = ffs(stage_state->per_thread_scratch) - 12;
         } else {
            /* Earlier platforms use the range [0, 11] to mean [1kB, 12kB]
             * where 0 = 1kB, 1 = 2kB, 2 = 3kB, ..., 11 = 12kB.
             */
            per_thread_scratch_value = stage_state->per_thread_scratch / 1024 - 1;
         }
         vfe.ScratchSpaceBasePointer = rw_32_bo(stage_state->scratch_bo, 0);
         vfe.PerThreadScratchSpace = per_thread_scratch_value;
      }

      vfe.MaximumNumberofThreads =
         devinfo->max_cs_threads * devinfo->subslice_total - 1;
      vfe.NumberofURBEntries = GFX_VER >= 8 ? 2 : 0;
#if GFX_VER < 11
      vfe.ResetGatewayTimer =
         Resettingrelativetimerandlatchingtheglobaltimestamp;
#endif
#if GFX_VER < 9
      vfe.BypassGatewayControl = BypassingOpenGatewayCloseGatewayprotocol;
#endif
#if GFX_VER == 7
      vfe.GPGPUMode = true;
#endif

      /* We are uploading duplicated copies of push constant uniforms for each
       * thread. Although the local id data needs to vary per thread, it won't
       * change for other uniform data. Unfortunately this duplication is
       * required for gfx7. As of Haswell, this duplication can be avoided,
       * but this older mechanism with duplicated data continues to work.
       *
       * FINISHME: As of Haswell, we could make use of the
       * INTERFACE_DESCRIPTOR_DATA "Cross-Thread Constant Data Read Length"
       * field to only store one copy of uniform data.
       *
       * FINISHME: Broadwell adds a new alternative "Indirect Payload Storage"
       * which is described in the GPGPU_WALKER command and in the Broadwell
       * PRM Volume 7: 3D Media GPGPU, under Media GPGPU Pipeline => Mode of
       * Operations => GPGPU Mode => Indirect Payload Storage.
       *
       * Note: The constant data is built in brw_upload_cs_push_constants
       * below.
       */
      vfe.URBEntryAllocationSize = GFX_VER >= 8 ? 2 : 0;

      const uint32_t vfe_curbe_allocation =
         ALIGN(cs_prog_data->push.per_thread.regs * dispatch.threads +
               cs_prog_data->push.cross_thread.regs, 2);
      vfe.CURBEAllocationSize = vfe_curbe_allocation;
   }

   const unsigned push_const_size =
      brw_cs_push_const_total_size(cs_prog_data, dispatch.threads);
   if (push_const_size > 0) {
      brw_batch_emit(brw, GENX(MEDIA_CURBE_LOAD), curbe) {
         curbe.CURBETotalDataLength = ALIGN(push_const_size, 64);
         curbe.CURBEDataStartAddress = stage_state->push_const_offset;
      }
   }

   /* BRW_NEW_SURFACES and BRW_NEW_*_CONSTBUF */
   memcpy(bind, stage_state->surf_offset,
          prog_data->binding_table.size_bytes);
   const uint64_t ksp = brw->cs.base.prog_offset +
                        brw_cs_prog_data_prog_offset(cs_prog_data,
                                                     dispatch.simd_size);
   const struct GENX(INTERFACE_DESCRIPTOR_DATA) idd = {
      .KernelStartPointer = ksp,
      .SamplerStatePointer = stage_state->sampler_offset,
      /* Wa_1606682166 */
      .SamplerCount = GFX_VER == 11 ? 0 :
                      DIV_ROUND_UP(CLAMP(stage_state->sampler_count, 0, 16), 4),
      .BindingTablePointer = stage_state->bind_bo_offset,
      .ConstantURBEntryReadLength = cs_prog_data->push.per_thread.regs,
      .NumberofThreadsinGPGPUThreadGroup = dispatch.threads,
      .SharedLocalMemorySize = encode_slm_size(GFX_VER,
                                               prog_data->total_shared),
      .BarrierEnable = cs_prog_data->uses_barrier,
#if GFX_VERx10 >= 75
      .CrossThreadConstantDataReadLength =
         cs_prog_data->push.cross_thread.regs,
#endif
   };

   GENX(INTERFACE_DESCRIPTOR_DATA_pack)(brw, desc, &idd);

   brw_batch_emit(brw, GENX(MEDIA_INTERFACE_DESCRIPTOR_LOAD), load) {
      load.InterfaceDescriptorTotalLength =
         GENX(INTERFACE_DESCRIPTOR_DATA_length) * sizeof(uint32_t);
      load.InterfaceDescriptorDataStartAddress = offset;
   }
}

static const struct brw_tracked_state genX(cs_state) = {
   .dirty = {
      .mesa = _NEW_PROGRAM_CONSTANTS,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_CS_PROG_DATA |
             BRW_NEW_SAMPLER_STATE_TABLE |
             BRW_NEW_SURFACES,
   },
   .emit = genX(upload_cs_state)
};

#define GPGPU_DISPATCHDIMX 0x2500
#define GPGPU_DISPATCHDIMY 0x2504
#define GPGPU_DISPATCHDIMZ 0x2508

#define MI_PREDICATE_SRC0  0x2400
#define MI_PREDICATE_SRC1  0x2408

static void
prepare_indirect_gpgpu_walker(struct brw_context *brw)
{
   GLintptr indirect_offset = brw->compute.num_work_groups_offset;
   struct brw_bo *bo = brw->compute.num_work_groups_bo;

   emit_lrm(brw, GPGPU_DISPATCHDIMX, ro_bo(bo, indirect_offset + 0));
   emit_lrm(brw, GPGPU_DISPATCHDIMY, ro_bo(bo, indirect_offset + 4));
   emit_lrm(brw, GPGPU_DISPATCHDIMZ, ro_bo(bo, indirect_offset + 8));

#if GFX_VER <= 7
   /* Clear upper 32-bits of SRC0 and all 64-bits of SRC1 */
   emit_lri(brw, MI_PREDICATE_SRC0 + 4, 0);
   emit_lri(brw, MI_PREDICATE_SRC1    , 0);
   emit_lri(brw, MI_PREDICATE_SRC1 + 4, 0);

   /* Load compute_dispatch_indirect_x_size into SRC0 */
   emit_lrm(brw, MI_PREDICATE_SRC0, ro_bo(bo, indirect_offset + 0));

   /* predicate = (compute_dispatch_indirect_x_size == 0); */
   brw_batch_emit(brw, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOAD;
      mip.CombineOperation = COMBINE_SET;
      mip.CompareOperation = COMPARE_SRCS_EQUAL;
   }

   /* Load compute_dispatch_indirect_y_size into SRC0 */
   emit_lrm(brw, MI_PREDICATE_SRC0, ro_bo(bo, indirect_offset + 4));

   /* predicate |= (compute_dispatch_indirect_y_size == 0); */
   brw_batch_emit(brw, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOAD;
      mip.CombineOperation = COMBINE_OR;
      mip.CompareOperation = COMPARE_SRCS_EQUAL;
   }

   /* Load compute_dispatch_indirect_z_size into SRC0 */
   emit_lrm(brw, MI_PREDICATE_SRC0, ro_bo(bo, indirect_offset + 8));

   /* predicate |= (compute_dispatch_indirect_z_size == 0); */
   brw_batch_emit(brw, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOAD;
      mip.CombineOperation = COMBINE_OR;
      mip.CompareOperation = COMPARE_SRCS_EQUAL;
   }

   /* predicate = !predicate; */
#define COMPARE_FALSE                           1
   brw_batch_emit(brw, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOADINV;
      mip.CombineOperation = COMBINE_OR;
      mip.CompareOperation = COMPARE_FALSE;
   }
#endif
}

static void
genX(emit_gpgpu_walker)(struct brw_context *brw)
{
   const GLuint *num_groups = brw->compute.num_work_groups;

   bool indirect = brw->compute.num_work_groups_bo != NULL;
   if (indirect)
      prepare_indirect_gpgpu_walker(brw);

   const struct brw_cs_dispatch_info dispatch =
      brw_cs_get_dispatch_info(&brw->screen->devinfo,
                               brw_cs_prog_data(brw->cs.base.prog_data),
                               brw->compute.group_size);

   brw_batch_emit(brw, GENX(GPGPU_WALKER), ggw) {
      ggw.IndirectParameterEnable      = indirect;
      ggw.PredicateEnable              = GFX_VER <= 7 && indirect;
      ggw.SIMDSize                     = dispatch.simd_size / 16;
      ggw.ThreadDepthCounterMaximum    = 0;
      ggw.ThreadHeightCounterMaximum   = 0;
      ggw.ThreadWidthCounterMaximum    = dispatch.threads - 1;
      ggw.ThreadGroupIDXDimension      = num_groups[0];
      ggw.ThreadGroupIDYDimension      = num_groups[1];
      ggw.ThreadGroupIDZDimension      = num_groups[2];
      ggw.RightExecutionMask           = dispatch.right_mask;
      ggw.BottomExecutionMask          = 0xffffffff;
   }

   brw_batch_emit(brw, GENX(MEDIA_STATE_FLUSH), msf);
}

#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER >= 8
static void
genX(upload_raster)(struct brw_context *brw)
{
   const struct gl_context *ctx = &brw->ctx;

   /* _NEW_BUFFERS */
   const bool flip_y = ctx->DrawBuffer->FlipY;

   /* _NEW_POLYGON */
   const struct gl_polygon_attrib *polygon = &ctx->Polygon;

   /* _NEW_POINT */
   const struct gl_point_attrib *point = &ctx->Point;

   brw_batch_emit(brw, GENX(3DSTATE_RASTER), raster) {
      if (brw->polygon_front_bit != flip_y)
         raster.FrontWinding = CounterClockwise;

      if (polygon->CullFlag) {
         switch (polygon->CullFaceMode) {
         case GL_FRONT:
            raster.CullMode = CULLMODE_FRONT;
            break;
         case GL_BACK:
            raster.CullMode = CULLMODE_BACK;
            break;
         case GL_FRONT_AND_BACK:
            raster.CullMode = CULLMODE_BOTH;
            break;
         default:
            unreachable("not reached");
         }
      } else {
         raster.CullMode = CULLMODE_NONE;
      }

      raster.SmoothPointEnable = point->SmoothFlag;

      raster.DXMultisampleRasterizationEnable =
         _mesa_is_multisample_enabled(ctx);

      raster.GlobalDepthOffsetEnableSolid = polygon->OffsetFill;
      raster.GlobalDepthOffsetEnableWireframe = polygon->OffsetLine;
      raster.GlobalDepthOffsetEnablePoint = polygon->OffsetPoint;

      switch (polygon->FrontMode) {
      case GL_FILL:
         raster.FrontFaceFillMode = FILL_MODE_SOLID;
         break;
      case GL_LINE:
         raster.FrontFaceFillMode = FILL_MODE_WIREFRAME;
         break;
      case GL_POINT:
         raster.FrontFaceFillMode = FILL_MODE_POINT;
         break;
      default:
         unreachable("not reached");
      }

      switch (polygon->BackMode) {
      case GL_FILL:
         raster.BackFaceFillMode = FILL_MODE_SOLID;
         break;
      case GL_LINE:
         raster.BackFaceFillMode = FILL_MODE_WIREFRAME;
         break;
      case GL_POINT:
         raster.BackFaceFillMode = FILL_MODE_POINT;
         break;
      default:
         unreachable("not reached");
      }

      /* _NEW_LINE */
      raster.AntialiasingEnable = ctx->Line.SmoothFlag;

#if GFX_VER == 10
      /* _NEW_BUFFERS
       * Antialiasing Enable bit MUST not be set when NUM_MULTISAMPLES > 1.
       */
      const bool multisampled_fbo =
         _mesa_geometric_samples(ctx->DrawBuffer) > 1;
      if (multisampled_fbo)
         raster.AntialiasingEnable = false;
#endif

      /* _NEW_SCISSOR */
      raster.ScissorRectangleEnable = ctx->Scissor.EnableFlags;

      /* _NEW_TRANSFORM */
#if GFX_VER < 9
      if (!(ctx->Transform.DepthClampNear &&
            ctx->Transform.DepthClampFar))
         raster.ViewportZClipTestEnable = true;
#endif

#if GFX_VER >= 9
      if (!ctx->Transform.DepthClampNear)
         raster.ViewportZNearClipTestEnable = true;

      if (!ctx->Transform.DepthClampFar)
         raster.ViewportZFarClipTestEnable = true;
#endif

      /* BRW_NEW_CONSERVATIVE_RASTERIZATION */
#if GFX_VER >= 9
      raster.ConservativeRasterizationEnable =
         ctx->IntelConservativeRasterization;
#endif

      raster.GlobalDepthOffsetClamp = polygon->OffsetClamp;
      raster.GlobalDepthOffsetScale = polygon->OffsetFactor;

      raster.GlobalDepthOffsetConstant = polygon->OffsetUnits * 2;
   }
}

static const struct brw_tracked_state genX(raster_state) = {
   .dirty = {
      .mesa  = _NEW_BUFFERS |
               _NEW_LINE |
               _NEW_MULTISAMPLE |
               _NEW_POINT |
               _NEW_POLYGON |
               _NEW_SCISSOR |
               _NEW_TRANSFORM,
      .brw   = BRW_NEW_BLORP |
               BRW_NEW_CONTEXT |
               BRW_NEW_CONSERVATIVE_RASTERIZATION,
   },
   .emit = genX(upload_raster),
};
#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER >= 8
static void
genX(upload_ps_extra)(struct brw_context *brw)
{
   UNUSED struct gl_context *ctx = &brw->ctx;

   const struct brw_wm_prog_data *prog_data =
      brw_wm_prog_data(brw->wm.base.prog_data);

   brw_batch_emit(brw, GENX(3DSTATE_PS_EXTRA), psx) {
      psx.PixelShaderValid = true;
      psx.PixelShaderComputedDepthMode = prog_data->computed_depth_mode;
      psx.PixelShaderKillsPixel = prog_data->uses_kill;
      psx.AttributeEnable = prog_data->num_varying_inputs != 0;
      psx.PixelShaderUsesSourceDepth = prog_data->uses_src_depth;
      psx.PixelShaderUsesSourceW = prog_data->uses_src_w;
      psx.PixelShaderIsPerSample = prog_data->persample_dispatch;

      /* _NEW_MULTISAMPLE | BRW_NEW_CONSERVATIVE_RASTERIZATION */
      if (prog_data->uses_sample_mask) {
#if GFX_VER >= 9
         if (prog_data->post_depth_coverage)
            psx.InputCoverageMaskState = ICMS_DEPTH_COVERAGE;
         else if (prog_data->inner_coverage && ctx->IntelConservativeRasterization)
            psx.InputCoverageMaskState = ICMS_INNER_CONSERVATIVE;
         else
            psx.InputCoverageMaskState = ICMS_NORMAL;
#else
         psx.PixelShaderUsesInputCoverageMask = true;
#endif
      }

      psx.oMaskPresenttoRenderTarget = prog_data->uses_omask;
#if GFX_VER >= 9
      psx.PixelShaderPullsBary = prog_data->pulls_bary;
      psx.PixelShaderComputesStencil = prog_data->computed_stencil;
#endif

      /* The stricter cross-primitive coherency guarantees that the hardware
       * gives us with the "Accesses UAV" bit set for at least one shader stage
       * and the "UAV coherency required" bit set on the 3DPRIMITIVE command
       * are redundant within the current image, atomic counter and SSBO GL
       * APIs, which all have very loose ordering and coherency requirements
       * and generally rely on the application to insert explicit barriers when
       * a shader invocation is expected to see the memory writes performed by
       * the invocations of some previous primitive.  Regardless of the value
       * of "UAV coherency required", the "Accesses UAV" bits will implicitly
       * cause an in most cases useless DC flush when the lowermost stage with
       * the bit set finishes execution.
       *
       * It would be nice to disable it, but in some cases we can't because on
       * Gfx8+ it also has an influence on rasterization via the PS UAV-only
       * signal (which could be set independently from the coherency mechanism
       * in the 3DSTATE_WM command on Gfx7), and because in some cases it will
       * determine whether the hardware skips execution of the fragment shader
       * or not via the ThreadDispatchEnable signal.  However if we know that
       * GFX8_PS_BLEND_HAS_WRITEABLE_RT is going to be set and
       * GFX8_PSX_PIXEL_SHADER_NO_RT_WRITE is not set it shouldn't make any
       * difference so we may just disable it here.
       *
       * Gfx8 hardware tries to compute ThreadDispatchEnable for us but doesn't
       * take into account KillPixels when no depth or stencil writes are
       * enabled.  In order for occlusion queries to work correctly with no
       * attachments, we need to force-enable here.
       *
       * BRW_NEW_FS_PROG_DATA | BRW_NEW_FRAGMENT_PROGRAM | _NEW_BUFFERS |
       * _NEW_COLOR
       */
      if ((prog_data->has_side_effects || prog_data->uses_kill) &&
          !brw_color_buffer_write_enabled(brw))
         psx.PixelShaderHasUAV = true;
   }
}

const struct brw_tracked_state genX(ps_extra) = {
   .dirty = {
      .mesa  = _NEW_BUFFERS | _NEW_COLOR,
      .brw   = BRW_NEW_BLORP |
               BRW_NEW_CONTEXT |
               BRW_NEW_FRAGMENT_PROGRAM |
               BRW_NEW_FS_PROG_DATA |
               BRW_NEW_CONSERVATIVE_RASTERIZATION,
   },
   .emit = genX(upload_ps_extra),
};
#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER >= 8
static void
genX(upload_ps_blend)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   /* _NEW_BUFFERS */
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0];
   const bool buffer0_is_integer = ctx->DrawBuffer->_IntegerBuffers & 0x1;

   /* _NEW_COLOR */
   struct gl_colorbuffer_attrib *color = &ctx->Color;

   brw_batch_emit(brw, GENX(3DSTATE_PS_BLEND), pb) {
      /* BRW_NEW_FRAGMENT_PROGRAM | _NEW_BUFFERS | _NEW_COLOR */
      pb.HasWriteableRT = brw_color_buffer_write_enabled(brw);

      bool alpha_to_one = false;

      if (!buffer0_is_integer) {
         /* _NEW_MULTISAMPLE */

         if (_mesa_is_multisample_enabled(ctx)) {
            pb.AlphaToCoverageEnable = ctx->Multisample.SampleAlphaToCoverage;
            alpha_to_one = ctx->Multisample.SampleAlphaToOne;
         }

         pb.AlphaTestEnable = color->AlphaEnabled;
      }

      /* Used for implementing the following bit of GL_EXT_texture_integer:
       * "Per-fragment operations that require floating-point color
       *  components, including multisample alpha operations, alpha test,
       *  blending, and dithering, have no effect when the corresponding
       *  colors are written to an integer color buffer."
       *
       * The OpenGL specification 3.3 (page 196), section 4.1.3 says:
       * "If drawbuffer zero is not NONE and the buffer it references has an
       *  integer format, the SAMPLE_ALPHA_TO_COVERAGE and SAMPLE_ALPHA_TO_ONE
       *  operations are skipped."
       */
      if (rb && !buffer0_is_integer && (color->BlendEnabled & 1)) {
         GLenum eqRGB = color->Blend[0].EquationRGB;
         GLenum eqA = color->Blend[0].EquationA;
         GLenum srcRGB = color->Blend[0].SrcRGB;
         GLenum dstRGB = color->Blend[0].DstRGB;
         GLenum srcA = color->Blend[0].SrcA;
         GLenum dstA = color->Blend[0].DstA;

         if (eqRGB == GL_MIN || eqRGB == GL_MAX)
            srcRGB = dstRGB = GL_ONE;

         if (eqA == GL_MIN || eqA == GL_MAX)
            srcA = dstA = GL_ONE;

         /* Due to hardware limitations, the destination may have information
          * in an alpha channel even when the format specifies no alpha
          * channel. In order to avoid getting any incorrect blending due to
          * that alpha channel, coerce the blend factors to values that will
          * not read the alpha channel, but will instead use the correct
          * implicit value for alpha.
          */
         if (!_mesa_base_format_has_channel(rb->_BaseFormat,
                                            GL_TEXTURE_ALPHA_TYPE)) {
            srcRGB = brw_fix_xRGB_alpha(srcRGB);
            srcA = brw_fix_xRGB_alpha(srcA);
            dstRGB = brw_fix_xRGB_alpha(dstRGB);
            dstA = brw_fix_xRGB_alpha(dstA);
         }

         /* Alpha to One doesn't work with Dual Color Blending.  Override
          * SRC1_ALPHA to ONE and ONE_MINUS_SRC1_ALPHA to ZERO.
          */
         if (alpha_to_one && color->_BlendUsesDualSrc & 0x1) {
            srcRGB = fix_dual_blend_alpha_to_one(srcRGB);
            srcA = fix_dual_blend_alpha_to_one(srcA);
            dstRGB = fix_dual_blend_alpha_to_one(dstRGB);
            dstA = fix_dual_blend_alpha_to_one(dstA);
         }

         /* BRW_NEW_FS_PROG_DATA */
         const struct brw_wm_prog_data *wm_prog_data =
            brw_wm_prog_data(brw->wm.base.prog_data);

         /* The Dual Source Blending documentation says:
          *
          * "If SRC1 is included in a src/dst blend factor and
          * a DualSource RT Write message is not used, results
          * are UNDEFINED. (This reflects the same restriction in DX APIs,
          * where undefined results are produced if “o1” is not written
          * by a PS – there are no default values defined).
          * If SRC1 is not included in a src/dst blend factor,
          * dual source blending must be disabled."
          *
          * There is no way to gracefully fix this undefined situation
          * so we just disable the blending to prevent possible issues.
          */
         pb.ColorBufferBlendEnable =
            !(color->_BlendUsesDualSrc & 0x1) || wm_prog_data->dual_src_blend;
         pb.SourceAlphaBlendFactor = brw_translate_blend_factor(srcA);
         pb.DestinationAlphaBlendFactor = brw_translate_blend_factor(dstA);
         pb.SourceBlendFactor = brw_translate_blend_factor(srcRGB);
         pb.DestinationBlendFactor = brw_translate_blend_factor(dstRGB);

         pb.IndependentAlphaBlendEnable =
            srcA != srcRGB || dstA != dstRGB || eqA != eqRGB;
      }
   }
}

static const struct brw_tracked_state genX(ps_blend) = {
   .dirty = {
      .mesa = _NEW_BUFFERS |
              _NEW_COLOR |
              _NEW_MULTISAMPLE,
      .brw = BRW_NEW_BLORP |
             BRW_NEW_CONTEXT |
             BRW_NEW_FRAGMENT_PROGRAM |
             BRW_NEW_FS_PROG_DATA,
   },
   .emit = genX(upload_ps_blend)
};
#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER >= 8
static void
genX(emit_vf_topology)(struct brw_context *brw)
{
   brw_batch_emit(brw, GENX(3DSTATE_VF_TOPOLOGY), vftopo) {
      vftopo.PrimitiveTopologyType = brw->primitive;
   }
}

static const struct brw_tracked_state genX(vf_topology) = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BLORP |
             BRW_NEW_PRIMITIVE,
   },
   .emit = genX(emit_vf_topology),
};
#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER >= 7
static void
genX(emit_mi_report_perf_count)(struct brw_context *brw,
                                struct brw_bo *bo,
                                uint32_t offset_in_bytes,
                                uint32_t report_id)
{
   brw_batch_emit(brw, GENX(MI_REPORT_PERF_COUNT), mi_rpc) {
      mi_rpc.MemoryAddress = ggtt_bo(bo, offset_in_bytes);
      mi_rpc.ReportID = report_id;
   }
}
#endif

/* ---------------------------------------------------------------------- */

/**
 * Emit a 3DSTATE_SAMPLER_STATE_POINTERS_{VS,HS,GS,DS,PS} packet.
 */
static void
genX(emit_sampler_state_pointers_xs)(UNUSED struct brw_context *brw,
                                     UNUSED struct brw_stage_state *stage_state)
{
#if GFX_VER >= 7
   static const uint16_t packet_headers[] = {
      [MESA_SHADER_VERTEX] = 43,
      [MESA_SHADER_TESS_CTRL] = 44,
      [MESA_SHADER_TESS_EVAL] = 45,
      [MESA_SHADER_GEOMETRY] = 46,
      [MESA_SHADER_FRAGMENT] = 47,
   };

   /* Ivybridge requires a workaround flush before VS packets. */
   if (GFX_VERx10 == 70 &&
       stage_state->stage == MESA_SHADER_VERTEX) {
      gfx7_emit_vs_workaround_flush(brw);
   }

   brw_batch_emit(brw, GENX(3DSTATE_SAMPLER_STATE_POINTERS_VS), ptr) {
      ptr._3DCommandSubOpcode = packet_headers[stage_state->stage];
      ptr.PointertoVSSamplerState = stage_state->sampler_offset;
   }
#endif
}

UNUSED static bool
has_component(mesa_format format, int i)
{
   if (_mesa_is_format_color_format(format))
      return _mesa_format_has_color_component(format, i);

   /* depth and stencil have only one component */
   return i == 0;
}

/**
 * Upload SAMPLER_BORDER_COLOR_STATE.
 */
static void
genX(upload_default_color)(struct brw_context *brw,
                           const struct gl_sampler_object *sampler,
                           UNUSED mesa_format format,
                           GLenum base_format,
                           bool is_integer_format, bool is_stencil_sampling,
                           uint32_t *sdc_offset)
{
   union gl_color_union color;

   switch (base_format) {
   case GL_DEPTH_COMPONENT:
      /* GL specs that border color for depth textures is taken from the
       * R channel, while the hardware uses A.  Spam R into all the
       * channels for safety.
       */
      color.ui[0] = sampler->Attrib.state.border_color.ui[0];
      color.ui[1] = sampler->Attrib.state.border_color.ui[0];
      color.ui[2] = sampler->Attrib.state.border_color.ui[0];
      color.ui[3] = sampler->Attrib.state.border_color.ui[0];
      break;
   case GL_ALPHA:
      color.ui[0] = 0u;
      color.ui[1] = 0u;
      color.ui[2] = 0u;
      color.ui[3] = sampler->Attrib.state.border_color.ui[3];
      break;
   case GL_INTENSITY:
      color.ui[0] = sampler->Attrib.state.border_color.ui[0];
      color.ui[1] = sampler->Attrib.state.border_color.ui[0];
      color.ui[2] = sampler->Attrib.state.border_color.ui[0];
      color.ui[3] = sampler->Attrib.state.border_color.ui[0];
      break;
   case GL_LUMINANCE:
      color.ui[0] = sampler->Attrib.state.border_color.ui[0];
      color.ui[1] = sampler->Attrib.state.border_color.ui[0];
      color.ui[2] = sampler->Attrib.state.border_color.ui[0];
      color.ui[3] = float_as_int(1.0);
      break;
   case GL_LUMINANCE_ALPHA:
      color.ui[0] = sampler->Attrib.state.border_color.ui[0];
      color.ui[1] = sampler->Attrib.state.border_color.ui[0];
      color.ui[2] = sampler->Attrib.state.border_color.ui[0];
      color.ui[3] = sampler->Attrib.state.border_color.ui[3];
      break;
   default:
      color.ui[0] = sampler->Attrib.state.border_color.ui[0];
      color.ui[1] = sampler->Attrib.state.border_color.ui[1];
      color.ui[2] = sampler->Attrib.state.border_color.ui[2];
      color.ui[3] = sampler->Attrib.state.border_color.ui[3];
      break;
   }

   /* In some cases we use an RGBA surface format for GL RGB textures,
    * where we've initialized the A channel to 1.0.  We also have to set
    * the border color alpha to 1.0 in that case.
    */
   if (base_format == GL_RGB)
      color.ui[3] = float_as_int(1.0);

   int alignment = 32;
   if (GFX_VER >= 8) {
      alignment = 64;
   } else if (GFX_VERx10 == 75 && (is_integer_format || is_stencil_sampling)) {
      alignment = 512;
   }

   uint32_t *sdc = brw_state_batch(
      brw, GENX(SAMPLER_BORDER_COLOR_STATE_length) * sizeof(uint32_t),
      alignment, sdc_offset);

   struct GENX(SAMPLER_BORDER_COLOR_STATE) state = { 0 };

#define ASSIGN(dst, src) \
   do {                  \
      dst = src;         \
   } while (0)

#define ASSIGNu16(dst, src) \
   do {                     \
      dst = (uint16_t)src;  \
   } while (0)

#define ASSIGNu8(dst, src) \
   do {                    \
      dst = (uint8_t)src;  \
   } while (0)

#define BORDER_COLOR_ATTR(macro, _color_type, src)              \
   macro(state.BorderColor ## _color_type ## Red, src[0]);   \
   macro(state.BorderColor ## _color_type ## Green, src[1]);   \
   macro(state.BorderColor ## _color_type ## Blue, src[2]);   \
   macro(state.BorderColor ## _color_type ## Alpha, src[3]);

#if GFX_VER >= 8
   /* On Broadwell, the border color is represented as four 32-bit floats,
    * integers, or unsigned values, interpreted according to the surface
    * format.  This matches the sampler->BorderColor union exactly; just
    * memcpy the values.
    */
   BORDER_COLOR_ATTR(ASSIGN, 32bit, color.ui);
#elif GFX_VERx10 == 75
   if (is_integer_format || is_stencil_sampling) {
      bool stencil = format == MESA_FORMAT_S_UINT8 || is_stencil_sampling;
      const int bits_per_channel =
         _mesa_get_format_bits(format, stencil ? GL_STENCIL_BITS : GL_RED_BITS);

      /* From the Haswell PRM, "Command Reference: Structures", Page 36:
       * "If any color channel is missing from the surface format,
       *  corresponding border color should be programmed as zero and if
       *  alpha channel is missing, corresponding Alpha border color should
       *  be programmed as 1."
       */
      unsigned c[4] = { 0, 0, 0, 1 };
      for (int i = 0; i < 4; i++) {
         if (has_component(format, i))
            c[i] = color.ui[i];
      }

      switch (bits_per_channel) {
      case 8:
         /* Copy RGBA in order. */
         BORDER_COLOR_ATTR(ASSIGNu8, 8bit, c);
         break;
      case 10:
         /* R10G10B10A2_UINT is treated like a 16-bit format. */
      case 16:
         BORDER_COLOR_ATTR(ASSIGNu16, 16bit, c);
         break;
      case 32:
         if (base_format == GL_RG) {
            /* Careful inspection of the tables reveals that for RG32 formats,
             * the green channel needs to go where blue normally belongs.
             */
            state.BorderColor32bitRed = c[0];
            state.BorderColor32bitBlue = c[1];
            state.BorderColor32bitAlpha = 1;
         } else {
            /* Copy RGBA in order. */
            BORDER_COLOR_ATTR(ASSIGN, 32bit, c);
         }
         break;
      default:
         assert(!"Invalid number of bits per channel in integer format.");
         break;
      }
   } else {
      BORDER_COLOR_ATTR(ASSIGN, Float, color.f);
   }
#elif GFX_VER == 5 || GFX_VER == 6
   BORDER_COLOR_ATTR(UNCLAMPED_FLOAT_TO_UBYTE, Unorm, color.f);
   BORDER_COLOR_ATTR(UNCLAMPED_FLOAT_TO_USHORT, Unorm16, color.f);
   BORDER_COLOR_ATTR(UNCLAMPED_FLOAT_TO_SHORT, Snorm16, color.f);

#define MESA_FLOAT_TO_HALF(dst, src) \
   dst = _mesa_float_to_half(src);

   BORDER_COLOR_ATTR(MESA_FLOAT_TO_HALF, Float16, color.f);

#undef MESA_FLOAT_TO_HALF

   state.BorderColorSnorm8Red   = state.BorderColorSnorm16Red >> 8;
   state.BorderColorSnorm8Green = state.BorderColorSnorm16Green >> 8;
   state.BorderColorSnorm8Blue  = state.BorderColorSnorm16Blue >> 8;
   state.BorderColorSnorm8Alpha = state.BorderColorSnorm16Alpha >> 8;

   BORDER_COLOR_ATTR(ASSIGN, Float, color.f);
#elif GFX_VER == 4
   BORDER_COLOR_ATTR(ASSIGN, , color.f);
#else
   BORDER_COLOR_ATTR(ASSIGN, Float, color.f);
#endif

#undef ASSIGN
#undef BORDER_COLOR_ATTR

   GENX(SAMPLER_BORDER_COLOR_STATE_pack)(brw, sdc, &state);
}

static uint32_t
translate_wrap_mode(GLenum wrap, UNUSED bool using_nearest)
{
   switch (wrap) {
   case GL_REPEAT:
      return TCM_WRAP;
   case GL_CLAMP:
#if GFX_VER >= 8
      /* GL_CLAMP is the weird mode where coordinates are clamped to
       * [0.0, 1.0], so linear filtering of coordinates outside of
       * [0.0, 1.0] give you half edge texel value and half border
       * color.
       *
       * Gfx8+ supports this natively.
       */
      return TCM_HALF_BORDER;
#else
      /* On Gfx4-7.5, we clamp the coordinates in the fragment shader
       * and set clamp_border here, which gets the result desired.
       * We just use clamp(_to_edge) for nearest, because for nearest
       * clamping to 1.0 gives border color instead of the desired
       * edge texels.
       */
      if (using_nearest)
         return TCM_CLAMP;
      else
         return TCM_CLAMP_BORDER;
#endif
   case GL_CLAMP_TO_EDGE:
      return TCM_CLAMP;
   case GL_CLAMP_TO_BORDER:
      return TCM_CLAMP_BORDER;
   case GL_MIRRORED_REPEAT:
      return TCM_MIRROR;
   case GL_MIRROR_CLAMP_TO_EDGE:
      return TCM_MIRROR_ONCE;
   default:
      return TCM_WRAP;
   }
}

/**
 * Return true if the given wrap mode requires the border color to exist.
 */
static bool
wrap_mode_needs_border_color(unsigned wrap_mode)
{
#if GFX_VER >= 8
   return wrap_mode == TCM_CLAMP_BORDER ||
          wrap_mode == TCM_HALF_BORDER;
#else
   return wrap_mode == TCM_CLAMP_BORDER;
#endif
}

/**
 * Sets the sampler state for a single unit based off of the sampler key
 * entry.
 */
static void
genX(update_sampler_state)(struct brw_context *brw,
                           GLenum target, bool tex_cube_map_seamless,
                           GLfloat tex_unit_lod_bias,
                           mesa_format format, GLenum base_format,
                           const struct gl_texture_object *texObj,
                           const struct gl_sampler_object *sampler,
                           uint32_t *sampler_state)
{
   struct GENX(SAMPLER_STATE) samp_st = { 0 };

   /* Select min and mip filters. */
   switch (sampler->Attrib.MinFilter) {
   case GL_NEAREST:
      samp_st.MinModeFilter = MAPFILTER_NEAREST;
      samp_st.MipModeFilter = MIPFILTER_NONE;
      break;
   case GL_LINEAR:
      samp_st.MinModeFilter = MAPFILTER_LINEAR;
      samp_st.MipModeFilter = MIPFILTER_NONE;
      break;
   case GL_NEAREST_MIPMAP_NEAREST:
      samp_st.MinModeFilter = MAPFILTER_NEAREST;
      samp_st.MipModeFilter = MIPFILTER_NEAREST;
      break;
   case GL_LINEAR_MIPMAP_NEAREST:
      samp_st.MinModeFilter = MAPFILTER_LINEAR;
      samp_st.MipModeFilter = MIPFILTER_NEAREST;
      break;
   case GL_NEAREST_MIPMAP_LINEAR:
      samp_st.MinModeFilter = MAPFILTER_NEAREST;
      samp_st.MipModeFilter = MIPFILTER_LINEAR;
      break;
   case GL_LINEAR_MIPMAP_LINEAR:
      samp_st.MinModeFilter = MAPFILTER_LINEAR;
      samp_st.MipModeFilter = MIPFILTER_LINEAR;
      break;
   default:
      unreachable("not reached");
   }

   /* Select mag filter. */
   samp_st.MagModeFilter = sampler->Attrib.MagFilter == GL_LINEAR ?
      MAPFILTER_LINEAR : MAPFILTER_NEAREST;

   /* Enable anisotropic filtering if desired. */
   samp_st.MaximumAnisotropy = RATIO21;

   if (sampler->Attrib.MaxAnisotropy > 1.0f) {
      if (samp_st.MinModeFilter == MAPFILTER_LINEAR)
         samp_st.MinModeFilter = MAPFILTER_ANISOTROPIC;
      if (samp_st.MagModeFilter == MAPFILTER_LINEAR)
         samp_st.MagModeFilter = MAPFILTER_ANISOTROPIC;

      if (sampler->Attrib.MaxAnisotropy > 2.0f) {
         samp_st.MaximumAnisotropy =
            MIN2((sampler->Attrib.MaxAnisotropy - 2) / 2, RATIO161);
      }
   }

   /* Set address rounding bits if not using nearest filtering. */
   if (samp_st.MinModeFilter != MAPFILTER_NEAREST) {
      samp_st.UAddressMinFilterRoundingEnable = true;
      samp_st.VAddressMinFilterRoundingEnable = true;
      samp_st.RAddressMinFilterRoundingEnable = true;
   }

   if (samp_st.MagModeFilter != MAPFILTER_NEAREST) {
      samp_st.UAddressMagFilterRoundingEnable = true;
      samp_st.VAddressMagFilterRoundingEnable = true;
      samp_st.RAddressMagFilterRoundingEnable = true;
   }

   bool either_nearest =
      sampler->Attrib.MinFilter == GL_NEAREST || sampler->Attrib.MagFilter == GL_NEAREST;
   unsigned wrap_s = translate_wrap_mode(sampler->Attrib.WrapS, either_nearest);
   unsigned wrap_t = translate_wrap_mode(sampler->Attrib.WrapT, either_nearest);
   unsigned wrap_r = translate_wrap_mode(sampler->Attrib.WrapR, either_nearest);

   if (target == GL_TEXTURE_CUBE_MAP ||
       target == GL_TEXTURE_CUBE_MAP_ARRAY) {
      /* Cube maps must use the same wrap mode for all three coordinate
       * dimensions.  Prior to Haswell, only CUBE and CLAMP are valid.
       *
       * Ivybridge and Baytrail seem to have problems with CUBE mode and
       * integer formats.  Fall back to CLAMP for now.
       */
      if ((tex_cube_map_seamless || sampler->Attrib.CubeMapSeamless) &&
          !(GFX_VERx10 == 70 && texObj->_IsIntegerFormat)) {
         wrap_s = TCM_CUBE;
         wrap_t = TCM_CUBE;
         wrap_r = TCM_CUBE;
      } else {
         wrap_s = TCM_CLAMP;
         wrap_t = TCM_CLAMP;
         wrap_r = TCM_CLAMP;
      }
   } else if (target == GL_TEXTURE_1D) {
      /* There's a bug in 1D texture sampling - it actually pays
       * attention to the wrap_t value, though it should not.
       * Override the wrap_t value here to GL_REPEAT to keep
       * any nonexistent border pixels from floating in.
       */
      wrap_t = TCM_WRAP;
   }

   samp_st.TCXAddressControlMode = wrap_s;
   samp_st.TCYAddressControlMode = wrap_t;
   samp_st.TCZAddressControlMode = wrap_r;

   samp_st.ShadowFunction =
      sampler->Attrib.CompareMode == GL_COMPARE_R_TO_TEXTURE_ARB ?
      brw_translate_shadow_compare_func(sampler->Attrib.CompareFunc) : 0;

#if GFX_VER >= 7
   /* Set shadow function. */
   samp_st.AnisotropicAlgorithm =
      samp_st.MinModeFilter == MAPFILTER_ANISOTROPIC ?
      EWAApproximation : LEGACY;
#endif

#if GFX_VER >= 6
   samp_st.NonnormalizedCoordinateEnable = target == GL_TEXTURE_RECTANGLE;
#endif

   const float hw_max_lod = GFX_VER >= 7 ? 14 : 13;
   samp_st.MinLOD = CLAMP(sampler->Attrib.MinLod, 0, hw_max_lod);
   samp_st.MaxLOD = CLAMP(sampler->Attrib.MaxLod, 0, hw_max_lod);
   samp_st.TextureLODBias =
      CLAMP(tex_unit_lod_bias + sampler->Attrib.LodBias, -16, 15);

#if GFX_VER == 6
   samp_st.BaseMipLevel =
      CLAMP(texObj->Attrib.MinLevel + texObj->Attrib.BaseLevel, 0, hw_max_lod);
   samp_st.MinandMagStateNotEqual =
      samp_st.MinModeFilter != samp_st.MagModeFilter;
#endif

   /* Upload the border color if necessary.  If not, just point it at
    * offset 0 (the start of the batch) - the color should be ignored,
    * but that address won't fault in case something reads it anyway.
    */
   uint32_t border_color_offset = 0;
   if (wrap_mode_needs_border_color(wrap_s) ||
       wrap_mode_needs_border_color(wrap_t) ||
       wrap_mode_needs_border_color(wrap_r)) {
      genX(upload_default_color)(brw, sampler, format, base_format,
                                 texObj->_IsIntegerFormat,
                                 texObj->StencilSampling,
                                 &border_color_offset);
   }
#if GFX_VER < 6
      samp_st.BorderColorPointer =
         ro_bo(brw->batch.state.bo, border_color_offset);
#else
      samp_st.BorderColorPointer = border_color_offset;
#endif

#if GFX_VER >= 8
   samp_st.LODPreClampMode = CLAMP_MODE_OGL;
#else
   samp_st.LODPreClampEnable = true;
#endif

   GENX(SAMPLER_STATE_pack)(brw, sampler_state, &samp_st);
}

static void
update_sampler_state(struct brw_context *brw,
                     int unit,
                     uint32_t *sampler_state)
{
   struct gl_context *ctx = &brw->ctx;
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   const struct gl_texture_object *texObj = texUnit->_Current;
   const struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit);

   /* These don't use samplers at all. */
   if (texObj->Target == GL_TEXTURE_BUFFER)
      return;

   struct gl_texture_image *firstImage = texObj->Image[0][texObj->Attrib.BaseLevel];
   genX(update_sampler_state)(brw, texObj->Target,
                              ctx->Texture.CubeMapSeamless,
                              texUnit->LodBias,
                              firstImage->TexFormat, firstImage->_BaseFormat,
                              texObj, sampler,
                              sampler_state);
}

static void
genX(upload_sampler_state_table)(struct brw_context *brw,
                                 struct gl_program *prog,
                                 struct brw_stage_state *stage_state)
{
   struct gl_context *ctx = &brw->ctx;
   uint32_t sampler_count = stage_state->sampler_count;

   GLbitfield SamplersUsed = prog->SamplersUsed;

   if (sampler_count == 0)
      return;

   /* SAMPLER_STATE is 4 DWords on all platforms. */
   const int dwords = GENX(SAMPLER_STATE_length);
   const int size_in_bytes = dwords * sizeof(uint32_t);

   uint32_t *sampler_state = brw_state_batch(brw,
                                             sampler_count * size_in_bytes,
                                             32, &stage_state->sampler_offset);
   /* memset(sampler_state, 0, sampler_count * size_in_bytes); */

   for (unsigned s = 0; s < sampler_count; s++) {
      if (SamplersUsed & (1 << s)) {
         const unsigned unit = prog->SamplerUnits[s];
         if (ctx->Texture.Unit[unit]._Current) {
            update_sampler_state(brw, unit, sampler_state);
         }
      }

      sampler_state += dwords;
   }

   if (GFX_VER >= 7 && stage_state->stage != MESA_SHADER_COMPUTE) {
      /* Emit a 3DSTATE_SAMPLER_STATE_POINTERS_XS packet. */
      genX(emit_sampler_state_pointers_xs)(brw, stage_state);
   } else {
      /* Flag that the sampler state table pointer has changed; later atoms
       * will handle it.
       */
      brw->ctx.NewDriverState |= BRW_NEW_SAMPLER_STATE_TABLE;
   }
}

static void
genX(upload_fs_samplers)(struct brw_context *brw)
{
   /* BRW_NEW_FRAGMENT_PROGRAM */
   struct gl_program *fs = brw->programs[MESA_SHADER_FRAGMENT];
   genX(upload_sampler_state_table)(brw, fs, &brw->wm.base);
}

static const struct brw_tracked_state genX(fs_samplers) = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_FRAGMENT_PROGRAM,
   },
   .emit = genX(upload_fs_samplers),
};

static void
genX(upload_vs_samplers)(struct brw_context *brw)
{
   /* BRW_NEW_VERTEX_PROGRAM */
   struct gl_program *vs = brw->programs[MESA_SHADER_VERTEX];
   genX(upload_sampler_state_table)(brw, vs, &brw->vs.base);
}

static const struct brw_tracked_state genX(vs_samplers) = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_VERTEX_PROGRAM,
   },
   .emit = genX(upload_vs_samplers),
};

#if GFX_VER >= 6
static void
genX(upload_gs_samplers)(struct brw_context *brw)
{
   /* BRW_NEW_GEOMETRY_PROGRAM */
   struct gl_program *gs = brw->programs[MESA_SHADER_GEOMETRY];
   if (!gs)
      return;

   genX(upload_sampler_state_table)(brw, gs, &brw->gs.base);
}


static const struct brw_tracked_state genX(gs_samplers) = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_GEOMETRY_PROGRAM,
   },
   .emit = genX(upload_gs_samplers),
};
#endif

#if GFX_VER >= 7
static void
genX(upload_tcs_samplers)(struct brw_context *brw)
{
   /* BRW_NEW_TESS_PROGRAMS */
   struct gl_program *tcs = brw->programs[MESA_SHADER_TESS_CTRL];
   if (!tcs)
      return;

   genX(upload_sampler_state_table)(brw, tcs, &brw->tcs.base);
}

static const struct brw_tracked_state genX(tcs_samplers) = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_TESS_PROGRAMS,
   },
   .emit = genX(upload_tcs_samplers),
};
#endif

#if GFX_VER >= 7
static void
genX(upload_tes_samplers)(struct brw_context *brw)
{
   /* BRW_NEW_TESS_PROGRAMS */
   struct gl_program *tes = brw->programs[MESA_SHADER_TESS_EVAL];
   if (!tes)
      return;

   genX(upload_sampler_state_table)(brw, tes, &brw->tes.base);
}

static const struct brw_tracked_state genX(tes_samplers) = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_TESS_PROGRAMS,
   },
   .emit = genX(upload_tes_samplers),
};
#endif

#if GFX_VER >= 7
static void
genX(upload_cs_samplers)(struct brw_context *brw)
{
   /* BRW_NEW_COMPUTE_PROGRAM */
   struct gl_program *cs = brw->programs[MESA_SHADER_COMPUTE];
   if (!cs)
      return;

   genX(upload_sampler_state_table)(brw, cs, &brw->cs.base);
}

const struct brw_tracked_state genX(cs_samplers) = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_COMPUTE_PROGRAM,
   },
   .emit = genX(upload_cs_samplers),
};
#endif

/* ---------------------------------------------------------------------- */

#if GFX_VER <= 5

static void genX(upload_blend_constant_color)(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   brw_batch_emit(brw, GENX(3DSTATE_CONSTANT_COLOR), blend_cc) {
      blend_cc.BlendConstantColorRed = ctx->Color.BlendColorUnclamped[0];
      blend_cc.BlendConstantColorGreen = ctx->Color.BlendColorUnclamped[1];
      blend_cc.BlendConstantColorBlue = ctx->Color.BlendColorUnclamped[2];
      blend_cc.BlendConstantColorAlpha = ctx->Color.BlendColorUnclamped[3];
   }
}

static const struct brw_tracked_state genX(blend_constant_color) = {
   .dirty = {
      .mesa = _NEW_COLOR,
      .brw = BRW_NEW_CONTEXT |
             BRW_NEW_BLORP,
   },
   .emit = genX(upload_blend_constant_color)
};
#endif

/* ---------------------------------------------------------------------- */

void
genX(init_atoms)(struct brw_context *brw)
{
#if GFX_VER < 6
   static const struct brw_tracked_state *render_atoms[] =
   {
      &genX(vf_statistics),

      /* Once all the programs are done, we know how large urb entry
       * sizes need to be and can decide if we need to change the urb
       * layout.
       */
      &brw_curbe_offsets,
      &brw_recalculate_urb_fence,

      &genX(cc_vp),
      &genX(color_calc_state),

      /* Surface state setup.  Must come before the VS/WM unit.  The binding
       * table upload must be last.
       */
      &brw_vs_pull_constants,
      &brw_wm_pull_constants,
      &brw_renderbuffer_surfaces,
      &brw_renderbuffer_read_surfaces,
      &brw_texture_surfaces,
      &brw_vs_binding_table,
      &brw_wm_binding_table,

      &genX(fs_samplers),
      &genX(vs_samplers),

      /* These set up state for brw_psp_urb_cbs */
      &genX(wm_state),
      &genX(sf_clip_viewport),
      &genX(sf_state),
      &genX(vs_state), /* always required, enabled or not */
      &genX(clip_state),
      &genX(gs_state),

      /* Command packets:
       */
      &brw_binding_table_pointers,
      &genX(blend_constant_color),

      &brw_depthbuffer,

      &genX(polygon_stipple),
      &genX(polygon_stipple_offset),

      &genX(line_stipple),

      &brw_psp_urb_cbs,

      &genX(drawing_rect),
      &brw_indices, /* must come before brw_vertices */
      &genX(index_buffer),
      &genX(vertices),

      &brw_constant_buffer
   };
#elif GFX_VER == 6
   static const struct brw_tracked_state *render_atoms[] =
   {
      &genX(vf_statistics),

      &genX(sf_clip_viewport),

      /* Command packets: */

      &genX(cc_vp),

      &gfx6_urb,
      &genX(blend_state),         /* must do before cc unit */
      &genX(color_calc_state),    /* must do before cc unit */
      &genX(depth_stencil_state), /* must do before cc unit */

      &genX(vs_push_constants), /* Before vs_state */
      &genX(gs_push_constants), /* Before gs_state */
      &genX(wm_push_constants), /* Before wm_state */

      /* Surface state setup.  Must come before the VS/WM unit.  The binding
       * table upload must be last.
       */
      &brw_vs_pull_constants,
      &brw_vs_ubo_surfaces,
      &brw_gs_pull_constants,
      &brw_gs_ubo_surfaces,
      &brw_wm_pull_constants,
      &brw_wm_ubo_surfaces,
      &gfx6_renderbuffer_surfaces,
      &brw_renderbuffer_read_surfaces,
      &brw_texture_surfaces,
      &gfx6_sol_surface,
      &brw_vs_binding_table,
      &gfx6_gs_binding_table,
      &brw_wm_binding_table,

      &genX(fs_samplers),
      &genX(vs_samplers),
      &genX(gs_samplers),
      &gfx6_sampler_state,
      &genX(multisample_state),

      &genX(vs_state),
      &genX(gs_state),
      &genX(clip_state),
      &genX(sf_state),
      &genX(wm_state),

      &genX(scissor_state),

      &gfx6_binding_table_pointers,

      &brw_depthbuffer,

      &genX(polygon_stipple),
      &genX(polygon_stipple_offset),

      &genX(line_stipple),

      &genX(drawing_rect),

      &brw_indices, /* must come before brw_vertices */
      &genX(index_buffer),
      &genX(vertices),
   };
#elif GFX_VER == 7
   static const struct brw_tracked_state *render_atoms[] =
   {
      &genX(vf_statistics),

      /* Command packets: */

      &genX(cc_vp),
      &genX(sf_clip_viewport),

      &gfx7_l3_state,
      &gfx7_push_constant_space,
      &gfx7_urb,
#if GFX_VERx10 == 75
      &genX(cc_and_blend_state),
#else
      &genX(blend_state),         /* must do before cc unit */
      &genX(color_calc_state),    /* must do before cc unit */
#endif
      &genX(depth_stencil_state), /* must do before cc unit */

      &brw_vs_image_surfaces, /* Before vs push/pull constants and binding table */
      &brw_tcs_image_surfaces, /* Before tcs push/pull constants and binding table */
      &brw_tes_image_surfaces, /* Before tes push/pull constants and binding table */
      &brw_gs_image_surfaces, /* Before gs push/pull constants and binding table */
      &brw_wm_image_surfaces, /* Before wm push/pull constants and binding table */

      &genX(vs_push_constants), /* Before vs_state */
      &genX(tcs_push_constants),
      &genX(tes_push_constants),
      &genX(gs_push_constants), /* Before gs_state */
      &genX(wm_push_constants), /* Before wm_surfaces and constant_buffer */

      /* Surface state setup.  Must come before the VS/WM unit.  The binding
       * table upload must be last.
       */
      &brw_vs_pull_constants,
      &brw_vs_ubo_surfaces,
      &brw_tcs_pull_constants,
      &brw_tcs_ubo_surfaces,
      &brw_tes_pull_constants,
      &brw_tes_ubo_surfaces,
      &brw_gs_pull_constants,
      &brw_gs_ubo_surfaces,
      &brw_wm_pull_constants,
      &brw_wm_ubo_surfaces,
      &gfx6_renderbuffer_surfaces,
      &brw_renderbuffer_read_surfaces,
      &brw_texture_surfaces,

      &genX(push_constant_packets),

      &brw_vs_binding_table,
      &brw_tcs_binding_table,
      &brw_tes_binding_table,
      &brw_gs_binding_table,
      &brw_wm_binding_table,

      &genX(fs_samplers),
      &genX(vs_samplers),
      &genX(tcs_samplers),
      &genX(tes_samplers),
      &genX(gs_samplers),
      &genX(multisample_state),

      &genX(vs_state),
      &genX(hs_state),
      &genX(te_state),
      &genX(ds_state),
      &genX(gs_state),
      &genX(sol_state),
      &genX(clip_state),
      &genX(sbe_state),
      &genX(sf_state),
      &genX(wm_state),
      &genX(ps_state),

      &genX(scissor_state),

      &brw_depthbuffer,

      &genX(polygon_stipple),
      &genX(polygon_stipple_offset),

      &genX(line_stipple),

      &genX(drawing_rect),

      &brw_indices, /* must come before brw_vertices */
      &genX(index_buffer),
      &genX(vertices),

#if GFX_VERx10 == 75
      &genX(cut_index),
#endif
   };
#elif GFX_VER >= 8
   static const struct brw_tracked_state *render_atoms[] =
   {
      &genX(vf_statistics),

      &genX(cc_vp),
      &genX(sf_clip_viewport),

      &gfx7_l3_state,
      &gfx7_push_constant_space,
      &gfx7_urb,
      &genX(blend_state),
      &genX(color_calc_state),

      &brw_vs_image_surfaces, /* Before vs push/pull constants and binding table */
      &brw_tcs_image_surfaces, /* Before tcs push/pull constants and binding table */
      &brw_tes_image_surfaces, /* Before tes push/pull constants and binding table */
      &brw_gs_image_surfaces, /* Before gs push/pull constants and binding table */
      &brw_wm_image_surfaces, /* Before wm push/pull constants and binding table */

      &genX(vs_push_constants), /* Before vs_state */
      &genX(tcs_push_constants),
      &genX(tes_push_constants),
      &genX(gs_push_constants), /* Before gs_state */
      &genX(wm_push_constants), /* Before wm_surfaces and constant_buffer */

      /* Surface state setup.  Must come before the VS/WM unit.  The binding
       * table upload must be last.
       */
      &brw_vs_pull_constants,
      &brw_vs_ubo_surfaces,
      &brw_tcs_pull_constants,
      &brw_tcs_ubo_surfaces,
      &brw_tes_pull_constants,
      &brw_tes_ubo_surfaces,
      &brw_gs_pull_constants,
      &brw_gs_ubo_surfaces,
      &brw_wm_pull_constants,
      &brw_wm_ubo_surfaces,
      &gfx6_renderbuffer_surfaces,
      &brw_renderbuffer_read_surfaces,
      &brw_texture_surfaces,

      &genX(push_constant_packets),

      &brw_vs_binding_table,
      &brw_tcs_binding_table,
      &brw_tes_binding_table,
      &brw_gs_binding_table,
      &brw_wm_binding_table,

      &genX(fs_samplers),
      &genX(vs_samplers),
      &genX(tcs_samplers),
      &genX(tes_samplers),
      &genX(gs_samplers),
      &genX(multisample_state),

      &genX(vs_state),
      &genX(hs_state),
      &genX(te_state),
      &genX(ds_state),
      &genX(gs_state),
      &genX(sol_state),
      &genX(clip_state),
      &genX(raster_state),
      &genX(sbe_state),
      &genX(sf_state),
      &genX(ps_blend),
      &genX(ps_extra),
      &genX(ps_state),
      &genX(depth_stencil_state),
      &genX(wm_state),

      &genX(scissor_state),

      &brw_depthbuffer,

      &genX(polygon_stipple),
      &genX(polygon_stipple_offset),

      &genX(line_stipple),

      &genX(drawing_rect),

      &genX(vf_topology),

      &brw_indices,
      &genX(index_buffer),
      &genX(vertices),

      &genX(cut_index),
      &gfx8_pma_fix,
   };
#endif

   STATIC_ASSERT(ARRAY_SIZE(render_atoms) <= ARRAY_SIZE(brw->render_atoms));
   brw_copy_pipeline_atoms(brw, BRW_RENDER_PIPELINE,
                           render_atoms, ARRAY_SIZE(render_atoms));

#if GFX_VER >= 7
   static const struct brw_tracked_state *compute_atoms[] =
   {
      &gfx7_l3_state,
      &brw_cs_image_surfaces,
      &genX(cs_push_constants),
      &genX(cs_pull_constants),
      &brw_cs_ubo_surfaces,
      &brw_cs_texture_surfaces,
      &brw_cs_work_groups_surface,
      &genX(cs_samplers),
      &genX(cs_state),
   };

   STATIC_ASSERT(ARRAY_SIZE(compute_atoms) <= ARRAY_SIZE(brw->compute_atoms));
   brw_copy_pipeline_atoms(brw, BRW_COMPUTE_PIPELINE,
                           compute_atoms, ARRAY_SIZE(compute_atoms));

   brw->vtbl.emit_mi_report_perf_count = genX(emit_mi_report_perf_count);
   brw->vtbl.emit_compute_walker = genX(emit_gpgpu_walker);
#endif

   assert(brw->screen->devinfo.verx10 == GFX_VERx10);
}
