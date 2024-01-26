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

#include "main/arrayobj.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/macros.h"
#include "main/glformats.h"
#include "nir.h"

#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"

#include "brw_batch.h"
#include "brw_buffer_objects.h"

static const GLuint double_types_float[5] = {
   0,
   ISL_FORMAT_R64_FLOAT,
   ISL_FORMAT_R64G64_FLOAT,
   ISL_FORMAT_R64G64B64_FLOAT,
   ISL_FORMAT_R64G64B64A64_FLOAT
};

static const GLuint double_types_passthru[5] = {
   0,
   ISL_FORMAT_R64_PASSTHRU,
   ISL_FORMAT_R64G64_PASSTHRU,
   ISL_FORMAT_R64G64B64_PASSTHRU,
   ISL_FORMAT_R64G64B64A64_PASSTHRU
};

static const GLuint float_types[5] = {
   0,
   ISL_FORMAT_R32_FLOAT,
   ISL_FORMAT_R32G32_FLOAT,
   ISL_FORMAT_R32G32B32_FLOAT,
   ISL_FORMAT_R32G32B32A32_FLOAT
};

static const GLuint half_float_types[5] = {
   0,
   ISL_FORMAT_R16_FLOAT,
   ISL_FORMAT_R16G16_FLOAT,
   ISL_FORMAT_R16G16B16_FLOAT,
   ISL_FORMAT_R16G16B16A16_FLOAT
};

static const GLuint fixed_point_types[5] = {
   0,
   ISL_FORMAT_R32_SFIXED,
   ISL_FORMAT_R32G32_SFIXED,
   ISL_FORMAT_R32G32B32_SFIXED,
   ISL_FORMAT_R32G32B32A32_SFIXED,
};

static const GLuint uint_types_direct[5] = {
   0,
   ISL_FORMAT_R32_UINT,
   ISL_FORMAT_R32G32_UINT,
   ISL_FORMAT_R32G32B32_UINT,
   ISL_FORMAT_R32G32B32A32_UINT
};

static const GLuint uint_types_norm[5] = {
   0,
   ISL_FORMAT_R32_UNORM,
   ISL_FORMAT_R32G32_UNORM,
   ISL_FORMAT_R32G32B32_UNORM,
   ISL_FORMAT_R32G32B32A32_UNORM
};

static const GLuint uint_types_scale[5] = {
   0,
   ISL_FORMAT_R32_USCALED,
   ISL_FORMAT_R32G32_USCALED,
   ISL_FORMAT_R32G32B32_USCALED,
   ISL_FORMAT_R32G32B32A32_USCALED
};

static const GLuint int_types_direct[5] = {
   0,
   ISL_FORMAT_R32_SINT,
   ISL_FORMAT_R32G32_SINT,
   ISL_FORMAT_R32G32B32_SINT,
   ISL_FORMAT_R32G32B32A32_SINT
};

static const GLuint int_types_norm[5] = {
   0,
   ISL_FORMAT_R32_SNORM,
   ISL_FORMAT_R32G32_SNORM,
   ISL_FORMAT_R32G32B32_SNORM,
   ISL_FORMAT_R32G32B32A32_SNORM
};

static const GLuint int_types_scale[5] = {
   0,
   ISL_FORMAT_R32_SSCALED,
   ISL_FORMAT_R32G32_SSCALED,
   ISL_FORMAT_R32G32B32_SSCALED,
   ISL_FORMAT_R32G32B32A32_SSCALED
};

static const GLuint ushort_types_direct[5] = {
   0,
   ISL_FORMAT_R16_UINT,
   ISL_FORMAT_R16G16_UINT,
   ISL_FORMAT_R16G16B16_UINT,
   ISL_FORMAT_R16G16B16A16_UINT
};

static const GLuint ushort_types_norm[5] = {
   0,
   ISL_FORMAT_R16_UNORM,
   ISL_FORMAT_R16G16_UNORM,
   ISL_FORMAT_R16G16B16_UNORM,
   ISL_FORMAT_R16G16B16A16_UNORM
};

static const GLuint ushort_types_scale[5] = {
   0,
   ISL_FORMAT_R16_USCALED,
   ISL_FORMAT_R16G16_USCALED,
   ISL_FORMAT_R16G16B16_USCALED,
   ISL_FORMAT_R16G16B16A16_USCALED
};

static const GLuint short_types_direct[5] = {
   0,
   ISL_FORMAT_R16_SINT,
   ISL_FORMAT_R16G16_SINT,
   ISL_FORMAT_R16G16B16_SINT,
   ISL_FORMAT_R16G16B16A16_SINT
};

static const GLuint short_types_norm[5] = {
   0,
   ISL_FORMAT_R16_SNORM,
   ISL_FORMAT_R16G16_SNORM,
   ISL_FORMAT_R16G16B16_SNORM,
   ISL_FORMAT_R16G16B16A16_SNORM
};

static const GLuint short_types_scale[5] = {
   0,
   ISL_FORMAT_R16_SSCALED,
   ISL_FORMAT_R16G16_SSCALED,
   ISL_FORMAT_R16G16B16_SSCALED,
   ISL_FORMAT_R16G16B16A16_SSCALED
};

static const GLuint ubyte_types_direct[5] = {
   0,
   ISL_FORMAT_R8_UINT,
   ISL_FORMAT_R8G8_UINT,
   ISL_FORMAT_R8G8B8_UINT,
   ISL_FORMAT_R8G8B8A8_UINT
};

static const GLuint ubyte_types_norm[5] = {
   0,
   ISL_FORMAT_R8_UNORM,
   ISL_FORMAT_R8G8_UNORM,
   ISL_FORMAT_R8G8B8_UNORM,
   ISL_FORMAT_R8G8B8A8_UNORM
};

static const GLuint ubyte_types_scale[5] = {
   0,
   ISL_FORMAT_R8_USCALED,
   ISL_FORMAT_R8G8_USCALED,
   ISL_FORMAT_R8G8B8_USCALED,
   ISL_FORMAT_R8G8B8A8_USCALED
};

static const GLuint byte_types_direct[5] = {
   0,
   ISL_FORMAT_R8_SINT,
   ISL_FORMAT_R8G8_SINT,
   ISL_FORMAT_R8G8B8_SINT,
   ISL_FORMAT_R8G8B8A8_SINT
};

static const GLuint byte_types_norm[5] = {
   0,
   ISL_FORMAT_R8_SNORM,
   ISL_FORMAT_R8G8_SNORM,
   ISL_FORMAT_R8G8B8_SNORM,
   ISL_FORMAT_R8G8B8A8_SNORM
};

static const GLuint byte_types_scale[5] = {
   0,
   ISL_FORMAT_R8_SSCALED,
   ISL_FORMAT_R8G8_SSCALED,
   ISL_FORMAT_R8G8B8_SSCALED,
   ISL_FORMAT_R8G8B8A8_SSCALED
};

static GLuint
double_types(int size, GLboolean doubles)
{
   /* From the BDW PRM, Volume 2d, page 588 (VERTEX_ELEMENT_STATE):
    * "When SourceElementFormat is set to one of the *64*_PASSTHRU formats,
    * 64-bit components are stored in the URB without any conversion."
    * Also included on BDW PRM, Volume 7, page 470, table "Source Element
    * Formats Supported in VF Unit"
    *
    * Previous PRMs don't include those references, so for gfx7 we can't use
    * PASSTHRU formats directly. But in any case, we prefer to return passthru
    * even in that case, because that reflects what we want to achieve, even
    * if we would need to workaround on gen < 8.
    */
   return (doubles
           ? double_types_passthru[size]
           : double_types_float[size]);
}

/**
 * Given vertex array type/size/format/normalized info, return
 * the appopriate hardware surface type.
 * Format will be GL_RGBA or possibly GL_BGRA for GLubyte[4] color arrays.
 */
unsigned
brw_get_vertex_surface_type(struct brw_context *brw,
                            const struct gl_vertex_format *glformat)
{
   int size = glformat->Size;
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   const bool is_ivybridge_or_older =
      devinfo->verx10 <= 70 && !devinfo->is_baytrail;

   if (INTEL_DEBUG(DEBUG_VERTS))
      fprintf(stderr, "type %s size %d normalized %d\n",
              _mesa_enum_to_string(glformat->Type),
              glformat->Size, glformat->Normalized);

   if (glformat->Integer) {
      assert(glformat->Format == GL_RGBA); /* sanity check */
      switch (glformat->Type) {
      case GL_INT: return int_types_direct[size];
      case GL_SHORT:
         if (is_ivybridge_or_older && size == 3)
            return short_types_direct[4];
         else
            return short_types_direct[size];
      case GL_BYTE:
         if (is_ivybridge_or_older && size == 3)
            return byte_types_direct[4];
         else
            return byte_types_direct[size];
      case GL_UNSIGNED_INT: return uint_types_direct[size];
      case GL_UNSIGNED_SHORT:
         if (is_ivybridge_or_older && size == 3)
            return ushort_types_direct[4];
         else
            return ushort_types_direct[size];
      case GL_UNSIGNED_BYTE:
         if (is_ivybridge_or_older && size == 3)
            return ubyte_types_direct[4];
         else
            return ubyte_types_direct[size];
      default: unreachable("not reached");
      }
   } else if (glformat->Type == GL_UNSIGNED_INT_10F_11F_11F_REV) {
      return ISL_FORMAT_R11G11B10_FLOAT;
   } else if (glformat->Normalized) {
      switch (glformat->Type) {
      case GL_DOUBLE: return double_types(size, glformat->Doubles);
      case GL_FLOAT: return float_types[size];
      case GL_HALF_FLOAT:
      case GL_HALF_FLOAT_OES:
         if (devinfo->ver < 6 && size == 3)
            return half_float_types[4];
         else
            return half_float_types[size];
      case GL_INT: return int_types_norm[size];
      case GL_SHORT: return short_types_norm[size];
      case GL_BYTE: return byte_types_norm[size];
      case GL_UNSIGNED_INT: return uint_types_norm[size];
      case GL_UNSIGNED_SHORT: return ushort_types_norm[size];
      case GL_UNSIGNED_BYTE:
         if (glformat->Format == GL_BGRA) {
            /* See GL_EXT_vertex_array_bgra */
            assert(size == 4);
            return ISL_FORMAT_B8G8R8A8_UNORM;
         }
         else {
            return ubyte_types_norm[size];
         }
      case GL_FIXED:
         if (devinfo->verx10 >= 75)
            return fixed_point_types[size];

         /* This produces GL_FIXED inputs as values between INT32_MIN and
          * INT32_MAX, which will be scaled down by 1/65536 by the VS.
          */
         return int_types_scale[size];
      /* See GL_ARB_vertex_type_2_10_10_10_rev.
       * W/A: Pre-Haswell, the hardware doesn't really support the formats we'd
       * like to use here, so upload everything as UINT and fix
       * it in the shader
       */
      case GL_INT_2_10_10_10_REV:
         assert(size == 4);
         if (devinfo->verx10 >= 75) {
            return glformat->Format == GL_BGRA
               ? ISL_FORMAT_B10G10R10A2_SNORM
               : ISL_FORMAT_R10G10B10A2_SNORM;
         }
         return ISL_FORMAT_R10G10B10A2_UINT;
      case GL_UNSIGNED_INT_2_10_10_10_REV:
         assert(size == 4);
         if (devinfo->verx10 >= 75) {
            return glformat->Format == GL_BGRA
               ? ISL_FORMAT_B10G10R10A2_UNORM
               : ISL_FORMAT_R10G10B10A2_UNORM;
         }
         return ISL_FORMAT_R10G10B10A2_UINT;
      default: unreachable("not reached");
      }
   }
   else {
      /* See GL_ARB_vertex_type_2_10_10_10_rev.
       * W/A: the hardware doesn't really support the formats we'd
       * like to use here, so upload everything as UINT and fix
       * it in the shader
       */
      if (glformat->Type == GL_INT_2_10_10_10_REV) {
         assert(size == 4);
         if (devinfo->verx10 >= 75) {
            return glformat->Format == GL_BGRA
               ? ISL_FORMAT_B10G10R10A2_SSCALED
               : ISL_FORMAT_R10G10B10A2_SSCALED;
         }
         return ISL_FORMAT_R10G10B10A2_UINT;
      } else if (glformat->Type == GL_UNSIGNED_INT_2_10_10_10_REV) {
         assert(size == 4);
         if (devinfo->verx10 >= 75) {
            return glformat->Format == GL_BGRA
               ? ISL_FORMAT_B10G10R10A2_USCALED
               : ISL_FORMAT_R10G10B10A2_USCALED;
         }
         return ISL_FORMAT_R10G10B10A2_UINT;
      }
      assert(glformat->Format == GL_RGBA); /* sanity check */
      switch (glformat->Type) {
      case GL_DOUBLE: return double_types(size, glformat->Doubles);
      case GL_FLOAT: return float_types[size];
      case GL_HALF_FLOAT:
      case GL_HALF_FLOAT_OES:
         if (devinfo->ver < 6 && size == 3)
            return half_float_types[4];
         else
            return half_float_types[size];
      case GL_INT: return int_types_scale[size];
      case GL_SHORT: return short_types_scale[size];
      case GL_BYTE: return byte_types_scale[size];
      case GL_UNSIGNED_INT: return uint_types_scale[size];
      case GL_UNSIGNED_SHORT: return ushort_types_scale[size];
      case GL_UNSIGNED_BYTE: return ubyte_types_scale[size];
      case GL_FIXED:
         if (devinfo->verx10 >= 75)
            return fixed_point_types[size];

         /* This produces GL_FIXED inputs as values between INT32_MIN and
          * INT32_MAX, which will be scaled down by 1/65536 by the VS.
          */
         return int_types_scale[size];
      default: unreachable("not reached");
      }
   }
}

static void
copy_array_to_vbo_array(struct brw_context *brw,
                        const uint8_t *const ptr, const int src_stride,
                        int min, int max,
                        struct brw_vertex_buffer *buffer,
                        GLuint dst_stride)
{
   const unsigned char *src = ptr + min * src_stride;
   int count = max - min + 1;
   GLuint size = count * dst_stride;
   uint8_t *dst = brw_upload_space(&brw->upload, size, dst_stride,
                                   &buffer->bo, &buffer->offset);

   /* The GL 4.5 spec says:
    *      "If any enabled array’s buffer binding is zero when DrawArrays or
    *      one of the other drawing commands defined in section 10.4 is called,
    *      the result is undefined."
    *
    * In this case, let's the dst with undefined values
    */
   if (ptr != NULL) {
      if (dst_stride == src_stride) {
         memcpy(dst, src, size);
      } else {
         while (count--) {
            memcpy(dst, src, dst_stride);
            src += src_stride;
            dst += dst_stride;
         }
      }
   }
   buffer->stride = dst_stride;
   buffer->size = size;
}

void
brw_prepare_vertices(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_VERTEX_PROGRAM */
   const struct gl_program *vp = brw->programs[MESA_SHADER_VERTEX];
   /* BRW_NEW_VS_PROG_DATA */
   const struct brw_vs_prog_data *vs_prog_data =
      brw_vs_prog_data(brw->vs.base.prog_data);
   const uint64_t vs_inputs64 =
      nir_get_single_slot_attribs_mask(vs_prog_data->inputs_read,
                                       vp->DualSlotInputs);
   assert((vs_inputs64 & ~(uint64_t)VERT_BIT_ALL) == 0);
   unsigned vs_inputs = (unsigned)vs_inputs64;
   unsigned int min_index = brw->vb.min_index + brw->basevertex;
   unsigned int max_index = brw->vb.max_index + brw->basevertex;
   int delta, j;

   /* _NEW_POLYGON
    *
    * On gfx6+, edge flags don't end up in the VUE (either in or out of the
    * VS).  Instead, they're uploaded as the last vertex element, and the data
    * is passed sideband through the fixed function units.  So, we need to
    * prepare the vertex buffer for it, but it's not present in inputs_read.
    */
   if (devinfo->ver >= 6 && (ctx->Polygon.FrontMode != GL_FILL ||
                           ctx->Polygon.BackMode != GL_FILL)) {
      vs_inputs |= VERT_BIT_EDGEFLAG;
   }

   if (0)
      fprintf(stderr, "%s %d..%d\n", __func__, min_index, max_index);

   /* Accumulate the list of enabled arrays. */
   brw->vb.nr_enabled = 0;

   unsigned mask = vs_inputs;
   while (mask) {
      const gl_vert_attrib attr = u_bit_scan(&mask);
      struct brw_vertex_element *input = &brw->vb.inputs[attr];
      brw->vb.enabled[brw->vb.nr_enabled++] = input;
   }
   assert(brw->vb.nr_enabled <= VERT_ATTRIB_MAX);

   if (brw->vb.nr_enabled == 0)
      return;

   if (brw->vb.nr_buffers)
      return;

   j = 0;
   const struct gl_vertex_array_object *vao = ctx->Array._DrawVAO;

   unsigned vbomask = vs_inputs & _mesa_draw_vbo_array_bits(ctx);
   while (vbomask) {
      const struct gl_vertex_buffer_binding *const glbinding =
         _mesa_draw_buffer_binding(vao, ffs(vbomask) - 1);
      const GLsizei stride = glbinding->Stride;

      assert(glbinding->BufferObj);

      /* Accumulate the range of a single vertex, start with inverted range */
      uint32_t vertex_range_start = ~(uint32_t)0;
      uint32_t vertex_range_end = 0;

      const unsigned boundmask = _mesa_draw_bound_attrib_bits(glbinding);
      unsigned attrmask = vbomask & boundmask;
      /* Mark the those attributes as processed */
      vbomask ^= attrmask;
      /* We can assume that we have an array for the binding */
      assert(attrmask);
      /* Walk attributes belonging to the binding */
      while (attrmask) {
         const gl_vert_attrib attr = u_bit_scan(&attrmask);
         const struct gl_array_attributes *const glattrib =
            _mesa_draw_array_attrib(vao, attr);
         const uint32_t rel_offset =
            _mesa_draw_attributes_relative_offset(glattrib);
         const uint32_t rel_end = rel_offset + glattrib->Format._ElementSize;

         vertex_range_start = MIN2(vertex_range_start, rel_offset);
         vertex_range_end = MAX2(vertex_range_end, rel_end);

         struct brw_vertex_element *input = &brw->vb.inputs[attr];
         input->glformat = &glattrib->Format;
         input->buffer = j;
         input->is_dual_slot = (vp->DualSlotInputs & BITFIELD64_BIT(attr)) != 0;
         input->offset = rel_offset;
      }
      assert(vertex_range_start <= vertex_range_end);

      struct brw_buffer_object *intel_buffer =
         brw_buffer_object(glbinding->BufferObj);
      struct brw_vertex_buffer *buffer = &brw->vb.buffers[j];

      const uint32_t offset = _mesa_draw_binding_offset(glbinding);

      /* If nothing else is known take the buffer size and offset as a bound */
      uint32_t start = vertex_range_start;
      uint32_t range = intel_buffer->Base.Size - offset - vertex_range_start;
      /* Check if we can get a more narrow range */
      if (glbinding->InstanceDivisor) {
         if (brw->num_instances) {
            const uint32_t vertex_size = vertex_range_end - vertex_range_start;
            start = vertex_range_start + stride * brw->baseinstance;
            range = (stride * ((brw->num_instances - 1) /
                               glbinding->InstanceDivisor) +
                     vertex_size);
         }
      } else {
         if (brw->vb.index_bounds_valid) {
            const uint32_t vertex_size = vertex_range_end - vertex_range_start;
            start = vertex_range_start + stride * min_index;
            range = (stride * (max_index - min_index) +
                     vertex_size);

            /**
             * Unreal Engine 4 has a bug in usage of glDrawRangeElements,
             * causing it to be called with a number of vertices in place
             * of "end" parameter (which specifies the maximum array index
             * contained in indices).
             *
             * Since there is unknown amount of games affected and we
             * could not identify that a game is built with UE4 - we are
             * forced to make a blanket workaround, disregarding max_index
             * in range calculations. Fortunately all such calls look like:
             *   glDrawRangeElements(GL_TRIANGLES, 0, 3, 3, ...);
             * So we are able to narrow down this workaround.
             *
             * See: https://gitlab.freedesktop.org/mesa/mesa/-/issues/2917
             */
            if (unlikely(max_index == 3 && min_index == 0 &&
                         brw->draw.derived_params.is_indexed_draw)) {
                  range = intel_buffer->Base.Size - offset - start;
            }
         }
      }

      buffer->offset = offset;
      buffer->size = start + range;
      buffer->stride = stride;
      buffer->step_rate = glbinding->InstanceDivisor;

      buffer->bo = brw_bufferobj_buffer(brw, intel_buffer, offset + start,
                                        range, false);
      brw_bo_reference(buffer->bo);

      j++;
   }

   /* If we need to upload all the arrays, then we can trim those arrays to
    * only the used elements [min_index, max_index] so long as we adjust all
    * the values used in the 3DPRIMITIVE i.e. by setting the vertex bias.
    */
   brw->vb.start_vertex_bias = 0;
   delta = min_index;
   if ((vs_inputs & _mesa_draw_vbo_array_bits(ctx)) == 0) {
      brw->vb.start_vertex_bias = -delta;
      delta = 0;
   }

   unsigned usermask = vs_inputs & _mesa_draw_user_array_bits(ctx);
   while (usermask) {
      const struct gl_vertex_buffer_binding *const glbinding =
         _mesa_draw_buffer_binding(vao, ffs(usermask) - 1);
      const GLsizei stride = glbinding->Stride;

      assert(!glbinding->BufferObj);
      assert(brw->vb.index_bounds_valid);

      /* Accumulate the range of a single vertex, start with inverted range */
      uint32_t vertex_range_start = ~(uint32_t)0;
      uint32_t vertex_range_end = 0;

      const unsigned boundmask = _mesa_draw_bound_attrib_bits(glbinding);
      unsigned attrmask = usermask & boundmask;
      /* Mark the those attributes as processed */
      usermask ^= attrmask;
      /* We can assume that we have an array for the binding */
      assert(attrmask);
      /* Walk attributes belonging to the binding */
      while (attrmask) {
         const gl_vert_attrib attr = u_bit_scan(&attrmask);
         const struct gl_array_attributes *const glattrib =
            _mesa_draw_array_attrib(vao, attr);
         const uint32_t rel_offset =
            _mesa_draw_attributes_relative_offset(glattrib);
         const uint32_t rel_end = rel_offset + glattrib->Format._ElementSize;

         vertex_range_start = MIN2(vertex_range_start, rel_offset);
         vertex_range_end = MAX2(vertex_range_end, rel_end);

         struct brw_vertex_element *input = &brw->vb.inputs[attr];
         input->glformat = &glattrib->Format;
         input->buffer = j;
         input->is_dual_slot = (vp->DualSlotInputs & BITFIELD64_BIT(attr)) != 0;
         input->offset = rel_offset;
      }
      assert(vertex_range_start <= vertex_range_end);

      struct brw_vertex_buffer *buffer = &brw->vb.buffers[j];

      const uint8_t *ptr = (const uint8_t*)_mesa_draw_binding_offset(glbinding);
      ptr += vertex_range_start;
      const uint32_t vertex_size = vertex_range_end - vertex_range_start;
      if (glbinding->Stride == 0) {
         /* If the source stride is zero, we just want to upload the current
          * attribute once and set the buffer's stride to 0.  There's no need
          * to replicate it out.
          */
         copy_array_to_vbo_array(brw, ptr, 0, 0, 0, buffer, vertex_size);
      } else if (glbinding->InstanceDivisor == 0) {
         copy_array_to_vbo_array(brw, ptr, stride, min_index,
                                 max_index, buffer, vertex_size);
      } else {
         /* This is an instanced attribute, since its InstanceDivisor
          * is not zero. Therefore, its data will be stepped after the
          * instanced draw has been run InstanceDivisor times.
          */
         uint32_t instanced_attr_max_index =
            (brw->num_instances - 1) / glbinding->InstanceDivisor;
         copy_array_to_vbo_array(brw, ptr, stride, 0,
                                 instanced_attr_max_index, buffer, vertex_size);
      }
      buffer->offset -= delta * buffer->stride + vertex_range_start;
      buffer->size += delta * buffer->stride + vertex_range_start;
      buffer->step_rate = glbinding->InstanceDivisor;

      j++;
   }

   /* Upload the current values */
   unsigned curmask = vs_inputs & _mesa_draw_current_bits(ctx);
   if (curmask) {
      /* For each attribute, upload the maximum possible size. */
      uint8_t data[VERT_ATTRIB_MAX * sizeof(GLdouble) * 4];
      uint8_t *cursor = data;

      do {
         const gl_vert_attrib attr = u_bit_scan(&curmask);
         const struct gl_array_attributes *const glattrib =
            _mesa_draw_current_attrib(ctx, attr);
         const unsigned size = glattrib->Format._ElementSize;
         const unsigned alignment = align(size, sizeof(GLdouble));
         memcpy(cursor, glattrib->Ptr, size);
         if (alignment != size)
            memset(cursor + size, 0, alignment - size);

         struct brw_vertex_element *input = &brw->vb.inputs[attr];
         input->glformat = &glattrib->Format;
         input->buffer = j;
         input->is_dual_slot = (vp->DualSlotInputs & BITFIELD64_BIT(attr)) != 0;
         input->offset = cursor - data;

         cursor += alignment;
      } while (curmask);

      struct brw_vertex_buffer *buffer = &brw->vb.buffers[j];
      const unsigned size = cursor - data;
      brw_upload_data(&brw->upload, data, size, size,
                      &buffer->bo, &buffer->offset);
      buffer->stride = 0;
      buffer->size = size;
      buffer->step_rate = 0;

      j++;
   }
   brw->vb.nr_buffers = j;
}

void
brw_prepare_shader_draw_parameters(struct brw_context *brw)
{
   const struct brw_vs_prog_data *vs_prog_data =
      brw_vs_prog_data(brw->vs.base.prog_data);

   /* For non-indirect draws, upload the shader draw parameters */
   if ((vs_prog_data->uses_firstvertex || vs_prog_data->uses_baseinstance) &&
       brw->draw.draw_params_bo == NULL) {
      brw_upload_data(&brw->upload,
                      &brw->draw.params, sizeof(brw->draw.params), 4,
                      &brw->draw.draw_params_bo,
                      &brw->draw.draw_params_offset);
   }

   if (vs_prog_data->uses_drawid || vs_prog_data->uses_is_indexed_draw) {
      brw_upload_data(&brw->upload,
                      &brw->draw.derived_params, sizeof(brw->draw.derived_params), 4,
                      &brw->draw.derived_draw_params_bo,
                      &brw->draw.derived_draw_params_offset);
   }
}

static void
brw_upload_indices(struct brw_context *brw)
{
   const struct _mesa_index_buffer *index_buffer = brw->ib.ib;
   GLuint ib_size;
   struct brw_bo *old_bo = brw->ib.bo;
   struct gl_buffer_object *bufferobj;
   GLuint offset;
   GLuint ib_type_size;

   if (index_buffer == NULL)
      return;

   ib_type_size = 1 << index_buffer->index_size_shift;
   ib_size = index_buffer->count ? ib_type_size * index_buffer->count :
                                   index_buffer->obj->Size;
   bufferobj = index_buffer->obj;

   /* Turn into a proper VBO:
    */
   if (!bufferobj) {
      /* Get new bufferobj, offset:
       */
      brw_upload_data(&brw->upload, index_buffer->ptr, ib_size, ib_type_size,
                      &brw->ib.bo, &offset);
      brw->ib.size = brw->ib.bo->size;
   } else {
      offset = (GLuint) (unsigned long) index_buffer->ptr;

      struct brw_bo *bo =
         brw_bufferobj_buffer(brw, brw_buffer_object(bufferobj),
                              offset, ib_size, false);
      if (bo != brw->ib.bo) {
         brw_bo_unreference(brw->ib.bo);
         brw->ib.bo = bo;
         brw->ib.size = bufferobj->Size;
         brw_bo_reference(bo);
      }
   }

   /* Use 3DPRIMITIVE's start_vertex_offset to avoid re-uploading
    * the index buffer state when we're just moving the start index
    * of our drawing.
    */
   brw->ib.start_vertex_offset = offset / ib_type_size;

   if (brw->ib.bo != old_bo)
      brw->ctx.NewDriverState |= BRW_NEW_INDEX_BUFFER;

   unsigned index_size = 1 << index_buffer->index_size_shift;
   if (index_size != brw->ib.index_size) {
      brw->ib.index_size = index_size;
      brw->ctx.NewDriverState |= BRW_NEW_INDEX_BUFFER;
   }

   /* We need to re-emit an index buffer state each time
    * when cut index flag is changed
    */
   if (brw->prim_restart.enable_cut_index != brw->ib.enable_cut_index) {
      brw->ib.enable_cut_index = brw->prim_restart.enable_cut_index;
      brw->ctx.NewDriverState |= BRW_NEW_INDEX_BUFFER;
   }
}

const struct brw_tracked_state brw_indices = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BLORP |
             BRW_NEW_INDICES,
   },
   .emit = brw_upload_indices,
};
