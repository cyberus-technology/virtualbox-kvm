/*
 * Copyright © 2015 Intel Corporation
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

#include "anv_private.h"

#include "genxml/gen_macros.h"
#include "genxml/genX_pack.h"
#include "genxml/gen_rt_pack.h"

#include "common/intel_l3_config.h"
#include "common/intel_sample_positions.h"
#include "nir/nir_xfb_info.h"
#include "vk_util.h"
#include "vk_format.h"
#include "vk_log.h"

static uint32_t
vertex_element_comp_control(enum isl_format format, unsigned comp)
{
   uint8_t bits;
   switch (comp) {
   case 0: bits = isl_format_layouts[format].channels.r.bits; break;
   case 1: bits = isl_format_layouts[format].channels.g.bits; break;
   case 2: bits = isl_format_layouts[format].channels.b.bits; break;
   case 3: bits = isl_format_layouts[format].channels.a.bits; break;
   default: unreachable("Invalid component");
   }

   /*
    * Take in account hardware restrictions when dealing with 64-bit floats.
    *
    * From Broadwell spec, command reference structures, page 586:
    *  "When SourceElementFormat is set to one of the *64*_PASSTHRU formats,
    *   64-bit components are stored * in the URB without any conversion. In
    *   this case, vertex elements must be written as 128 or 256 bits, with
    *   VFCOMP_STORE_0 being used to pad the output as required. E.g., if
    *   R64_PASSTHRU is used to copy a 64-bit Red component into the URB,
    *   Component 1 must be specified as VFCOMP_STORE_0 (with Components 2,3
    *   set to VFCOMP_NOSTORE) in order to output a 128-bit vertex element, or
    *   Components 1-3 must be specified as VFCOMP_STORE_0 in order to output
    *   a 256-bit vertex element. Likewise, use of R64G64B64_PASSTHRU requires
    *   Component 3 to be specified as VFCOMP_STORE_0 in order to output a
    *   256-bit vertex element."
    */
   if (bits) {
      return VFCOMP_STORE_SRC;
   } else if (comp >= 2 &&
              !isl_format_layouts[format].channels.b.bits &&
              isl_format_layouts[format].channels.r.type == ISL_RAW) {
      /* When emitting 64-bit attributes, we need to write either 128 or 256
       * bit chunks, using VFCOMP_NOSTORE when not writing the chunk, and
       * VFCOMP_STORE_0 to pad the written chunk */
      return VFCOMP_NOSTORE;
   } else if (comp < 3 ||
              isl_format_layouts[format].channels.r.type == ISL_RAW) {
      /* Note we need to pad with value 0, not 1, due hardware restrictions
       * (see comment above) */
      return VFCOMP_STORE_0;
   } else if (isl_format_layouts[format].channels.r.type == ISL_UINT ||
            isl_format_layouts[format].channels.r.type == ISL_SINT) {
      assert(comp == 3);
      return VFCOMP_STORE_1_INT;
   } else {
      assert(comp == 3);
      return VFCOMP_STORE_1_FP;
   }
}

static void
emit_vertex_input(struct anv_graphics_pipeline *pipeline,
                  const VkPipelineVertexInputStateCreateInfo *info)
{
   const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);

   /* Pull inputs_read out of the VS prog data */
   const uint64_t inputs_read = vs_prog_data->inputs_read;
   const uint64_t double_inputs_read =
      vs_prog_data->double_inputs_read & inputs_read;
   assert((inputs_read & ((1 << VERT_ATTRIB_GENERIC0) - 1)) == 0);
   const uint32_t elements = inputs_read >> VERT_ATTRIB_GENERIC0;
   const uint32_t elements_double = double_inputs_read >> VERT_ATTRIB_GENERIC0;
   const bool needs_svgs_elem = vs_prog_data->uses_vertexid ||
                                vs_prog_data->uses_instanceid ||
                                vs_prog_data->uses_firstvertex ||
                                vs_prog_data->uses_baseinstance;

   uint32_t elem_count = __builtin_popcount(elements) -
      __builtin_popcount(elements_double) / 2;

   const uint32_t total_elems =
      MAX2(1, elem_count + needs_svgs_elem + vs_prog_data->uses_drawid);

   uint32_t *p;

   const uint32_t num_dwords = 1 + total_elems * 2;
   p = anv_batch_emitn(&pipeline->base.batch, num_dwords,
                       GENX(3DSTATE_VERTEX_ELEMENTS));
   if (!p)
      return;

   for (uint32_t i = 0; i < total_elems; i++) {
      /* The SKL docs for VERTEX_ELEMENT_STATE say:
       *
       *    "All elements must be valid from Element[0] to the last valid
       *    element. (I.e. if Element[2] is valid then Element[1] and
       *    Element[0] must also be valid)."
       *
       * The SKL docs for 3D_Vertex_Component_Control say:
       *
       *    "Don't store this component. (Not valid for Component 0, but can
       *    be used for Component 1-3)."
       *
       * So we can't just leave a vertex element blank and hope for the best.
       * We have to tell the VF hardware to put something in it; so we just
       * store a bunch of zero.
       *
       * TODO: Compact vertex elements so we never end up with holes.
       */
      struct GENX(VERTEX_ELEMENT_STATE) element = {
         .Valid = true,
         .Component0Control = VFCOMP_STORE_0,
         .Component1Control = VFCOMP_STORE_0,
         .Component2Control = VFCOMP_STORE_0,
         .Component3Control = VFCOMP_STORE_0,
      };
      GENX(VERTEX_ELEMENT_STATE_pack)(NULL, &p[1 + i * 2], &element);
   }

   for (uint32_t i = 0; i < info->vertexAttributeDescriptionCount; i++) {
      const VkVertexInputAttributeDescription *desc =
         &info->pVertexAttributeDescriptions[i];
      enum isl_format format = anv_get_isl_format(&pipeline->base.device->info,
                                                  desc->format,
                                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                                  VK_IMAGE_TILING_LINEAR);

      assert(desc->binding < MAX_VBS);

      if ((elements & (1 << desc->location)) == 0)
         continue; /* Binding unused */

      uint32_t slot =
         __builtin_popcount(elements & ((1 << desc->location) - 1)) -
         DIV_ROUND_UP(__builtin_popcount(elements_double &
                                        ((1 << desc->location) -1)), 2);

      struct GENX(VERTEX_ELEMENT_STATE) element = {
         .VertexBufferIndex = desc->binding,
         .Valid = true,
         .SourceElementFormat = format,
         .EdgeFlagEnable = false,
         .SourceElementOffset = desc->offset,
         .Component0Control = vertex_element_comp_control(format, 0),
         .Component1Control = vertex_element_comp_control(format, 1),
         .Component2Control = vertex_element_comp_control(format, 2),
         .Component3Control = vertex_element_comp_control(format, 3),
      };
      GENX(VERTEX_ELEMENT_STATE_pack)(NULL, &p[1 + slot * 2], &element);

#if GFX_VER >= 8
      /* On Broadwell and later, we have a separate VF_INSTANCING packet
       * that controls instancing.  On Haswell and prior, that's part of
       * VERTEX_BUFFER_STATE which we emit later.
       */
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_VF_INSTANCING), vfi) {
         vfi.InstancingEnable = pipeline->vb[desc->binding].instanced;
         vfi.VertexElementIndex = slot;
         vfi.InstanceDataStepRate =
            pipeline->vb[desc->binding].instance_divisor;
      }
#endif
   }

   const uint32_t id_slot = elem_count;
   if (needs_svgs_elem) {
      /* From the Broadwell PRM for the 3D_Vertex_Component_Control enum:
       *    "Within a VERTEX_ELEMENT_STATE structure, if a Component
       *    Control field is set to something other than VFCOMP_STORE_SRC,
       *    no higher-numbered Component Control fields may be set to
       *    VFCOMP_STORE_SRC"
       *
       * This means, that if we have BaseInstance, we need BaseVertex as
       * well.  Just do all or nothing.
       */
      uint32_t base_ctrl = (vs_prog_data->uses_firstvertex ||
                            vs_prog_data->uses_baseinstance) ?
                           VFCOMP_STORE_SRC : VFCOMP_STORE_0;

      struct GENX(VERTEX_ELEMENT_STATE) element = {
         .VertexBufferIndex = ANV_SVGS_VB_INDEX,
         .Valid = true,
         .SourceElementFormat = ISL_FORMAT_R32G32_UINT,
         .Component0Control = base_ctrl,
         .Component1Control = base_ctrl,
#if GFX_VER >= 8
         .Component2Control = VFCOMP_STORE_0,
         .Component3Control = VFCOMP_STORE_0,
#else
         .Component2Control = VFCOMP_STORE_VID,
         .Component3Control = VFCOMP_STORE_IID,
#endif
      };
      GENX(VERTEX_ELEMENT_STATE_pack)(NULL, &p[1 + id_slot * 2], &element);

#if GFX_VER >= 8
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_VF_INSTANCING), vfi) {
         vfi.VertexElementIndex = id_slot;
      }
#endif
   }

#if GFX_VER >= 8
   anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_VF_SGVS), sgvs) {
      sgvs.VertexIDEnable              = vs_prog_data->uses_vertexid;
      sgvs.VertexIDComponentNumber     = 2;
      sgvs.VertexIDElementOffset       = id_slot;
      sgvs.InstanceIDEnable            = vs_prog_data->uses_instanceid;
      sgvs.InstanceIDComponentNumber   = 3;
      sgvs.InstanceIDElementOffset     = id_slot;
   }
#endif

   const uint32_t drawid_slot = elem_count + needs_svgs_elem;
   if (vs_prog_data->uses_drawid) {
      struct GENX(VERTEX_ELEMENT_STATE) element = {
         .VertexBufferIndex = ANV_DRAWID_VB_INDEX,
         .Valid = true,
         .SourceElementFormat = ISL_FORMAT_R32_UINT,
         .Component0Control = VFCOMP_STORE_SRC,
         .Component1Control = VFCOMP_STORE_0,
         .Component2Control = VFCOMP_STORE_0,
         .Component3Control = VFCOMP_STORE_0,
      };
      GENX(VERTEX_ELEMENT_STATE_pack)(NULL,
                                      &p[1 + drawid_slot * 2],
                                      &element);

#if GFX_VER >= 8
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_VF_INSTANCING), vfi) {
         vfi.VertexElementIndex = drawid_slot;
      }
#endif
   }
}

void
genX(emit_urb_setup)(struct anv_device *device, struct anv_batch *batch,
                     const struct intel_l3_config *l3_config,
                     VkShaderStageFlags active_stages,
                     const unsigned entry_size[4],
                     enum intel_urb_deref_block_size *deref_block_size)
{
   const struct intel_device_info *devinfo = &device->info;

   unsigned entries[4];
   unsigned start[4];
   bool constrained;
   intel_get_urb_config(devinfo, l3_config,
                        active_stages &
                           VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                        active_stages & VK_SHADER_STAGE_GEOMETRY_BIT,
                        entry_size, entries, start, deref_block_size,
                        &constrained);

#if GFX_VERx10 == 70
   /* From the IVB PRM Vol. 2, Part 1, Section 3.2.1:
    *
    *    "A PIPE_CONTROL with Post-Sync Operation set to 1h and a depth stall
    *    needs to be sent just prior to any 3DSTATE_VS, 3DSTATE_URB_VS,
    *    3DSTATE_CONSTANT_VS, 3DSTATE_BINDING_TABLE_POINTER_VS,
    *    3DSTATE_SAMPLER_STATE_POINTER_VS command.  Only one PIPE_CONTROL
    *    needs to be sent before any combination of VS associated 3DSTATE."
    */
   anv_batch_emit(batch, GFX7_PIPE_CONTROL, pc) {
      pc.DepthStallEnable  = true;
      pc.PostSyncOperation = WriteImmediateData;
      pc.Address           = device->workaround_address;
   }
#endif

   for (int i = 0; i <= MESA_SHADER_GEOMETRY; i++) {
      anv_batch_emit(batch, GENX(3DSTATE_URB_VS), urb) {
         urb._3DCommandSubOpcode      += i;
         urb.VSURBStartingAddress      = start[i];
         urb.VSURBEntryAllocationSize  = entry_size[i] - 1;
         urb.VSNumberofURBEntries      = entries[i];
      }
   }
}

static void
emit_urb_setup(struct anv_graphics_pipeline *pipeline,
               enum intel_urb_deref_block_size *deref_block_size)
{
   unsigned entry_size[4];
   for (int i = MESA_SHADER_VERTEX; i <= MESA_SHADER_GEOMETRY; i++) {
      const struct brw_vue_prog_data *prog_data =
         !anv_pipeline_has_stage(pipeline, i) ? NULL :
         (const struct brw_vue_prog_data *) pipeline->shaders[i]->prog_data;

      entry_size[i] = prog_data ? prog_data->urb_entry_size : 1;
   }

   genX(emit_urb_setup)(pipeline->base.device, &pipeline->base.batch,
                        pipeline->base.l3_config,
                        pipeline->active_stages, entry_size,
                        deref_block_size);
}

static void
emit_3dstate_sbe(struct anv_graphics_pipeline *pipeline)
{
   const struct brw_wm_prog_data *wm_prog_data = get_wm_prog_data(pipeline);

   if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT)) {
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_SBE), sbe);
#if GFX_VER >= 8
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_SBE_SWIZ), sbe);
#endif
      return;
   }

   struct GENX(3DSTATE_SBE) sbe = {
      GENX(3DSTATE_SBE_header),
      .AttributeSwizzleEnable = true,
      .PointSpriteTextureCoordinateOrigin = UPPERLEFT,
      .NumberofSFOutputAttributes = wm_prog_data->num_varying_inputs,
      .ConstantInterpolationEnable = wm_prog_data->flat_inputs,
   };

#if GFX_VER >= 9
   for (unsigned i = 0; i < 32; i++)
      sbe.AttributeActiveComponentFormat[i] = ACF_XYZW;
#endif

#if GFX_VER >= 8
   /* On Broadwell, they broke 3DSTATE_SBE into two packets */
   struct GENX(3DSTATE_SBE_SWIZ) swiz = {
      GENX(3DSTATE_SBE_SWIZ_header),
   };
#else
#  define swiz sbe
#endif

   if (anv_pipeline_is_primitive(pipeline)) {
      const struct brw_vue_map *fs_input_map =
         &anv_pipeline_get_last_vue_prog_data(pipeline)->vue_map;

      int first_slot = brw_compute_first_urb_slot_required(wm_prog_data->inputs,
                                                           fs_input_map);
      assert(first_slot % 2 == 0);
      unsigned urb_entry_read_offset = first_slot / 2;
      int max_source_attr = 0;
      for (uint8_t idx = 0; idx < wm_prog_data->urb_setup_attribs_count; idx++) {
         uint8_t attr = wm_prog_data->urb_setup_attribs[idx];
         int input_index = wm_prog_data->urb_setup[attr];

         assert(0 <= input_index);

         /* gl_Viewport, gl_Layer and FragmentShadingRateKHR are stored in the
          * VUE header
          */
         if (attr == VARYING_SLOT_VIEWPORT ||
             attr == VARYING_SLOT_LAYER ||
             attr == VARYING_SLOT_PRIMITIVE_SHADING_RATE) {
            continue;
         }

         if (attr == VARYING_SLOT_PNTC) {
            sbe.PointSpriteTextureCoordinateEnable = 1 << input_index;
            continue;
         }

         const int slot = fs_input_map->varying_to_slot[attr];

         if (slot == -1) {
            /* This attribute does not exist in the VUE--that means that the
             * vertex shader did not write to it.  It could be that it's a
             * regular varying read by the fragment shader but not written by
             * the vertex shader or it's gl_PrimitiveID. In the first case the
             * value is undefined, in the second it needs to be
             * gl_PrimitiveID.
             */
            swiz.Attribute[input_index].ConstantSource = PRIM_ID;
            swiz.Attribute[input_index].ComponentOverrideX = true;
            swiz.Attribute[input_index].ComponentOverrideY = true;
            swiz.Attribute[input_index].ComponentOverrideZ = true;
            swiz.Attribute[input_index].ComponentOverrideW = true;
            continue;
         }

         /* We have to subtract two slots to accout for the URB entry output
          * read offset in the VS and GS stages.
          */
         const int source_attr = slot - 2 * urb_entry_read_offset;
         assert(source_attr >= 0 && source_attr < 32);
         max_source_attr = MAX2(max_source_attr, source_attr);
         /* The hardware can only do overrides on 16 overrides at a time, and the
          * other up to 16 have to be lined up so that the input index = the
          * output index. We'll need to do some tweaking to make sure that's the
          * case.
          */
         if (input_index < 16)
            swiz.Attribute[input_index].SourceAttribute = source_attr;
         else
            assert(source_attr == input_index);
      }

      sbe.VertexURBEntryReadOffset = urb_entry_read_offset;
      sbe.VertexURBEntryReadLength = DIV_ROUND_UP(max_source_attr + 1, 2);
#if GFX_VER >= 8
      sbe.ForceVertexURBEntryReadOffset = true;
      sbe.ForceVertexURBEntryReadLength = true;
#endif
   }

   uint32_t *dw = anv_batch_emit_dwords(&pipeline->base.batch,
                                        GENX(3DSTATE_SBE_length));
   if (!dw)
      return;
   GENX(3DSTATE_SBE_pack)(&pipeline->base.batch, dw, &sbe);

#if GFX_VER >= 8
   dw = anv_batch_emit_dwords(&pipeline->base.batch, GENX(3DSTATE_SBE_SWIZ_length));
   if (!dw)
      return;
   GENX(3DSTATE_SBE_SWIZ_pack)(&pipeline->base.batch, dw, &swiz);
#endif
}

/** Returns the final polygon mode for rasterization
 *
 * This function takes into account polygon mode, primitive topology and the
 * different shader stages which might generate their own type of primitives.
 */
VkPolygonMode
genX(raster_polygon_mode)(struct anv_graphics_pipeline *pipeline,
                          VkPrimitiveTopology primitive_topology)
{
   if (anv_pipeline_has_stage(pipeline, MESA_SHADER_GEOMETRY)) {
      switch (get_gs_prog_data(pipeline)->output_topology) {
      case _3DPRIM_POINTLIST:
         return VK_POLYGON_MODE_POINT;

      case _3DPRIM_LINELIST:
      case _3DPRIM_LINESTRIP:
      case _3DPRIM_LINELOOP:
         return VK_POLYGON_MODE_LINE;

      case _3DPRIM_TRILIST:
      case _3DPRIM_TRIFAN:
      case _3DPRIM_TRISTRIP:
      case _3DPRIM_RECTLIST:
      case _3DPRIM_QUADLIST:
      case _3DPRIM_QUADSTRIP:
      case _3DPRIM_POLYGON:
         return pipeline->polygon_mode;
      }
      unreachable("Unsupported GS output topology");
   } else if (anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL)) {
      switch (get_tes_prog_data(pipeline)->output_topology) {
      case BRW_TESS_OUTPUT_TOPOLOGY_POINT:
         return VK_POLYGON_MODE_POINT;

      case BRW_TESS_OUTPUT_TOPOLOGY_LINE:
         return VK_POLYGON_MODE_LINE;

      case BRW_TESS_OUTPUT_TOPOLOGY_TRI_CW:
      case BRW_TESS_OUTPUT_TOPOLOGY_TRI_CCW:
         return pipeline->polygon_mode;
      }
      unreachable("Unsupported TCS output topology");
   } else {
      switch (primitive_topology) {
      case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
         return VK_POLYGON_MODE_POINT;

      case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
      case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
      case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
      case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
         return VK_POLYGON_MODE_LINE;

      case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
      case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
      case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
      case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
      case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
         return pipeline->polygon_mode;

      default:
         unreachable("Unsupported primitive topology");
      }
   }
}

uint32_t
genX(ms_rasterization_mode)(struct anv_graphics_pipeline *pipeline,
                            VkPolygonMode raster_mode)
{
#if GFX_VER <= 7
   if (raster_mode == VK_POLYGON_MODE_LINE) {
      switch (pipeline->line_mode) {
      case VK_LINE_RASTERIZATION_MODE_RECTANGULAR_EXT:
         return MSRASTMODE_ON_PATTERN;

      case VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT:
      case VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT:
         return MSRASTMODE_OFF_PIXEL;

      default:
         unreachable("Unsupported line rasterization mode");
      }
   } else {
      return pipeline->rasterization_samples > 1 ?
         MSRASTMODE_ON_PATTERN : MSRASTMODE_OFF_PIXEL;
   }
#else
   unreachable("Only on gen7");
#endif
}

static VkProvokingVertexModeEXT
vk_provoking_vertex_mode(const VkPipelineRasterizationStateCreateInfo *rs_info)
{
   const VkPipelineRasterizationProvokingVertexStateCreateInfoEXT *rs_pv_info =
      vk_find_struct_const(rs_info, PIPELINE_RASTERIZATION_PROVOKING_VERTEX_STATE_CREATE_INFO_EXT);

   return rs_pv_info == NULL ? VK_PROVOKING_VERTEX_MODE_FIRST_VERTEX_EXT :
                               rs_pv_info->provokingVertexMode;
}

const uint32_t genX(vk_to_intel_cullmode)[] = {
   [VK_CULL_MODE_NONE]                       = CULLMODE_NONE,
   [VK_CULL_MODE_FRONT_BIT]                  = CULLMODE_FRONT,
   [VK_CULL_MODE_BACK_BIT]                   = CULLMODE_BACK,
   [VK_CULL_MODE_FRONT_AND_BACK]             = CULLMODE_BOTH
};

const uint32_t genX(vk_to_intel_fillmode)[] = {
   [VK_POLYGON_MODE_FILL]                    = FILL_MODE_SOLID,
   [VK_POLYGON_MODE_LINE]                    = FILL_MODE_WIREFRAME,
   [VK_POLYGON_MODE_POINT]                   = FILL_MODE_POINT,
};

const uint32_t genX(vk_to_intel_front_face)[] = {
   [VK_FRONT_FACE_COUNTER_CLOCKWISE]         = 1,
   [VK_FRONT_FACE_CLOCKWISE]                 = 0
};

#if GFX_VER >= 9
static VkConservativeRasterizationModeEXT
vk_conservative_rasterization_mode(const VkPipelineRasterizationStateCreateInfo *rs_info)
{
   const VkPipelineRasterizationConservativeStateCreateInfoEXT *cr =
      vk_find_struct_const(rs_info, PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT);

   return cr ? cr->conservativeRasterizationMode :
               VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT;
}
#endif

void
genX(rasterization_mode)(VkPolygonMode raster_mode,
                         VkLineRasterizationModeEXT line_mode,
                         float line_width,
                         uint32_t *api_mode,
                         bool *msaa_rasterization_enable)
{
#if GFX_VER >= 8
   if (raster_mode == VK_POLYGON_MODE_LINE) {
      /* Unfortunately, configuring our line rasterization hardware on gfx8
       * and later is rather painful.  Instead of giving us bits to tell the
       * hardware what line mode to use like we had on gfx7, we now have an
       * arcane combination of API Mode and MSAA enable bits which do things
       * in a table which are expected to magically put the hardware into the
       * right mode for your API.  Sadly, Vulkan isn't any of the APIs the
       * hardware people thought of so nothing works the way you want it to.
       *
       * Look at the table titled "Multisample Rasterization Modes" in Vol 7
       * of the Skylake PRM for more details.
       */
      switch (line_mode) {
      case VK_LINE_RASTERIZATION_MODE_RECTANGULAR_EXT:
         *api_mode = DX100;
#if GFX_VER <= 9
         /* Prior to ICL, the algorithm the HW uses to draw wide lines
          * doesn't quite match what the CTS expects, at least for rectangular
          * lines, so we set this to false here, making it draw parallelograms
          * instead, which work well enough.
          */
         *msaa_rasterization_enable = line_width < 1.0078125;
#else
         *msaa_rasterization_enable = true;
#endif
         break;

      case VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT:
      case VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT:
         *api_mode = DX9OGL;
         *msaa_rasterization_enable = false;
         break;

      default:
         unreachable("Unsupported line rasterization mode");
      }
   } else {
      *api_mode = DX100;
      *msaa_rasterization_enable = true;
   }
#else
   unreachable("Invalid call");
#endif
}

static void
emit_rs_state(struct anv_graphics_pipeline *pipeline,
              const VkPipelineInputAssemblyStateCreateInfo *ia_info,
              const VkPipelineRasterizationStateCreateInfo *rs_info,
              const VkPipelineMultisampleStateCreateInfo *ms_info,
              const VkPipelineRasterizationLineStateCreateInfoEXT *line_info,
              const uint32_t dynamic_states,
              const struct anv_render_pass *pass,
              const struct anv_subpass *subpass,
              enum intel_urb_deref_block_size urb_deref_block_size)
{
   struct GENX(3DSTATE_SF) sf = {
      GENX(3DSTATE_SF_header),
   };

   sf.ViewportTransformEnable = true;
   sf.StatisticsEnable = true;
   sf.VertexSubPixelPrecisionSelect = _8Bit;
   sf.AALineDistanceMode = true;

   switch (vk_provoking_vertex_mode(rs_info)) {
   case VK_PROVOKING_VERTEX_MODE_FIRST_VERTEX_EXT:
      sf.TriangleStripListProvokingVertexSelect = 0;
      sf.LineStripListProvokingVertexSelect = 0;
      sf.TriangleFanProvokingVertexSelect = 1;
      break;

   case VK_PROVOKING_VERTEX_MODE_LAST_VERTEX_EXT:
      sf.TriangleStripListProvokingVertexSelect = 2;
      sf.LineStripListProvokingVertexSelect = 1;
      sf.TriangleFanProvokingVertexSelect = 2;
      break;

   default:
      unreachable("Invalid provoking vertex mode");
   }

#if GFX_VERx10 == 75
   sf.LineStippleEnable = line_info && line_info->stippledLineEnable;
#endif

#if GFX_VER >= 12
   sf.DerefBlockSize = urb_deref_block_size;
#endif

   if (anv_pipeline_is_primitive(pipeline)) {
      const struct brw_vue_prog_data *last_vue_prog_data =
         anv_pipeline_get_last_vue_prog_data(pipeline);

      if (last_vue_prog_data->vue_map.slots_valid & VARYING_BIT_PSIZ) {
         sf.PointWidthSource = Vertex;
      } else {
         sf.PointWidthSource = State;
         sf.PointWidth = 1.0;
      }
   }

#if GFX_VER >= 8
   struct GENX(3DSTATE_RASTER) raster = {
      GENX(3DSTATE_RASTER_header),
   };
#else
#  define raster sf
#endif

   VkPolygonMode raster_mode =
      genX(raster_polygon_mode)(pipeline, ia_info->topology);
   bool dynamic_primitive_topology =
      dynamic_states & ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY;

   /* For details on 3DSTATE_RASTER multisample state, see the BSpec table
    * "Multisample Modes State".
    */
#if GFX_VER >= 8
   if (!dynamic_primitive_topology)
      genX(rasterization_mode)(raster_mode, pipeline->line_mode,
                               rs_info->lineWidth,
                               &raster.APIMode,
                               &raster.DXMultisampleRasterizationEnable);

   /* NOTE: 3DSTATE_RASTER::ForcedSampleCount affects the BDW and SKL PMA fix
    * computations.  If we ever set this bit to a different value, they will
    * need to be updated accordingly.
    */
   raster.ForcedSampleCount = FSC_NUMRASTSAMPLES_0;
   raster.ForceMultisampling = false;
#else
   uint32_t ms_rast_mode = 0;

   if (!dynamic_primitive_topology)
      ms_rast_mode = genX(ms_rasterization_mode)(pipeline, raster_mode);

   raster.MultisampleRasterizationMode = ms_rast_mode;
#endif

   raster.AntialiasingEnable =
      dynamic_primitive_topology ? 0 :
      anv_rasterization_aa_mode(raster_mode, pipeline->line_mode);

   raster.FrontWinding =
      dynamic_states & ANV_CMD_DIRTY_DYNAMIC_FRONT_FACE ?
         0 : genX(vk_to_intel_front_face)[rs_info->frontFace];
   raster.CullMode =
      dynamic_states & ANV_CMD_DIRTY_DYNAMIC_CULL_MODE ?
         0 : genX(vk_to_intel_cullmode)[rs_info->cullMode];

   raster.FrontFaceFillMode = genX(vk_to_intel_fillmode)[rs_info->polygonMode];
   raster.BackFaceFillMode = genX(vk_to_intel_fillmode)[rs_info->polygonMode];
   raster.ScissorRectangleEnable = true;

#if GFX_VER >= 9
   /* GFX9+ splits ViewportZClipTestEnable into near and far enable bits */
   raster.ViewportZFarClipTestEnable = pipeline->depth_clip_enable;
   raster.ViewportZNearClipTestEnable = pipeline->depth_clip_enable;
#elif GFX_VER >= 8
   raster.ViewportZClipTestEnable = pipeline->depth_clip_enable;
#endif

#if GFX_VER >= 9
   raster.ConservativeRasterizationEnable =
      vk_conservative_rasterization_mode(rs_info) !=
         VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT;
#endif

   bool depth_bias_enable =
      dynamic_states & ANV_CMD_DIRTY_DYNAMIC_DEPTH_BIAS_ENABLE ?
         0 : rs_info->depthBiasEnable;

   raster.GlobalDepthOffsetEnableSolid = depth_bias_enable;
   raster.GlobalDepthOffsetEnableWireframe = depth_bias_enable;
   raster.GlobalDepthOffsetEnablePoint = depth_bias_enable;

#if GFX_VER == 7
   /* Gfx7 requires that we provide the depth format in 3DSTATE_SF so that it
    * can get the depth offsets correct.
    */
   if (subpass->depth_stencil_attachment) {
      VkFormat vk_format =
         pass->attachments[subpass->depth_stencil_attachment->attachment].format;
      assert(vk_format_is_depth_or_stencil(vk_format));
      if (vk_format_aspects(vk_format) & VK_IMAGE_ASPECT_DEPTH_BIT) {
         enum isl_format isl_format =
            anv_get_isl_format(&pipeline->base.device->info, vk_format,
                               VK_IMAGE_ASPECT_DEPTH_BIT,
                               VK_IMAGE_TILING_OPTIMAL);
         sf.DepthBufferSurfaceFormat =
            isl_format_get_depth_format(isl_format, false);
      }
   }
#endif

#if GFX_VER >= 8
   GENX(3DSTATE_SF_pack)(NULL, pipeline->gfx8.sf, &sf);
   GENX(3DSTATE_RASTER_pack)(NULL, pipeline->gfx8.raster, &raster);
#else
#  undef raster
   GENX(3DSTATE_SF_pack)(NULL, &pipeline->gfx7.sf, &sf);
#endif
}

static void
emit_ms_state(struct anv_graphics_pipeline *pipeline,
              const VkPipelineMultisampleStateCreateInfo *info,
              uint32_t dynamic_states)
{
   /* Only lookup locations if the extensions is active, otherwise the default
    * ones will be used either at device initialization time or through
    * 3DSTATE_MULTISAMPLE on Gfx7/7.5 by passing NULL locations.
    */
   if (pipeline->base.device->vk.enabled_extensions.EXT_sample_locations) {
      /* If the sample locations are dynamic, 3DSTATE_MULTISAMPLE on Gfx7/7.5
       * will be emitted dynamically, so skip it here. On Gfx8+
       * 3DSTATE_SAMPLE_PATTERN will be emitted dynamically, so skip it here.
       */
      if (!(dynamic_states & ANV_CMD_DIRTY_DYNAMIC_SAMPLE_LOCATIONS)) {
#if GFX_VER >= 8
         genX(emit_sample_pattern)(&pipeline->base.batch,
                                   pipeline->dynamic_state.sample_locations.samples,
                                   pipeline->dynamic_state.sample_locations.locations);
#endif
      }

      genX(emit_multisample)(&pipeline->base.batch,
                             pipeline->dynamic_state.sample_locations.samples,
                             pipeline->dynamic_state.sample_locations.locations);
   } else {
      /* On Gfx8+ 3DSTATE_MULTISAMPLE does not hold anything we need to modify
       * for sample locations, so we don't have to emit it dynamically.
       */
#if GFX_VER >= 8
      genX(emit_multisample)(&pipeline->base.batch,
                             info ? info->rasterizationSamples : 1,
                             NULL);
#endif
   }

   /* From the Vulkan 1.0 spec:
    *    If pSampleMask is NULL, it is treated as if the mask has all bits
    *    enabled, i.e. no coverage is removed from fragments.
    *
    * 3DSTATE_SAMPLE_MASK.SampleMask is 16 bits.
    */
#if GFX_VER >= 8
   uint32_t sample_mask = 0xffff;
#else
   uint32_t sample_mask = 0xff;
#endif

   if (info && info->pSampleMask)
      sample_mask &= info->pSampleMask[0];

   anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_SAMPLE_MASK), sm) {
      sm.SampleMask = sample_mask;
   }

   pipeline->cps_state = ANV_STATE_NULL;
#if GFX_VER >= 11
   if (!(dynamic_states & ANV_CMD_DIRTY_DYNAMIC_SHADING_RATE) &&
       pipeline->base.device->vk.enabled_extensions.KHR_fragment_shading_rate) {
#if GFX_VER >= 12
      struct anv_device *device = pipeline->base.device;
      const uint32_t num_dwords =
         GENX(CPS_STATE_length) * 4 * pipeline->dynamic_state.viewport.count;
      pipeline->cps_state =
         anv_state_pool_alloc(&device->dynamic_state_pool, num_dwords, 32);
#endif

      genX(emit_shading_rate)(&pipeline->base.batch,
                              pipeline,
                              pipeline->cps_state,
                              &pipeline->dynamic_state);
   }
#endif
}

const uint32_t genX(vk_to_intel_logic_op)[] = {
   [VK_LOGIC_OP_COPY]                        = LOGICOP_COPY,
   [VK_LOGIC_OP_CLEAR]                       = LOGICOP_CLEAR,
   [VK_LOGIC_OP_AND]                         = LOGICOP_AND,
   [VK_LOGIC_OP_AND_REVERSE]                 = LOGICOP_AND_REVERSE,
   [VK_LOGIC_OP_AND_INVERTED]                = LOGICOP_AND_INVERTED,
   [VK_LOGIC_OP_NO_OP]                       = LOGICOP_NOOP,
   [VK_LOGIC_OP_XOR]                         = LOGICOP_XOR,
   [VK_LOGIC_OP_OR]                          = LOGICOP_OR,
   [VK_LOGIC_OP_NOR]                         = LOGICOP_NOR,
   [VK_LOGIC_OP_EQUIVALENT]                  = LOGICOP_EQUIV,
   [VK_LOGIC_OP_INVERT]                      = LOGICOP_INVERT,
   [VK_LOGIC_OP_OR_REVERSE]                  = LOGICOP_OR_REVERSE,
   [VK_LOGIC_OP_COPY_INVERTED]               = LOGICOP_COPY_INVERTED,
   [VK_LOGIC_OP_OR_INVERTED]                 = LOGICOP_OR_INVERTED,
   [VK_LOGIC_OP_NAND]                        = LOGICOP_NAND,
   [VK_LOGIC_OP_SET]                         = LOGICOP_SET,
};

static const uint32_t vk_to_intel_blend[] = {
   [VK_BLEND_FACTOR_ZERO]                    = BLENDFACTOR_ZERO,
   [VK_BLEND_FACTOR_ONE]                     = BLENDFACTOR_ONE,
   [VK_BLEND_FACTOR_SRC_COLOR]               = BLENDFACTOR_SRC_COLOR,
   [VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR]     = BLENDFACTOR_INV_SRC_COLOR,
   [VK_BLEND_FACTOR_DST_COLOR]               = BLENDFACTOR_DST_COLOR,
   [VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR]     = BLENDFACTOR_INV_DST_COLOR,
   [VK_BLEND_FACTOR_SRC_ALPHA]               = BLENDFACTOR_SRC_ALPHA,
   [VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA]     = BLENDFACTOR_INV_SRC_ALPHA,
   [VK_BLEND_FACTOR_DST_ALPHA]               = BLENDFACTOR_DST_ALPHA,
   [VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA]     = BLENDFACTOR_INV_DST_ALPHA,
   [VK_BLEND_FACTOR_CONSTANT_COLOR]          = BLENDFACTOR_CONST_COLOR,
   [VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR]= BLENDFACTOR_INV_CONST_COLOR,
   [VK_BLEND_FACTOR_CONSTANT_ALPHA]          = BLENDFACTOR_CONST_ALPHA,
   [VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA]= BLENDFACTOR_INV_CONST_ALPHA,
   [VK_BLEND_FACTOR_SRC_ALPHA_SATURATE]      = BLENDFACTOR_SRC_ALPHA_SATURATE,
   [VK_BLEND_FACTOR_SRC1_COLOR]              = BLENDFACTOR_SRC1_COLOR,
   [VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR]    = BLENDFACTOR_INV_SRC1_COLOR,
   [VK_BLEND_FACTOR_SRC1_ALPHA]              = BLENDFACTOR_SRC1_ALPHA,
   [VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA]    = BLENDFACTOR_INV_SRC1_ALPHA,
};

static const uint32_t vk_to_intel_blend_op[] = {
   [VK_BLEND_OP_ADD]                         = BLENDFUNCTION_ADD,
   [VK_BLEND_OP_SUBTRACT]                    = BLENDFUNCTION_SUBTRACT,
   [VK_BLEND_OP_REVERSE_SUBTRACT]            = BLENDFUNCTION_REVERSE_SUBTRACT,
   [VK_BLEND_OP_MIN]                         = BLENDFUNCTION_MIN,
   [VK_BLEND_OP_MAX]                         = BLENDFUNCTION_MAX,
};

const uint32_t genX(vk_to_intel_compare_op)[] = {
   [VK_COMPARE_OP_NEVER]                        = PREFILTEROP_NEVER,
   [VK_COMPARE_OP_LESS]                         = PREFILTEROP_LESS,
   [VK_COMPARE_OP_EQUAL]                        = PREFILTEROP_EQUAL,
   [VK_COMPARE_OP_LESS_OR_EQUAL]                = PREFILTEROP_LEQUAL,
   [VK_COMPARE_OP_GREATER]                      = PREFILTEROP_GREATER,
   [VK_COMPARE_OP_NOT_EQUAL]                    = PREFILTEROP_NOTEQUAL,
   [VK_COMPARE_OP_GREATER_OR_EQUAL]             = PREFILTEROP_GEQUAL,
   [VK_COMPARE_OP_ALWAYS]                       = PREFILTEROP_ALWAYS,
};

const uint32_t genX(vk_to_intel_stencil_op)[] = {
   [VK_STENCIL_OP_KEEP]                         = STENCILOP_KEEP,
   [VK_STENCIL_OP_ZERO]                         = STENCILOP_ZERO,
   [VK_STENCIL_OP_REPLACE]                      = STENCILOP_REPLACE,
   [VK_STENCIL_OP_INCREMENT_AND_CLAMP]          = STENCILOP_INCRSAT,
   [VK_STENCIL_OP_DECREMENT_AND_CLAMP]          = STENCILOP_DECRSAT,
   [VK_STENCIL_OP_INVERT]                       = STENCILOP_INVERT,
   [VK_STENCIL_OP_INCREMENT_AND_WRAP]           = STENCILOP_INCR,
   [VK_STENCIL_OP_DECREMENT_AND_WRAP]           = STENCILOP_DECR,
};

const uint32_t genX(vk_to_intel_primitive_type)[] = {
   [VK_PRIMITIVE_TOPOLOGY_POINT_LIST]                    = _3DPRIM_POINTLIST,
   [VK_PRIMITIVE_TOPOLOGY_LINE_LIST]                     = _3DPRIM_LINELIST,
   [VK_PRIMITIVE_TOPOLOGY_LINE_STRIP]                    = _3DPRIM_LINESTRIP,
   [VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST]                 = _3DPRIM_TRILIST,
   [VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP]                = _3DPRIM_TRISTRIP,
   [VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN]                  = _3DPRIM_TRIFAN,
   [VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY]      = _3DPRIM_LINELIST_ADJ,
   [VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY]     = _3DPRIM_LINESTRIP_ADJ,
   [VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY]  = _3DPRIM_TRILIST_ADJ,
   [VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY] = _3DPRIM_TRISTRIP_ADJ,
};

/* This function sanitizes the VkStencilOpState by looking at the compare ops
 * and trying to determine whether or not a given stencil op can ever actually
 * occur.  Stencil ops which can never occur are set to VK_STENCIL_OP_KEEP.
 * This function returns true if, after sanitation, any of the stencil ops are
 * set to something other than VK_STENCIL_OP_KEEP.
 */
static bool
sanitize_stencil_face(VkStencilOpState *face,
                      VkCompareOp depthCompareOp)
{
   /* If compareOp is ALWAYS then the stencil test will never fail and failOp
    * will never happen.  Set failOp to KEEP in this case.
    */
   if (face->compareOp == VK_COMPARE_OP_ALWAYS)
      face->failOp = VK_STENCIL_OP_KEEP;

   /* If compareOp is NEVER or depthCompareOp is NEVER then one of the depth
    * or stencil tests will fail and passOp will never happen.
    */
   if (face->compareOp == VK_COMPARE_OP_NEVER ||
       depthCompareOp == VK_COMPARE_OP_NEVER)
      face->passOp = VK_STENCIL_OP_KEEP;

   /* If compareOp is NEVER or depthCompareOp is ALWAYS then either the
    * stencil test will fail or the depth test will pass.  In either case,
    * depthFailOp will never happen.
    */
   if (face->compareOp == VK_COMPARE_OP_NEVER ||
       depthCompareOp == VK_COMPARE_OP_ALWAYS)
      face->depthFailOp = VK_STENCIL_OP_KEEP;

   return face->failOp != VK_STENCIL_OP_KEEP ||
          face->depthFailOp != VK_STENCIL_OP_KEEP ||
          face->passOp != VK_STENCIL_OP_KEEP;
}

/* Intel hardware is fairly sensitive to whether or not depth/stencil writes
 * are enabled.  In the presence of discards, it's fairly easy to get into the
 * non-promoted case which means a fairly big performance hit.  From the Iron
 * Lake PRM, Vol 2, pt. 1, section 8.4.3.2, "Early Depth Test Cases":
 *
 *    "Non-promoted depth (N) is active whenever the depth test can be done
 *    early but it cannot determine whether or not to write source depth to
 *    the depth buffer, therefore the depth write must be performed post pixel
 *    shader. This includes cases where the pixel shader can kill pixels,
 *    including via sampler chroma key, as well as cases where the alpha test
 *    function is enabled, which kills pixels based on a programmable alpha
 *    test. In this case, even if the depth test fails, the pixel cannot be
 *    killed if a stencil write is indicated. Whether or not the stencil write
 *    happens depends on whether or not the pixel is killed later. In these
 *    cases if stencil test fails and stencil writes are off, the pixels can
 *    also be killed early. If stencil writes are enabled, the pixels must be
 *    treated as Computed depth (described above)."
 *
 * The same thing as mentioned in the stencil case can happen in the depth
 * case as well if it thinks it writes depth but, thanks to the depth test
 * being GL_EQUAL, the write doesn't actually matter.  A little extra work
 * up-front to try and disable depth and stencil writes can make a big
 * difference.
 *
 * Unfortunately, the way depth and stencil testing is specified, there are
 * many case where, regardless of depth/stencil writes being enabled, nothing
 * actually gets written due to some other bit of state being set.  This
 * function attempts to "sanitize" the depth stencil state and disable writes
 * and sometimes even testing whenever possible.
 */
static void
sanitize_ds_state(VkPipelineDepthStencilStateCreateInfo *state,
                  bool *stencilWriteEnable,
                  VkImageAspectFlags ds_aspects)
{
   *stencilWriteEnable = state->stencilTestEnable;

   /* If the depth test is disabled, we won't be writing anything. Make sure we
    * treat the test as always passing later on as well.
    *
    * Also, the Vulkan spec requires that if either depth or stencil is not
    * present, the pipeline is to act as if the test silently passes. In that
    * case we won't write either.
    */
   if (!state->depthTestEnable || !(ds_aspects & VK_IMAGE_ASPECT_DEPTH_BIT)) {
      state->depthWriteEnable = false;
      state->depthCompareOp = VK_COMPARE_OP_ALWAYS;
   }

   if (!(ds_aspects & VK_IMAGE_ASPECT_STENCIL_BIT)) {
      *stencilWriteEnable = false;
      state->front.compareOp = VK_COMPARE_OP_ALWAYS;
      state->back.compareOp = VK_COMPARE_OP_ALWAYS;
   }

   /* If the stencil test is enabled and always fails, then we will never get
    * to the depth test so we can just disable the depth test entirely.
    */
   if (state->stencilTestEnable &&
       state->front.compareOp == VK_COMPARE_OP_NEVER &&
       state->back.compareOp == VK_COMPARE_OP_NEVER) {
      state->depthTestEnable = false;
      state->depthWriteEnable = false;
   }

   /* If depthCompareOp is EQUAL then the value we would be writing to the
    * depth buffer is the same as the value that's already there so there's no
    * point in writing it.
    */
   if (state->depthCompareOp == VK_COMPARE_OP_EQUAL)
      state->depthWriteEnable = false;

   /* If the stencil ops are such that we don't actually ever modify the
    * stencil buffer, we should disable writes.
    */
   if (!sanitize_stencil_face(&state->front, state->depthCompareOp) &&
       !sanitize_stencil_face(&state->back, state->depthCompareOp))
      *stencilWriteEnable = false;

   /* If the depth test always passes and we never write out depth, that's the
    * same as if the depth test is disabled entirely.
    */
   if (state->depthCompareOp == VK_COMPARE_OP_ALWAYS &&
       !state->depthWriteEnable)
      state->depthTestEnable = false;

   /* If the stencil test always passes and we never write out stencil, that's
    * the same as if the stencil test is disabled entirely.
    */
   if (state->front.compareOp == VK_COMPARE_OP_ALWAYS &&
       state->back.compareOp == VK_COMPARE_OP_ALWAYS &&
       !*stencilWriteEnable)
      state->stencilTestEnable = false;
}

static void
emit_ds_state(struct anv_graphics_pipeline *pipeline,
              const VkPipelineDepthStencilStateCreateInfo *pCreateInfo,
              const uint32_t dynamic_states,
              const struct anv_render_pass *pass,
              const struct anv_subpass *subpass)
{
#if GFX_VER == 7
#  define depth_stencil_dw pipeline->gfx7.depth_stencil_state
#elif GFX_VER == 8
#  define depth_stencil_dw pipeline->gfx8.wm_depth_stencil
#else
#  define depth_stencil_dw pipeline->gfx9.wm_depth_stencil
#endif

   if (pCreateInfo == NULL) {
      /* We're going to OR this together with the dynamic state.  We need
       * to make sure it's initialized to something useful.
       */
      pipeline->writes_stencil = false;
      pipeline->stencil_test_enable = false;
      pipeline->writes_depth = false;
      pipeline->depth_test_enable = false;
      pipeline->depth_bounds_test_enable = false;
      memset(depth_stencil_dw, 0, sizeof(depth_stencil_dw));
      return;
   }

   VkImageAspectFlags ds_aspects = 0;
   if (subpass->depth_stencil_attachment) {
      VkFormat depth_stencil_format =
         pass->attachments[subpass->depth_stencil_attachment->attachment].format;
      ds_aspects = vk_format_aspects(depth_stencil_format);
   }

   VkPipelineDepthStencilStateCreateInfo info = *pCreateInfo;
   sanitize_ds_state(&info, &pipeline->writes_stencil, ds_aspects);
   pipeline->stencil_test_enable = info.stencilTestEnable;
   pipeline->writes_depth = info.depthWriteEnable;
   pipeline->depth_test_enable = info.depthTestEnable;
   pipeline->depth_bounds_test_enable = info.depthBoundsTestEnable;

   bool dynamic_stencil_op =
      dynamic_states & ANV_CMD_DIRTY_DYNAMIC_STENCIL_OP;

#if GFX_VER <= 7
   struct GENX(DEPTH_STENCIL_STATE) depth_stencil = {
#else
   struct GENX(3DSTATE_WM_DEPTH_STENCIL) depth_stencil = {
#endif
      .DepthTestEnable =
         dynamic_states & ANV_CMD_DIRTY_DYNAMIC_DEPTH_TEST_ENABLE ?
            0 : info.depthTestEnable,

      .DepthBufferWriteEnable =
         dynamic_states & ANV_CMD_DIRTY_DYNAMIC_DEPTH_WRITE_ENABLE ?
            0 : info.depthWriteEnable,

      .DepthTestFunction =
         dynamic_states & ANV_CMD_DIRTY_DYNAMIC_DEPTH_COMPARE_OP ?
            0 : genX(vk_to_intel_compare_op)[info.depthCompareOp],

      .DoubleSidedStencilEnable = true,

      .StencilTestEnable =
         dynamic_states & ANV_CMD_DIRTY_DYNAMIC_STENCIL_TEST_ENABLE ?
            0 : info.stencilTestEnable,

      .StencilFailOp = genX(vk_to_intel_stencil_op)[info.front.failOp],
      .StencilPassDepthPassOp = genX(vk_to_intel_stencil_op)[info.front.passOp],
      .StencilPassDepthFailOp = genX(vk_to_intel_stencil_op)[info.front.depthFailOp],
      .StencilTestFunction = genX(vk_to_intel_compare_op)[info.front.compareOp],
      .BackfaceStencilFailOp = genX(vk_to_intel_stencil_op)[info.back.failOp],
      .BackfaceStencilPassDepthPassOp = genX(vk_to_intel_stencil_op)[info.back.passOp],
      .BackfaceStencilPassDepthFailOp = genX(vk_to_intel_stencil_op)[info.back.depthFailOp],
      .BackfaceStencilTestFunction = genX(vk_to_intel_compare_op)[info.back.compareOp],
   };

   if (dynamic_stencil_op) {
      depth_stencil.StencilFailOp = 0;
      depth_stencil.StencilPassDepthPassOp = 0;
      depth_stencil.StencilPassDepthFailOp = 0;
      depth_stencil.StencilTestFunction = 0;
      depth_stencil.BackfaceStencilFailOp = 0;
      depth_stencil.BackfaceStencilPassDepthPassOp = 0;
      depth_stencil.BackfaceStencilPassDepthFailOp = 0;
      depth_stencil.BackfaceStencilTestFunction = 0;
   }

#if GFX_VER <= 7
   GENX(DEPTH_STENCIL_STATE_pack)(NULL, depth_stencil_dw, &depth_stencil);
#else
   GENX(3DSTATE_WM_DEPTH_STENCIL_pack)(NULL, depth_stencil_dw, &depth_stencil);
#endif
}

static bool
is_dual_src_blend_factor(VkBlendFactor factor)
{
   return factor == VK_BLEND_FACTOR_SRC1_COLOR ||
          factor == VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR ||
          factor == VK_BLEND_FACTOR_SRC1_ALPHA ||
          factor == VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
}

static inline uint32_t *
write_disabled_blend(uint32_t *state)
{
   struct GENX(BLEND_STATE_ENTRY) entry = {
      .WriteDisableAlpha = true,
      .WriteDisableRed = true,
      .WriteDisableGreen = true,
      .WriteDisableBlue = true,
   };
   GENX(BLEND_STATE_ENTRY_pack)(NULL, state, &entry);
   return state + GENX(BLEND_STATE_ENTRY_length);
}

static void
emit_cb_state(struct anv_graphics_pipeline *pipeline,
              const VkPipelineColorBlendStateCreateInfo *info,
              const VkPipelineMultisampleStateCreateInfo *ms_info,
              uint32_t dynamic_states)
{
   struct anv_device *device = pipeline->base.device;
   const struct brw_wm_prog_data *wm_prog_data = get_wm_prog_data(pipeline);

   struct GENX(BLEND_STATE) blend_state = {
#if GFX_VER >= 8
      .AlphaToCoverageEnable = ms_info && ms_info->alphaToCoverageEnable,
      .AlphaToOneEnable = ms_info && ms_info->alphaToOneEnable,
#endif
   };

   uint32_t surface_count = 0;
   struct anv_pipeline_bind_map *map;
   if (anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT)) {
      map = &pipeline->shaders[MESA_SHADER_FRAGMENT]->bind_map;
      surface_count = map->surface_count;
   }

   const uint32_t num_dwords = GENX(BLEND_STATE_length) +
      GENX(BLEND_STATE_ENTRY_length) * surface_count;
   uint32_t *blend_state_start, *state_pos;

   if (dynamic_states & (ANV_CMD_DIRTY_DYNAMIC_COLOR_BLEND_STATE |
                         ANV_CMD_DIRTY_DYNAMIC_LOGIC_OP)) {
      const struct intel_device_info *devinfo = &pipeline->base.device->info;
      blend_state_start = devinfo->ver >= 8 ?
         pipeline->gfx8.blend_state : pipeline->gfx7.blend_state;
      pipeline->blend_state = ANV_STATE_NULL;
   } else {
      pipeline->blend_state =
         anv_state_pool_alloc(&device->dynamic_state_pool, num_dwords * 4, 64);
      blend_state_start = pipeline->blend_state.map;
   }
   state_pos = blend_state_start;

   bool has_writeable_rt = false;
   state_pos += GENX(BLEND_STATE_length);
#if GFX_VER >= 8
   struct GENX(BLEND_STATE_ENTRY) bs0 = { 0 };
#endif
   for (unsigned i = 0; i < surface_count; i++) {
      struct anv_pipeline_binding *binding = &map->surface_to_descriptor[i];

      /* All color attachments are at the beginning of the binding table */
      if (binding->set != ANV_DESCRIPTOR_SET_COLOR_ATTACHMENTS)
         break;

      /* We can have at most 8 attachments */
      assert(i < MAX_RTS);

      if (info == NULL || binding->index >= info->attachmentCount) {
         state_pos = write_disabled_blend(state_pos);
         continue;
      }

      if ((pipeline->dynamic_state.color_writes & (1u << binding->index)) == 0) {
         state_pos = write_disabled_blend(state_pos);
         continue;
      }

      const VkPipelineColorBlendAttachmentState *a =
         &info->pAttachments[binding->index];

      struct GENX(BLEND_STATE_ENTRY) entry = {
#if GFX_VER < 8
         .AlphaToCoverageEnable = ms_info && ms_info->alphaToCoverageEnable,
         .AlphaToOneEnable = ms_info && ms_info->alphaToOneEnable,
#endif
         .LogicOpEnable = info->logicOpEnable,
         .LogicOpFunction = dynamic_states & ANV_CMD_DIRTY_DYNAMIC_LOGIC_OP ?
                            0: genX(vk_to_intel_logic_op)[info->logicOp],

         /* Vulkan specification 1.2.168, VkLogicOp:
          *
          *   "Logical operations are controlled by the logicOpEnable and
          *    logicOp members of VkPipelineColorBlendStateCreateInfo. If
          *    logicOpEnable is VK_TRUE, then a logical operation selected by
          *    logicOp is applied between each color attachment and the
          *    fragment’s corresponding output value, and blending of all
          *    attachments is treated as if it were disabled."
          *
          * From the Broadwell PRM Volume 2d: Command Reference: Structures:
          * BLEND_STATE_ENTRY:
          *
          *   "Enabling LogicOp and Color Buffer Blending at the same time is
          *    UNDEFINED"
          */
         .ColorBufferBlendEnable = !info->logicOpEnable && a->blendEnable,
         .ColorClampRange = COLORCLAMP_RTFORMAT,
         .PreBlendColorClampEnable = true,
         .PostBlendColorClampEnable = true,
         .SourceBlendFactor = vk_to_intel_blend[a->srcColorBlendFactor],
         .DestinationBlendFactor = vk_to_intel_blend[a->dstColorBlendFactor],
         .ColorBlendFunction = vk_to_intel_blend_op[a->colorBlendOp],
         .SourceAlphaBlendFactor = vk_to_intel_blend[a->srcAlphaBlendFactor],
         .DestinationAlphaBlendFactor = vk_to_intel_blend[a->dstAlphaBlendFactor],
         .AlphaBlendFunction = vk_to_intel_blend_op[a->alphaBlendOp],
         .WriteDisableAlpha = !(a->colorWriteMask & VK_COLOR_COMPONENT_A_BIT),
         .WriteDisableRed = !(a->colorWriteMask & VK_COLOR_COMPONENT_R_BIT),
         .WriteDisableGreen = !(a->colorWriteMask & VK_COLOR_COMPONENT_G_BIT),
         .WriteDisableBlue = !(a->colorWriteMask & VK_COLOR_COMPONENT_B_BIT),
      };

      if (a->srcColorBlendFactor != a->srcAlphaBlendFactor ||
          a->dstColorBlendFactor != a->dstAlphaBlendFactor ||
          a->colorBlendOp != a->alphaBlendOp) {
#if GFX_VER >= 8
         blend_state.IndependentAlphaBlendEnable = true;
#else
         entry.IndependentAlphaBlendEnable = true;
#endif
      }

      /* The Dual Source Blending documentation says:
       *
       * "If SRC1 is included in a src/dst blend factor and
       * a DualSource RT Write message is not used, results
       * are UNDEFINED. (This reflects the same restriction in DX APIs,
       * where undefined results are produced if “o1” is not written
       * by a PS – there are no default values defined)."
       *
       * There is no way to gracefully fix this undefined situation
       * so we just disable the blending to prevent possible issues.
       */
      if (!wm_prog_data->dual_src_blend &&
          (is_dual_src_blend_factor(a->srcColorBlendFactor) ||
           is_dual_src_blend_factor(a->dstColorBlendFactor) ||
           is_dual_src_blend_factor(a->srcAlphaBlendFactor) ||
           is_dual_src_blend_factor(a->dstAlphaBlendFactor))) {
         vk_logw(VK_LOG_OBJS(&device->vk.base),
                 "Enabled dual-src blend factors without writing both targets "
                 "in the shader.  Disabling blending to avoid GPU hangs.");
         entry.ColorBufferBlendEnable = false;
      }

      if (a->colorWriteMask != 0)
         has_writeable_rt = true;

      /* Our hardware applies the blend factor prior to the blend function
       * regardless of what function is used.  Technically, this means the
       * hardware can do MORE than GL or Vulkan specify.  However, it also
       * means that, for MIN and MAX, we have to stomp the blend factor to
       * ONE to make it a no-op.
       */
      if (a->colorBlendOp == VK_BLEND_OP_MIN ||
          a->colorBlendOp == VK_BLEND_OP_MAX) {
         entry.SourceBlendFactor = BLENDFACTOR_ONE;
         entry.DestinationBlendFactor = BLENDFACTOR_ONE;
      }
      if (a->alphaBlendOp == VK_BLEND_OP_MIN ||
          a->alphaBlendOp == VK_BLEND_OP_MAX) {
         entry.SourceAlphaBlendFactor = BLENDFACTOR_ONE;
         entry.DestinationAlphaBlendFactor = BLENDFACTOR_ONE;
      }
      GENX(BLEND_STATE_ENTRY_pack)(NULL, state_pos, &entry);
      state_pos += GENX(BLEND_STATE_ENTRY_length);
#if GFX_VER >= 8
      if (i == 0)
         bs0 = entry;
#endif
   }

#if GFX_VER >= 8
   struct GENX(3DSTATE_PS_BLEND) blend = {
      GENX(3DSTATE_PS_BLEND_header),
   };
   blend.AlphaToCoverageEnable         = blend_state.AlphaToCoverageEnable;
   blend.HasWriteableRT                = has_writeable_rt;
   blend.ColorBufferBlendEnable        = bs0.ColorBufferBlendEnable;
   blend.SourceAlphaBlendFactor        = bs0.SourceAlphaBlendFactor;
   blend.DestinationAlphaBlendFactor   = bs0.DestinationAlphaBlendFactor;
   blend.SourceBlendFactor             = bs0.SourceBlendFactor;
   blend.DestinationBlendFactor        = bs0.DestinationBlendFactor;
   blend.AlphaTestEnable               = false;
   blend.IndependentAlphaBlendEnable   = blend_state.IndependentAlphaBlendEnable;

   if (dynamic_states & (ANV_CMD_DIRTY_DYNAMIC_COLOR_BLEND_STATE |
                        ANV_CMD_DIRTY_DYNAMIC_LOGIC_OP)) {
      GENX(3DSTATE_PS_BLEND_pack)(NULL, pipeline->gfx8.ps_blend, &blend);
   } else {
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_PS_BLEND), _blend)
         _blend = blend;
   }
#else
   (void)has_writeable_rt;
#endif

   GENX(BLEND_STATE_pack)(NULL, blend_state_start, &blend_state);

   if (!(dynamic_states & (ANV_CMD_DIRTY_DYNAMIC_COLOR_BLEND_STATE |
                           ANV_CMD_DIRTY_DYNAMIC_LOGIC_OP))) {
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_BLEND_STATE_POINTERS), bsp) {
         bsp.BlendStatePointer      = pipeline->blend_state.offset;
#if GFX_VER >= 8
         bsp.BlendStatePointerValid = true;
#endif
      }
   }
}

static void
emit_3dstate_clip(struct anv_graphics_pipeline *pipeline,
                  const VkPipelineInputAssemblyStateCreateInfo *ia_info,
                  const VkPipelineViewportStateCreateInfo *vp_info,
                  const VkPipelineRasterizationStateCreateInfo *rs_info,
                  const uint32_t dynamic_states)
{
   const struct brw_wm_prog_data *wm_prog_data = get_wm_prog_data(pipeline);
   (void) wm_prog_data;

   struct GENX(3DSTATE_CLIP) clip = {
      GENX(3DSTATE_CLIP_header),
   };

   clip.ClipEnable               = true;
   clip.StatisticsEnable         = true;
   clip.EarlyCullEnable          = true;
   clip.APIMode                  = APIMODE_D3D;
   clip.GuardbandClipTestEnable  = true;

   /* Only enable the XY clip test when the final polygon rasterization
    * mode is VK_POLYGON_MODE_FILL.  We want to leave it disabled for
    * points and lines so we get "pop-free" clipping.
    */
   VkPolygonMode raster_mode =
      genX(raster_polygon_mode)(pipeline, ia_info->topology);
   clip.ViewportXYClipTestEnable =
      dynamic_states & ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY ?
         0 : (raster_mode == VK_POLYGON_MODE_FILL);

#if GFX_VER >= 8
   clip.VertexSubPixelPrecisionSelect = _8Bit;
#endif
   clip.ClipMode = CLIPMODE_NORMAL;

   switch (vk_provoking_vertex_mode(rs_info)) {
   case VK_PROVOKING_VERTEX_MODE_FIRST_VERTEX_EXT:
      clip.TriangleStripListProvokingVertexSelect = 0;
      clip.LineStripListProvokingVertexSelect = 0;
      clip.TriangleFanProvokingVertexSelect = 1;
      break;

   case VK_PROVOKING_VERTEX_MODE_LAST_VERTEX_EXT:
      clip.TriangleStripListProvokingVertexSelect = 2;
      clip.LineStripListProvokingVertexSelect = 1;
      clip.TriangleFanProvokingVertexSelect = 2;
      break;

   default:
      unreachable("Invalid provoking vertex mode");
   }

   clip.MinimumPointWidth = 0.125;
   clip.MaximumPointWidth = 255.875;

   if (anv_pipeline_is_primitive(pipeline)) {
      const struct brw_vue_prog_data *last =
         anv_pipeline_get_last_vue_prog_data(pipeline);

      /* From the Vulkan 1.0.45 spec:
       *
       *    "If the last active vertex processing stage shader entry point's
       *    interface does not include a variable decorated with
       *    ViewportIndex, then the first viewport is used."
       */
      if (vp_info && (last->vue_map.slots_valid & VARYING_BIT_VIEWPORT)) {
         clip.MaximumVPIndex = vp_info->viewportCount > 0 ?
            vp_info->viewportCount - 1 : 0;
      } else {
         clip.MaximumVPIndex = 0;
      }

      /* From the Vulkan 1.0.45 spec:
       *
       *    "If the last active vertex processing stage shader entry point's
       *    interface does not include a variable decorated with Layer, then
       *    the first layer is used."
       */
      clip.ForceZeroRTAIndexEnable =
         !(last->vue_map.slots_valid & VARYING_BIT_LAYER);

#if GFX_VER == 7
      clip.UserClipDistanceClipTestEnableBitmask = last->clip_distance_mask;
      clip.UserClipDistanceCullTestEnableBitmask = last->cull_distance_mask;
#endif
   }

#if GFX_VER == 7
   clip.FrontWinding            = genX(vk_to_intel_front_face)[rs_info->frontFace];
   clip.CullMode                = genX(vk_to_intel_cullmode)[rs_info->cullMode];
   clip.ViewportZClipTestEnable = pipeline->depth_clip_enable;
#else
   clip.NonPerspectiveBarycentricEnable = wm_prog_data ?
      (wm_prog_data->barycentric_interp_modes &
       BRW_BARYCENTRIC_NONPERSPECTIVE_BITS) != 0 : 0;
#endif

   GENX(3DSTATE_CLIP_pack)(NULL, pipeline->gfx7.clip, &clip);
}

static void
emit_3dstate_streamout(struct anv_graphics_pipeline *pipeline,
                       const VkPipelineRasterizationStateCreateInfo *rs_info,
                       const uint32_t dynamic_states)
{
   const struct brw_vue_prog_data *prog_data =
      anv_pipeline_get_last_vue_prog_data(pipeline);
   const struct brw_vue_map *vue_map = &prog_data->vue_map;

   nir_xfb_info *xfb_info;
   if (anv_pipeline_has_stage(pipeline, MESA_SHADER_GEOMETRY))
      xfb_info = pipeline->shaders[MESA_SHADER_GEOMETRY]->xfb_info;
   else if (anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL))
      xfb_info = pipeline->shaders[MESA_SHADER_TESS_EVAL]->xfb_info;
   else
      xfb_info = pipeline->shaders[MESA_SHADER_VERTEX]->xfb_info;

#if GFX_VER == 7
#  define streamout_state_dw pipeline->gfx7.streamout_state
#else
#  define streamout_state_dw pipeline->gfx8.streamout_state
#endif

   struct GENX(3DSTATE_STREAMOUT) so = {
      GENX(3DSTATE_STREAMOUT_header),
      .RenderingDisable =
         (dynamic_states & ANV_CMD_DIRTY_DYNAMIC_RASTERIZER_DISCARD_ENABLE) ?
            0 : rs_info->rasterizerDiscardEnable,
   };

   if (xfb_info) {
      so.SOFunctionEnable = true;
      so.SOStatisticsEnable = true;

      switch (vk_provoking_vertex_mode(rs_info)) {
      case VK_PROVOKING_VERTEX_MODE_FIRST_VERTEX_EXT:
         so.ReorderMode = LEADING;
         break;

      case VK_PROVOKING_VERTEX_MODE_LAST_VERTEX_EXT:
         so.ReorderMode = TRAILING;
         break;

      default:
         unreachable("Invalid provoking vertex mode");
      }

      const VkPipelineRasterizationStateStreamCreateInfoEXT *stream_info =
         vk_find_struct_const(rs_info, PIPELINE_RASTERIZATION_STATE_STREAM_CREATE_INFO_EXT);
      so.RenderStreamSelect = stream_info ?
                              stream_info->rasterizationStream : 0;

#if GFX_VER >= 8
      so.Buffer0SurfacePitch = xfb_info->buffers[0].stride;
      so.Buffer1SurfacePitch = xfb_info->buffers[1].stride;
      so.Buffer2SurfacePitch = xfb_info->buffers[2].stride;
      so.Buffer3SurfacePitch = xfb_info->buffers[3].stride;
#else
      pipeline->gfx7.xfb_bo_pitch[0] = xfb_info->buffers[0].stride;
      pipeline->gfx7.xfb_bo_pitch[1] = xfb_info->buffers[1].stride;
      pipeline->gfx7.xfb_bo_pitch[2] = xfb_info->buffers[2].stride;
      pipeline->gfx7.xfb_bo_pitch[3] = xfb_info->buffers[3].stride;

      /* On Gfx7, the SO buffer enables live in 3DSTATE_STREAMOUT which
       * is a bit inconvenient because we don't know what buffers will
       * actually be enabled until draw time.  We do our best here by
       * setting them based on buffers_written and we disable them
       * as-needed at draw time by setting EndAddress = BaseAddress.
       */
      so.SOBufferEnable0 = xfb_info->buffers_written & (1 << 0);
      so.SOBufferEnable1 = xfb_info->buffers_written & (1 << 1);
      so.SOBufferEnable2 = xfb_info->buffers_written & (1 << 2);
      so.SOBufferEnable3 = xfb_info->buffers_written & (1 << 3);
#endif

      int urb_entry_read_offset = 0;
      int urb_entry_read_length =
         (prog_data->vue_map.num_slots + 1) / 2 - urb_entry_read_offset;

      /* We always read the whole vertex.  This could be reduced at some
       * point by reading less and offsetting the register index in the
       * SO_DECLs.
       */
      so.Stream0VertexReadOffset = urb_entry_read_offset;
      so.Stream0VertexReadLength = urb_entry_read_length - 1;
      so.Stream1VertexReadOffset = urb_entry_read_offset;
      so.Stream1VertexReadLength = urb_entry_read_length - 1;
      so.Stream2VertexReadOffset = urb_entry_read_offset;
      so.Stream2VertexReadLength = urb_entry_read_length - 1;
      so.Stream3VertexReadOffset = urb_entry_read_offset;
      so.Stream3VertexReadLength = urb_entry_read_length - 1;
   }

   if (dynamic_states & ANV_CMD_DIRTY_DYNAMIC_RASTERIZER_DISCARD_ENABLE) {
      GENX(3DSTATE_STREAMOUT_pack)(NULL, streamout_state_dw, &so);
   } else {
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_STREAMOUT), _so)
         _so = so;
   }

   if (xfb_info) {
      struct GENX(SO_DECL) so_decl[MAX_XFB_STREAMS][128];
      int next_offset[MAX_XFB_BUFFERS] = {0, 0, 0, 0};
      int decls[MAX_XFB_STREAMS] = {0, 0, 0, 0};

      memset(so_decl, 0, sizeof(so_decl));

      for (unsigned i = 0; i < xfb_info->output_count; i++) {
         const nir_xfb_output_info *output = &xfb_info->outputs[i];
         unsigned buffer = output->buffer;
         unsigned stream = xfb_info->buffer_to_stream[buffer];

         /* Our hardware is unusual in that it requires us to program SO_DECLs
          * for fake "hole" components, rather than simply taking the offset
          * for each real varying.  Each hole can have size 1, 2, 3, or 4; we
          * program as many size = 4 holes as we can, then a final hole to
          * accommodate the final 1, 2, or 3 remaining.
          */
         int hole_dwords = (output->offset - next_offset[buffer]) / 4;
         while (hole_dwords > 0) {
            so_decl[stream][decls[stream]++] = (struct GENX(SO_DECL)) {
               .HoleFlag = 1,
               .OutputBufferSlot = buffer,
               .ComponentMask = (1 << MIN2(hole_dwords, 4)) - 1,
            };
            hole_dwords -= 4;
         }

         int varying = output->location;
         uint8_t component_mask = output->component_mask;
         /* VARYING_SLOT_PSIZ contains four scalar fields packed together:
          * - VARYING_SLOT_PRIMITIVE_SHADING_RATE in VARYING_SLOT_PSIZ.x
          * - VARYING_SLOT_LAYER                  in VARYING_SLOT_PSIZ.y
          * - VARYING_SLOT_VIEWPORT               in VARYING_SLOT_PSIZ.z
          * - VARYING_SLOT_PSIZ                   in VARYING_SLOT_PSIZ.w
          */
         if (varying == VARYING_SLOT_PRIMITIVE_SHADING_RATE) {
            varying = VARYING_SLOT_PSIZ;
            component_mask = 1 << 0; // SO_DECL_COMPMASK_X
         } else if (varying == VARYING_SLOT_LAYER) {
            varying = VARYING_SLOT_PSIZ;
            component_mask = 1 << 1; // SO_DECL_COMPMASK_Y
         } else if (varying == VARYING_SLOT_VIEWPORT) {
            varying = VARYING_SLOT_PSIZ;
            component_mask = 1 << 2; // SO_DECL_COMPMASK_Z
         } else if (varying == VARYING_SLOT_PSIZ) {
            component_mask = 1 << 3; // SO_DECL_COMPMASK_W
         }

         next_offset[buffer] = output->offset +
                               __builtin_popcount(component_mask) * 4;

         const int slot = vue_map->varying_to_slot[varying];
         if (slot < 0) {
            /* This can happen if the shader never writes to the varying.
             * Insert a hole instead of actual varying data.
             */
            so_decl[stream][decls[stream]++] = (struct GENX(SO_DECL)) {
               .HoleFlag = true,
               .OutputBufferSlot = buffer,
               .ComponentMask = component_mask,
            };
         } else {
            so_decl[stream][decls[stream]++] = (struct GENX(SO_DECL)) {
               .OutputBufferSlot = buffer,
               .RegisterIndex = slot,
               .ComponentMask = component_mask,
            };
         }
      }

      int max_decls = 0;
      for (unsigned s = 0; s < MAX_XFB_STREAMS; s++)
         max_decls = MAX2(max_decls, decls[s]);

      uint8_t sbs[MAX_XFB_STREAMS] = { };
      for (unsigned b = 0; b < MAX_XFB_BUFFERS; b++) {
         if (xfb_info->buffers_written & (1 << b))
            sbs[xfb_info->buffer_to_stream[b]] |= 1 << b;
      }

      uint32_t *dw = anv_batch_emitn(&pipeline->base.batch, 3 + 2 * max_decls,
                                     GENX(3DSTATE_SO_DECL_LIST),
                                     .StreamtoBufferSelects0 = sbs[0],
                                     .StreamtoBufferSelects1 = sbs[1],
                                     .StreamtoBufferSelects2 = sbs[2],
                                     .StreamtoBufferSelects3 = sbs[3],
                                     .NumEntries0 = decls[0],
                                     .NumEntries1 = decls[1],
                                     .NumEntries2 = decls[2],
                                     .NumEntries3 = decls[3]);

      for (int i = 0; i < max_decls; i++) {
         GENX(SO_DECL_ENTRY_pack)(NULL, dw + 3 + i * 2,
            &(struct GENX(SO_DECL_ENTRY)) {
               .Stream0Decl = so_decl[0][i],
               .Stream1Decl = so_decl[1][i],
               .Stream2Decl = so_decl[2][i],
               .Stream3Decl = so_decl[3][i],
            });
      }
   }
}

static uint32_t
get_sampler_count(const struct anv_shader_bin *bin)
{
   uint32_t count_by_4 = DIV_ROUND_UP(bin->bind_map.sampler_count, 4);

   /* We can potentially have way more than 32 samplers and that's ok.
    * However, the 3DSTATE_XS packets only have 3 bits to specify how
    * many to pre-fetch and all values above 4 are marked reserved.
    */
   return MIN2(count_by_4, 4);
}

static UNUSED struct anv_address
get_scratch_address(struct anv_pipeline *pipeline,
                    gl_shader_stage stage,
                    const struct anv_shader_bin *bin)
{
   return (struct anv_address) {
      .bo = anv_scratch_pool_alloc(pipeline->device,
                                   &pipeline->device->scratch_pool,
                                   stage, bin->prog_data->total_scratch),
      .offset = 0,
   };
}

static UNUSED uint32_t
get_scratch_space(const struct anv_shader_bin *bin)
{
   return ffs(bin->prog_data->total_scratch / 2048);
}

static UNUSED uint32_t
get_scratch_surf(struct anv_pipeline *pipeline,
                 gl_shader_stage stage,
                 const struct anv_shader_bin *bin)
{
   if (bin->prog_data->total_scratch == 0)
      return 0;

   struct anv_bo *bo =
      anv_scratch_pool_alloc(pipeline->device,
                             &pipeline->device->scratch_pool,
                             stage, bin->prog_data->total_scratch);
   anv_reloc_list_add_bo(pipeline->batch.relocs,
                         pipeline->batch.alloc, bo);
   return anv_scratch_pool_get_surf(pipeline->device,
                                    &pipeline->device->scratch_pool,
                                    bin->prog_data->total_scratch) >> 4;
}

static void
emit_3dstate_vs(struct anv_graphics_pipeline *pipeline)
{
   const struct intel_device_info *devinfo = &pipeline->base.device->info;
   const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);
   const struct anv_shader_bin *vs_bin =
      pipeline->shaders[MESA_SHADER_VERTEX];

   assert(anv_pipeline_has_stage(pipeline, MESA_SHADER_VERTEX));

   anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_VS), vs) {
      vs.Enable               = true;
      vs.StatisticsEnable     = true;
      vs.KernelStartPointer   = vs_bin->kernel.offset;
#if GFX_VER >= 8
      vs.SIMD8DispatchEnable  =
         vs_prog_data->base.dispatch_mode == DISPATCH_MODE_SIMD8;
#endif

      assert(!vs_prog_data->base.base.use_alt_mode);
#if GFX_VER < 11
      vs.SingleVertexDispatch       = false;
#endif
      vs.VectorMaskEnable           = false;
      /* Wa_1606682166:
       * Incorrect TDL's SSP address shift in SARB for 16:6 & 18:8 modes.
       * Disable the Sampler state prefetch functionality in the SARB by
       * programming 0xB000[30] to '1'.
       */
      vs.SamplerCount               = GFX_VER == 11 ? 0 : get_sampler_count(vs_bin);
      vs.BindingTableEntryCount     = vs_bin->bind_map.surface_count;
      vs.FloatingPointMode          = IEEE754;
      vs.IllegalOpcodeExceptionEnable = false;
      vs.SoftwareExceptionEnable    = false;
      vs.MaximumNumberofThreads     = devinfo->max_vs_threads - 1;

      if (GFX_VER == 9 && devinfo->gt == 4 &&
          anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL)) {
         /* On Sky Lake GT4, we have experienced some hangs related to the VS
          * cache and tessellation.  It is unknown exactly what is happening
          * but the Haswell docs for the "VS Reference Count Full Force Miss
          * Enable" field of the "Thread Mode" register refer to a HSW bug in
          * which the VUE handle reference count would overflow resulting in
          * internal reference counting bugs.  My (Jason's) best guess is that
          * this bug cropped back up on SKL GT4 when we suddenly had more
          * threads in play than any previous gfx9 hardware.
          *
          * What we do know for sure is that setting this bit when
          * tessellation shaders are in use fixes a GPU hang in Batman: Arkham
          * City when playing with DXVK (https://bugs.freedesktop.org/107280).
          * Disabling the vertex cache with tessellation shaders should only
          * have a minor performance impact as the tessellation shaders are
          * likely generating and processing far more geometry than the vertex
          * stage.
          */
         vs.VertexCacheDisable = true;
      }

      vs.VertexURBEntryReadLength      = vs_prog_data->base.urb_read_length;
      vs.VertexURBEntryReadOffset      = 0;
      vs.DispatchGRFStartRegisterForURBData =
         vs_prog_data->base.base.dispatch_grf_start_reg;

#if GFX_VER >= 8
      vs.UserClipDistanceClipTestEnableBitmask =
         vs_prog_data->base.clip_distance_mask;
      vs.UserClipDistanceCullTestEnableBitmask =
         vs_prog_data->base.cull_distance_mask;
#endif

#if GFX_VERx10 >= 125
      vs.ScratchSpaceBuffer =
         get_scratch_surf(&pipeline->base, MESA_SHADER_VERTEX, vs_bin);
#else
      vs.PerThreadScratchSpace   = get_scratch_space(vs_bin);
      vs.ScratchSpaceBasePointer =
         get_scratch_address(&pipeline->base, MESA_SHADER_VERTEX, vs_bin);
#endif
   }
}

static void
emit_3dstate_hs_te_ds(struct anv_graphics_pipeline *pipeline,
                      const VkPipelineTessellationStateCreateInfo *tess_info)
{
   if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL)) {
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_HS), hs);
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_TE), te);
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_DS), ds);
      return;
   }

   const struct intel_device_info *devinfo = &pipeline->base.device->info;
   const struct anv_shader_bin *tcs_bin =
      pipeline->shaders[MESA_SHADER_TESS_CTRL];
   const struct anv_shader_bin *tes_bin =
      pipeline->shaders[MESA_SHADER_TESS_EVAL];

   const struct brw_tcs_prog_data *tcs_prog_data = get_tcs_prog_data(pipeline);
   const struct brw_tes_prog_data *tes_prog_data = get_tes_prog_data(pipeline);

   anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_HS), hs) {
      hs.Enable = true;
      hs.StatisticsEnable = true;
      hs.KernelStartPointer = tcs_bin->kernel.offset;
      /* Wa_1606682166 */
      hs.SamplerCount = GFX_VER == 11 ? 0 : get_sampler_count(tcs_bin);
      hs.BindingTableEntryCount = tcs_bin->bind_map.surface_count;

#if GFX_VER >= 12
      /* Wa_1604578095:
       *
       *    Hang occurs when the number of max threads is less than 2 times
       *    the number of instance count. The number of max threads must be
       *    more than 2 times the number of instance count.
       */
      assert((devinfo->max_tcs_threads / 2) > tcs_prog_data->instances);
#endif

      hs.MaximumNumberofThreads = devinfo->max_tcs_threads - 1;
      hs.IncludeVertexHandles = true;
      hs.InstanceCount = tcs_prog_data->instances - 1;

      hs.VertexURBEntryReadLength = 0;
      hs.VertexURBEntryReadOffset = 0;
      hs.DispatchGRFStartRegisterForURBData =
         tcs_prog_data->base.base.dispatch_grf_start_reg & 0x1f;
#if GFX_VER >= 12
      hs.DispatchGRFStartRegisterForURBData5 =
         tcs_prog_data->base.base.dispatch_grf_start_reg >> 5;
#endif

#if GFX_VERx10 >= 125
      hs.ScratchSpaceBuffer =
         get_scratch_surf(&pipeline->base, MESA_SHADER_TESS_CTRL, tcs_bin);
#else
      hs.PerThreadScratchSpace = get_scratch_space(tcs_bin);
      hs.ScratchSpaceBasePointer =
         get_scratch_address(&pipeline->base, MESA_SHADER_TESS_CTRL, tcs_bin);
#endif

#if GFX_VER == 12
      /*  Patch Count threshold specifies the maximum number of patches that
       *  will be accumulated before a thread dispatch is forced.
       */
      hs.PatchCountThreshold = tcs_prog_data->patch_count_threshold;
#endif

#if GFX_VER >= 9
      hs.DispatchMode = tcs_prog_data->base.dispatch_mode;
      hs.IncludePrimitiveID = tcs_prog_data->include_primitive_id;
#endif
   }

   const VkPipelineTessellationDomainOriginStateCreateInfo *domain_origin_state =
      tess_info ? vk_find_struct_const(tess_info, PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO) : NULL;

   VkTessellationDomainOrigin uv_origin =
      domain_origin_state ? domain_origin_state->domainOrigin :
                            VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT;

   anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_TE), te) {
      te.Partitioning = tes_prog_data->partitioning;

      if (uv_origin == VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT) {
         te.OutputTopology = tes_prog_data->output_topology;
      } else {
         /* When the origin is upper-left, we have to flip the winding order */
         if (tes_prog_data->output_topology == OUTPUT_TRI_CCW) {
            te.OutputTopology = OUTPUT_TRI_CW;
         } else if (tes_prog_data->output_topology == OUTPUT_TRI_CW) {
            te.OutputTopology = OUTPUT_TRI_CCW;
         } else {
            te.OutputTopology = tes_prog_data->output_topology;
         }
      }

      te.TEDomain = tes_prog_data->domain;
      te.TEEnable = true;
      te.MaximumTessellationFactorOdd = 63.0;
      te.MaximumTessellationFactorNotOdd = 64.0;
   }

   anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_DS), ds) {
      ds.Enable = true;
      ds.StatisticsEnable = true;
      ds.KernelStartPointer = tes_bin->kernel.offset;
      /* Wa_1606682166 */
      ds.SamplerCount = GFX_VER == 11 ? 0 : get_sampler_count(tes_bin);
      ds.BindingTableEntryCount = tes_bin->bind_map.surface_count;
      ds.MaximumNumberofThreads = devinfo->max_tes_threads - 1;

      ds.ComputeWCoordinateEnable =
         tes_prog_data->domain == BRW_TESS_DOMAIN_TRI;

      ds.PatchURBEntryReadLength = tes_prog_data->base.urb_read_length;
      ds.PatchURBEntryReadOffset = 0;
      ds.DispatchGRFStartRegisterForURBData =
         tes_prog_data->base.base.dispatch_grf_start_reg;

#if GFX_VER >= 8
#if GFX_VER < 11
      ds.DispatchMode =
         tes_prog_data->base.dispatch_mode == DISPATCH_MODE_SIMD8 ?
            DISPATCH_MODE_SIMD8_SINGLE_PATCH :
            DISPATCH_MODE_SIMD4X2;
#else
      assert(tes_prog_data->base.dispatch_mode == DISPATCH_MODE_SIMD8);
      ds.DispatchMode = DISPATCH_MODE_SIMD8_SINGLE_PATCH;
#endif

      ds.UserClipDistanceClipTestEnableBitmask =
         tes_prog_data->base.clip_distance_mask;
      ds.UserClipDistanceCullTestEnableBitmask =
         tes_prog_data->base.cull_distance_mask;
#endif

#if GFX_VERx10 >= 125
      ds.ScratchSpaceBuffer =
         get_scratch_surf(&pipeline->base, MESA_SHADER_TESS_EVAL, tes_bin);
#else
      ds.PerThreadScratchSpace = get_scratch_space(tes_bin);
      ds.ScratchSpaceBasePointer =
         get_scratch_address(&pipeline->base, MESA_SHADER_TESS_EVAL, tes_bin);
#endif
   }
}

static void
emit_3dstate_gs(struct anv_graphics_pipeline *pipeline)
{
   const struct intel_device_info *devinfo = &pipeline->base.device->info;
   const struct anv_shader_bin *gs_bin =
      pipeline->shaders[MESA_SHADER_GEOMETRY];

   if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_GEOMETRY)) {
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_GS), gs);
      return;
   }

   const struct brw_gs_prog_data *gs_prog_data = get_gs_prog_data(pipeline);

   anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_GS), gs) {
      gs.Enable                  = true;
      gs.StatisticsEnable        = true;
      gs.KernelStartPointer      = gs_bin->kernel.offset;
      gs.DispatchMode            = gs_prog_data->base.dispatch_mode;

      gs.SingleProgramFlow       = false;
      gs.VectorMaskEnable        = false;
      /* Wa_1606682166 */
      gs.SamplerCount            = GFX_VER == 11 ? 0 : get_sampler_count(gs_bin);
      gs.BindingTableEntryCount  = gs_bin->bind_map.surface_count;
      gs.IncludeVertexHandles    = gs_prog_data->base.include_vue_handles;
      gs.IncludePrimitiveID      = gs_prog_data->include_primitive_id;

      if (GFX_VER == 8) {
         /* Broadwell is weird.  It needs us to divide by 2. */
         gs.MaximumNumberofThreads = devinfo->max_gs_threads / 2 - 1;
      } else {
         gs.MaximumNumberofThreads = devinfo->max_gs_threads - 1;
      }

      gs.OutputVertexSize        = gs_prog_data->output_vertex_size_hwords * 2 - 1;
      gs.OutputTopology          = gs_prog_data->output_topology;
      gs.ControlDataFormat       = gs_prog_data->control_data_format;
      gs.ControlDataHeaderSize   = gs_prog_data->control_data_header_size_hwords;
      gs.InstanceControl         = MAX2(gs_prog_data->invocations, 1) - 1;
      gs.ReorderMode             = TRAILING;

#if GFX_VER >= 8
      gs.ExpectedVertexCount     = gs_prog_data->vertices_in;
      gs.StaticOutput            = gs_prog_data->static_vertex_count >= 0;
      gs.StaticOutputVertexCount = gs_prog_data->static_vertex_count >= 0 ?
                                   gs_prog_data->static_vertex_count : 0;
#endif

      gs.VertexURBEntryReadOffset = 0;
      gs.VertexURBEntryReadLength = gs_prog_data->base.urb_read_length;
      gs.DispatchGRFStartRegisterForURBData =
         gs_prog_data->base.base.dispatch_grf_start_reg;

#if GFX_VER >= 8
      gs.UserClipDistanceClipTestEnableBitmask =
         gs_prog_data->base.clip_distance_mask;
      gs.UserClipDistanceCullTestEnableBitmask =
         gs_prog_data->base.cull_distance_mask;
#endif

#if GFX_VERx10 >= 125
      gs.ScratchSpaceBuffer =
         get_scratch_surf(&pipeline->base, MESA_SHADER_GEOMETRY, gs_bin);
#else
      gs.PerThreadScratchSpace   = get_scratch_space(gs_bin);
      gs.ScratchSpaceBasePointer =
         get_scratch_address(&pipeline->base, MESA_SHADER_GEOMETRY, gs_bin);
#endif
   }
}

static bool
has_color_buffer_write_enabled(const struct anv_graphics_pipeline *pipeline,
                               const VkPipelineColorBlendStateCreateInfo *blend)
{
   const struct anv_shader_bin *shader_bin =
      pipeline->shaders[MESA_SHADER_FRAGMENT];
   if (!shader_bin)
      return false;

   if (!pipeline->dynamic_state.color_writes)
      return false;

   const struct anv_pipeline_bind_map *bind_map = &shader_bin->bind_map;
   for (int i = 0; i < bind_map->surface_count; i++) {
      struct anv_pipeline_binding *binding = &bind_map->surface_to_descriptor[i];

      if (binding->set != ANV_DESCRIPTOR_SET_COLOR_ATTACHMENTS)
         continue;

      if (binding->index == UINT32_MAX)
         continue;

      if (blend && blend->pAttachments[binding->index].colorWriteMask != 0)
         return true;
   }

   return false;
}

static void
emit_3dstate_wm(struct anv_graphics_pipeline *pipeline, struct anv_subpass *subpass,
                const VkPipelineInputAssemblyStateCreateInfo *ia,
                const VkPipelineRasterizationStateCreateInfo *raster,
                const VkPipelineColorBlendStateCreateInfo *blend,
                const VkPipelineMultisampleStateCreateInfo *multisample,
                const VkPipelineRasterizationLineStateCreateInfoEXT *line,
                const uint32_t dynamic_states)
{
   const struct brw_wm_prog_data *wm_prog_data = get_wm_prog_data(pipeline);

   struct GENX(3DSTATE_WM) wm = {
      GENX(3DSTATE_WM_header),
   };
   wm.StatisticsEnable                    = true;
   wm.LineEndCapAntialiasingRegionWidth   = _05pixels;
   wm.LineAntialiasingRegionWidth         = _10pixels;
   wm.PointRasterizationRule              = RASTRULE_UPPER_RIGHT;

   if (anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT)) {
      if (wm_prog_data->early_fragment_tests) {
            wm.EarlyDepthStencilControl         = EDSC_PREPS;
      } else if (wm_prog_data->has_side_effects) {
         wm.EarlyDepthStencilControl         = EDSC_PSEXEC;
      } else {
         wm.EarlyDepthStencilControl         = EDSC_NORMAL;
      }

#if GFX_VER >= 8
      /* Gen8 hardware tries to compute ThreadDispatchEnable for us but
       * doesn't take into account KillPixels when no depth or stencil
       * writes are enabled.  In order for occlusion queries to work
       * correctly with no attachments, we need to force-enable PS thread
       * dispatch.
       *
       * The BDW docs are pretty clear that that this bit isn't validated
       * and probably shouldn't be used in production:
       *
       *    "This must always be set to Normal. This field should not be
       *    tested for functional validation."
       *
       * Unfortunately, however, the other mechanism we have for doing this
       * is 3DSTATE_PS_EXTRA::PixelShaderHasUAV which causes hangs on BDW.
       * Given two bad options, we choose the one which works.
       */
      pipeline->force_fragment_thread_dispatch =
         wm_prog_data->has_side_effects ||
         wm_prog_data->uses_kill;

      if (pipeline->force_fragment_thread_dispatch ||
          !has_color_buffer_write_enabled(pipeline, blend)) {
         /* Only set this value in non dynamic mode. */
         wm.ForceThreadDispatchEnable =
            !(dynamic_states & ANV_CMD_DIRTY_DYNAMIC_COLOR_BLEND_STATE) ? ForceON : 0;
      }
#endif

      wm.BarycentricInterpolationMode =
         wm_prog_data->barycentric_interp_modes;

#if GFX_VER < 8
      wm.PixelShaderComputedDepthMode  = wm_prog_data->computed_depth_mode;
      wm.PixelShaderUsesSourceDepth    = wm_prog_data->uses_src_depth;
      wm.PixelShaderUsesSourceW        = wm_prog_data->uses_src_w;
      wm.PixelShaderUsesInputCoverageMask = wm_prog_data->uses_sample_mask;

      /* If the subpass has a depth or stencil self-dependency, then we
       * need to force the hardware to do the depth/stencil write *after*
       * fragment shader execution.  Otherwise, the writes may hit memory
       * before we get around to fetching from the input attachment and we
       * may get the depth or stencil value from the current draw rather
       * than the previous one.
       */
      wm.PixelShaderKillsPixel         = subpass->has_ds_self_dep ||
                                         wm_prog_data->uses_kill;

      pipeline->force_fragment_thread_dispatch =
         wm.PixelShaderComputedDepthMode != PSCDEPTH_OFF ||
         wm_prog_data->has_side_effects ||
         wm.PixelShaderKillsPixel;

      if (pipeline->force_fragment_thread_dispatch ||
          has_color_buffer_write_enabled(pipeline, blend)) {
         /* Only set this value in non dynamic mode. */
         wm.ThreadDispatchEnable = !(dynamic_states & ANV_CMD_DIRTY_DYNAMIC_COLOR_BLEND_STATE);
      }

      if (multisample && multisample->rasterizationSamples > 1) {
         if (wm_prog_data->persample_dispatch) {
            wm.MultisampleDispatchMode = MSDISPMODE_PERSAMPLE;
         } else {
            wm.MultisampleDispatchMode = MSDISPMODE_PERPIXEL;
         }
      } else {
         wm.MultisampleDispatchMode = MSDISPMODE_PERSAMPLE;
      }

      VkPolygonMode raster_mode =
         genX(raster_polygon_mode)(pipeline, ia->topology);

      wm.MultisampleRasterizationMode =
         dynamic_states & ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY ? 0 :
         genX(ms_rasterization_mode)(pipeline, raster_mode);
#endif

      wm.LineStippleEnable = line && line->stippledLineEnable;
   }

   uint32_t dynamic_wm_states = ANV_CMD_DIRTY_DYNAMIC_COLOR_BLEND_STATE;

#if GFX_VER < 8
   dynamic_wm_states |= ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY;
#endif

   if (dynamic_states & dynamic_wm_states) {
      const struct intel_device_info *devinfo = &pipeline->base.device->info;
      uint32_t *dws = devinfo->ver >= 8 ? pipeline->gfx8.wm : pipeline->gfx7.wm;
      GENX(3DSTATE_WM_pack)(NULL, dws, &wm);
   } else {
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_WM), _wm)
         _wm = wm;
   }
}

static void
emit_3dstate_ps(struct anv_graphics_pipeline *pipeline,
                const VkPipelineColorBlendStateCreateInfo *blend,
                const VkPipelineMultisampleStateCreateInfo *multisample)
{
   UNUSED const struct intel_device_info *devinfo =
      &pipeline->base.device->info;
   const struct anv_shader_bin *fs_bin =
      pipeline->shaders[MESA_SHADER_FRAGMENT];

   if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT)) {
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_PS), ps) {
#if GFX_VER == 7
         /* Even if no fragments are ever dispatched, gfx7 hardware hangs if
          * we don't at least set the maximum number of threads.
          */
         ps.MaximumNumberofThreads = devinfo->max_wm_threads - 1;
#endif
      }
      return;
   }

   const struct brw_wm_prog_data *wm_prog_data = get_wm_prog_data(pipeline);

#if GFX_VER < 8
   /* The hardware wedges if you have this bit set but don't turn on any dual
    * source blend factors.
    */
   bool dual_src_blend = false;
   if (wm_prog_data->dual_src_blend && blend) {
      for (uint32_t i = 0; i < blend->attachmentCount; i++) {
         const VkPipelineColorBlendAttachmentState *bstate =
            &blend->pAttachments[i];

         if (bstate->blendEnable &&
             (is_dual_src_blend_factor(bstate->srcColorBlendFactor) ||
              is_dual_src_blend_factor(bstate->dstColorBlendFactor) ||
              is_dual_src_blend_factor(bstate->srcAlphaBlendFactor) ||
              is_dual_src_blend_factor(bstate->dstAlphaBlendFactor))) {
            dual_src_blend = true;
            break;
         }
      }
   }
#endif

   anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_PS), ps) {
      ps._8PixelDispatchEnable      = wm_prog_data->dispatch_8;
      ps._16PixelDispatchEnable     = wm_prog_data->dispatch_16;
      ps._32PixelDispatchEnable     = wm_prog_data->dispatch_32;

      /* From the Sky Lake PRM 3DSTATE_PS::32 Pixel Dispatch Enable:
       *
       *    "When NUM_MULTISAMPLES = 16 or FORCE_SAMPLE_COUNT = 16, SIMD32
       *    Dispatch must not be enabled for PER_PIXEL dispatch mode."
       *
       * Since 16x MSAA is first introduced on SKL, we don't need to apply
       * the workaround on any older hardware.
       */
      if (GFX_VER >= 9 && !wm_prog_data->persample_dispatch &&
          multisample && multisample->rasterizationSamples == 16) {
         assert(ps._8PixelDispatchEnable || ps._16PixelDispatchEnable);
         ps._32PixelDispatchEnable = false;
      }

      ps.KernelStartPointer0 = fs_bin->kernel.offset +
                               brw_wm_prog_data_prog_offset(wm_prog_data, ps, 0);
      ps.KernelStartPointer1 = fs_bin->kernel.offset +
                               brw_wm_prog_data_prog_offset(wm_prog_data, ps, 1);
      ps.KernelStartPointer2 = fs_bin->kernel.offset +
                               brw_wm_prog_data_prog_offset(wm_prog_data, ps, 2);

      ps.SingleProgramFlow          = false;
      ps.VectorMaskEnable           = GFX_VER >= 8;
      /* Wa_1606682166 */
      ps.SamplerCount               = GFX_VER == 11 ? 0 : get_sampler_count(fs_bin);
      ps.BindingTableEntryCount     = fs_bin->bind_map.surface_count;
      ps.PushConstantEnable         = wm_prog_data->base.nr_params > 0 ||
                                      wm_prog_data->base.ubo_ranges[0].length;
      ps.PositionXYOffsetSelect     = wm_prog_data->uses_pos_offset ?
                                      POSOFFSET_SAMPLE: POSOFFSET_NONE;
#if GFX_VER < 8
      ps.AttributeEnable            = wm_prog_data->num_varying_inputs > 0;
      ps.oMaskPresenttoRenderTarget = wm_prog_data->uses_omask;
      ps.DualSourceBlendEnable      = dual_src_blend;
#endif

#if GFX_VERx10 == 75
      /* Haswell requires the sample mask to be set in this packet as well
       * as in 3DSTATE_SAMPLE_MASK; the values should match.
       */
      ps.SampleMask                 = 0xff;
#endif

#if GFX_VER >= 9
      ps.MaximumNumberofThreadsPerPSD  = 64 - 1;
#elif GFX_VER >= 8
      ps.MaximumNumberofThreadsPerPSD  = 64 - 2;
#else
      ps.MaximumNumberofThreads        = devinfo->max_wm_threads - 1;
#endif

      ps.DispatchGRFStartRegisterForConstantSetupData0 =
         brw_wm_prog_data_dispatch_grf_start_reg(wm_prog_data, ps, 0);
      ps.DispatchGRFStartRegisterForConstantSetupData1 =
         brw_wm_prog_data_dispatch_grf_start_reg(wm_prog_data, ps, 1);
      ps.DispatchGRFStartRegisterForConstantSetupData2 =
         brw_wm_prog_data_dispatch_grf_start_reg(wm_prog_data, ps, 2);

#if GFX_VERx10 >= 125
      ps.ScratchSpaceBuffer =
         get_scratch_surf(&pipeline->base, MESA_SHADER_FRAGMENT, fs_bin);
#else
      ps.PerThreadScratchSpace   = get_scratch_space(fs_bin);
      ps.ScratchSpaceBasePointer =
         get_scratch_address(&pipeline->base, MESA_SHADER_FRAGMENT, fs_bin);
#endif
   }
}

#if GFX_VER >= 8
static void
emit_3dstate_ps_extra(struct anv_graphics_pipeline *pipeline,
                      struct anv_subpass *subpass,
                      const VkPipelineRasterizationStateCreateInfo *rs_info)
{
   const struct brw_wm_prog_data *wm_prog_data = get_wm_prog_data(pipeline);

   if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT)) {
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_PS_EXTRA), ps);
      return;
   }

   anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_PS_EXTRA), ps) {
      ps.PixelShaderValid              = true;
      ps.AttributeEnable               = wm_prog_data->num_varying_inputs > 0;
      ps.oMaskPresenttoRenderTarget    = wm_prog_data->uses_omask;
      ps.PixelShaderIsPerSample        = wm_prog_data->persample_dispatch;
      ps.PixelShaderComputedDepthMode  = wm_prog_data->computed_depth_mode;
      ps.PixelShaderUsesSourceDepth    = wm_prog_data->uses_src_depth;
      ps.PixelShaderUsesSourceW        = wm_prog_data->uses_src_w;

      /* If the subpass has a depth or stencil self-dependency, then we need
       * to force the hardware to do the depth/stencil write *after* fragment
       * shader execution.  Otherwise, the writes may hit memory before we get
       * around to fetching from the input attachment and we may get the depth
       * or stencil value from the current draw rather than the previous one.
       */
      ps.PixelShaderKillsPixel         = subpass->has_ds_self_dep ||
                                         wm_prog_data->uses_kill;

#if GFX_VER >= 9
      ps.PixelShaderComputesStencil = wm_prog_data->computed_stencil;
      ps.PixelShaderPullsBary    = wm_prog_data->pulls_bary;

      ps.InputCoverageMaskState = ICMS_NONE;
      assert(!wm_prog_data->inner_coverage); /* Not available in SPIR-V */
      if (!wm_prog_data->uses_sample_mask)
         ps.InputCoverageMaskState = ICMS_NONE;
      else if (wm_prog_data->per_coarse_pixel_dispatch)
         ps.InputCoverageMaskState  = ICMS_NORMAL;
      else if (wm_prog_data->post_depth_coverage)
         ps.InputCoverageMaskState = ICMS_DEPTH_COVERAGE;
      else
         ps.InputCoverageMaskState = ICMS_NORMAL;
#else
      ps.PixelShaderUsesInputCoverageMask = wm_prog_data->uses_sample_mask;
#endif

#if GFX_VER >= 11
      ps.PixelShaderRequiresSourceDepthandorWPlaneCoefficients =
         wm_prog_data->uses_depth_w_coefficients;
      ps.PixelShaderIsPerCoarsePixel = wm_prog_data->per_coarse_pixel_dispatch;
#endif
   }
}

static void
emit_3dstate_vf_topology(struct anv_graphics_pipeline *pipeline)
{
   anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_VF_TOPOLOGY), vft) {
      vft.PrimitiveTopologyType = pipeline->topology;
   }
}
#endif

static void
emit_3dstate_vf_statistics(struct anv_graphics_pipeline *pipeline)
{
   anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_VF_STATISTICS), vfs) {
      vfs.StatisticsEnable = true;
   }
}

static void
compute_kill_pixel(struct anv_graphics_pipeline *pipeline,
                   const VkPipelineMultisampleStateCreateInfo *ms_info,
                   const struct anv_subpass *subpass)
{
   if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT)) {
      pipeline->kill_pixel = false;
      return;
   }

   const struct brw_wm_prog_data *wm_prog_data = get_wm_prog_data(pipeline);

   /* This computes the KillPixel portion of the computation for whether or
    * not we want to enable the PMA fix on gfx8 or gfx9.  It's given by this
    * chunk of the giant formula:
    *
    *    (3DSTATE_PS_EXTRA::PixelShaderKillsPixels ||
    *     3DSTATE_PS_EXTRA::oMask Present to RenderTarget ||
    *     3DSTATE_PS_BLEND::AlphaToCoverageEnable ||
    *     3DSTATE_PS_BLEND::AlphaTestEnable ||
    *     3DSTATE_WM_CHROMAKEY::ChromaKeyKillEnable)
    *
    * 3DSTATE_WM_CHROMAKEY::ChromaKeyKillEnable is always false and so is
    * 3DSTATE_PS_BLEND::AlphaTestEnable since Vulkan doesn't have a concept
    * of an alpha test.
    */
   pipeline->kill_pixel =
      subpass->has_ds_self_dep || wm_prog_data->uses_kill ||
      wm_prog_data->uses_omask ||
      (ms_info && ms_info->alphaToCoverageEnable);
}

#if GFX_VER == 12
static void
emit_3dstate_primitive_replication(struct anv_graphics_pipeline *pipeline)
{
   if (!pipeline->use_primitive_replication) {
      anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_PRIMITIVE_REPLICATION), pr);
      return;
   }

   uint32_t view_mask = pipeline->subpass->view_mask;
   int view_count = util_bitcount(view_mask);
   assert(view_count > 1 && view_count <= MAX_VIEWS_FOR_PRIMITIVE_REPLICATION);

   anv_batch_emit(&pipeline->base.batch, GENX(3DSTATE_PRIMITIVE_REPLICATION), pr) {
      pr.ReplicaMask = (1 << view_count) - 1;
      pr.ReplicationCount = view_count - 1;

      int i = 0;
      u_foreach_bit(view_index, view_mask) {
         pr.RTAIOffset[i] = view_index;
         i++;
      }
   }
}
#endif

static VkResult
genX(graphics_pipeline_create)(
    VkDevice                                    _device,
    struct anv_pipeline_cache *                 cache,
    const VkGraphicsPipelineCreateInfo*         pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipeline)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_render_pass, pass, pCreateInfo->renderPass);
   struct anv_subpass *subpass = &pass->subpasses[pCreateInfo->subpass];
   struct anv_graphics_pipeline *pipeline;
   VkResult result;

   assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);

   /* Use the default pipeline cache if none is specified */
   if (cache == NULL && device->physical->instance->pipeline_cache_enabled)
      cache = &device->default_pipeline_cache;

   pipeline = vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*pipeline), 8,
                         VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (pipeline == NULL)
      return vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);

   result = anv_graphics_pipeline_init(pipeline, device, cache,
                                       pCreateInfo, pAllocator);
   if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, pipeline);
      if (result == VK_PIPELINE_COMPILE_REQUIRED_EXT)
         *pPipeline = VK_NULL_HANDLE;
      return result;
   }

   /* Information on which states are considered dynamic. */
   const VkPipelineDynamicStateCreateInfo *dyn_info =
      pCreateInfo->pDynamicState;
   uint32_t dynamic_states = 0;
   if (dyn_info) {
      for (unsigned i = 0; i < dyn_info->dynamicStateCount; i++)
         dynamic_states |=
            anv_cmd_dirty_bit_for_vk_dynamic_state(dyn_info->pDynamicStates[i]);
   }


   /* If rasterization is not enabled, various CreateInfo structs must be
    * ignored.
    */
   const bool raster_enabled =
      !pCreateInfo->pRasterizationState->rasterizerDiscardEnable ||
      (dynamic_states & ANV_CMD_DIRTY_DYNAMIC_RASTERIZER_DISCARD_ENABLE);

   const VkPipelineViewportStateCreateInfo *vp_info =
      raster_enabled ? pCreateInfo->pViewportState : NULL;

   const VkPipelineMultisampleStateCreateInfo *ms_info =
      raster_enabled ? pCreateInfo->pMultisampleState : NULL;

   const VkPipelineDepthStencilStateCreateInfo *ds_info =
      raster_enabled ? pCreateInfo->pDepthStencilState : NULL;

   const VkPipelineColorBlendStateCreateInfo *cb_info =
      raster_enabled ? pCreateInfo->pColorBlendState : NULL;

   const VkPipelineRasterizationLineStateCreateInfoEXT *line_info =
      vk_find_struct_const(pCreateInfo->pRasterizationState->pNext,
                           PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT);

   enum intel_urb_deref_block_size urb_deref_block_size;
   emit_urb_setup(pipeline, &urb_deref_block_size);

   assert(pCreateInfo->pRasterizationState);
   emit_rs_state(pipeline, pCreateInfo->pInputAssemblyState,
                           pCreateInfo->pRasterizationState,
                           ms_info, line_info, dynamic_states, pass, subpass,
                           urb_deref_block_size);
   emit_ms_state(pipeline, ms_info, dynamic_states);
   emit_ds_state(pipeline, ds_info, dynamic_states, pass, subpass);
   emit_cb_state(pipeline, cb_info, ms_info, dynamic_states);
   compute_kill_pixel(pipeline, ms_info, subpass);

   emit_3dstate_clip(pipeline,
                     pCreateInfo->pInputAssemblyState,
                     vp_info,
                     pCreateInfo->pRasterizationState,
                     dynamic_states);

#if GFX_VER == 12
   emit_3dstate_primitive_replication(pipeline);
#endif

#if 0
   /* From gfx7_vs_state.c */

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
   if (!device->info.is_haswell && !device->info.is_baytrail)
      gfx7_emit_vs_workaround_flush(brw);
#endif

   if (anv_pipeline_is_primitive(pipeline)) {
      assert(pCreateInfo->pVertexInputState);
      emit_vertex_input(pipeline, pCreateInfo->pVertexInputState);

      emit_3dstate_vs(pipeline);
      emit_3dstate_hs_te_ds(pipeline, pCreateInfo->pTessellationState);
      emit_3dstate_gs(pipeline);

#if GFX_VER >= 8
      if (!(dynamic_states & ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY))
         emit_3dstate_vf_topology(pipeline);
#endif

      emit_3dstate_vf_statistics(pipeline);

      emit_3dstate_streamout(pipeline, pCreateInfo->pRasterizationState,
                             dynamic_states);
   }

   emit_3dstate_sbe(pipeline);
   emit_3dstate_wm(pipeline, subpass,
                   pCreateInfo->pInputAssemblyState,
                   pCreateInfo->pRasterizationState,
                   cb_info, ms_info, line_info, dynamic_states);
   emit_3dstate_ps(pipeline, cb_info, ms_info);
#if GFX_VER >= 8
   emit_3dstate_ps_extra(pipeline, subpass,
                         pCreateInfo->pRasterizationState);
#endif

   *pPipeline = anv_pipeline_to_handle(&pipeline->base);

   return pipeline->base.batch.status;
}

#if GFX_VERx10 >= 125

static void
emit_compute_state(struct anv_compute_pipeline *pipeline,
                   const struct anv_device *device)
{
   const struct brw_cs_prog_data *cs_prog_data = get_cs_prog_data(pipeline);
   anv_pipeline_setup_l3_config(&pipeline->base, cs_prog_data->base.total_shared > 0);

   const UNUSED struct anv_shader_bin *cs_bin = pipeline->cs;
   const struct intel_device_info *devinfo = &device->info;

   anv_batch_emit(&pipeline->base.batch, GENX(CFE_STATE), cfe) {
      cfe.MaximumNumberofThreads =
         devinfo->max_cs_threads * devinfo->subslice_total - 1;
      cfe.ScratchSpaceBuffer =
         get_scratch_surf(&pipeline->base, MESA_SHADER_COMPUTE, cs_bin);
   }
}

#else /* #if GFX_VERx10 >= 125 */

static void
emit_compute_state(struct anv_compute_pipeline *pipeline,
                   const struct anv_device *device)
{
   const struct intel_device_info *devinfo = &device->info;
   const struct brw_cs_prog_data *cs_prog_data = get_cs_prog_data(pipeline);

   anv_pipeline_setup_l3_config(&pipeline->base, cs_prog_data->base.total_shared > 0);

   const struct brw_cs_dispatch_info dispatch =
      brw_cs_get_dispatch_info(devinfo, cs_prog_data, NULL);
   const uint32_t vfe_curbe_allocation =
      ALIGN(cs_prog_data->push.per_thread.regs * dispatch.threads +
            cs_prog_data->push.cross_thread.regs, 2);

   const struct anv_shader_bin *cs_bin = pipeline->cs;

   anv_batch_emit(&pipeline->base.batch, GENX(MEDIA_VFE_STATE), vfe) {
#if GFX_VER > 7
      vfe.StackSize              = 0;
#else
      vfe.GPGPUMode              = true;
#endif
      vfe.MaximumNumberofThreads =
         devinfo->max_cs_threads * devinfo->subslice_total - 1;
      vfe.NumberofURBEntries     = GFX_VER <= 7 ? 0 : 2;
#if GFX_VER < 11
      vfe.ResetGatewayTimer      = true;
#endif
#if GFX_VER <= 8
      vfe.BypassGatewayControl   = true;
#endif
      vfe.URBEntryAllocationSize = GFX_VER <= 7 ? 0 : 2;
      vfe.CURBEAllocationSize    = vfe_curbe_allocation;

      if (cs_bin->prog_data->total_scratch) {
         if (GFX_VER >= 8) {
            /* Broadwell's Per Thread Scratch Space is in the range [0, 11]
             * where 0 = 1k, 1 = 2k, 2 = 4k, ..., 11 = 2M.
             */
            vfe.PerThreadScratchSpace =
               ffs(cs_bin->prog_data->total_scratch) - 11;
         } else if (GFX_VERx10 == 75) {
            /* Haswell's Per Thread Scratch Space is in the range [0, 10]
             * where 0 = 2k, 1 = 4k, 2 = 8k, ..., 10 = 2M.
             */
            vfe.PerThreadScratchSpace =
               ffs(cs_bin->prog_data->total_scratch) - 12;
         } else {
            /* IVB and BYT use the range [0, 11] to mean [1kB, 12kB]
             * where 0 = 1kB, 1 = 2kB, 2 = 3kB, ..., 11 = 12kB.
             */
            vfe.PerThreadScratchSpace =
               cs_bin->prog_data->total_scratch / 1024 - 1;
         }
         vfe.ScratchSpaceBasePointer =
            get_scratch_address(&pipeline->base, MESA_SHADER_COMPUTE, cs_bin);
      }
   }

   struct GENX(INTERFACE_DESCRIPTOR_DATA) desc = {
      .KernelStartPointer     =
         cs_bin->kernel.offset +
         brw_cs_prog_data_prog_offset(cs_prog_data, dispatch.simd_size),

      /* Wa_1606682166 */
      .SamplerCount           = GFX_VER == 11 ? 0 : get_sampler_count(cs_bin),
      /* We add 1 because the CS indirect parameters buffer isn't accounted
       * for in bind_map.surface_count.
       */
      .BindingTableEntryCount = 1 + MIN2(cs_bin->bind_map.surface_count, 30),
      .BarrierEnable          = cs_prog_data->uses_barrier,
      .SharedLocalMemorySize  =
         encode_slm_size(GFX_VER, cs_prog_data->base.total_shared),

#if GFX_VERx10 != 75
      .ConstantURBEntryReadOffset = 0,
#endif
      .ConstantURBEntryReadLength = cs_prog_data->push.per_thread.regs,
#if GFX_VERx10 >= 75
      .CrossThreadConstantDataReadLength =
         cs_prog_data->push.cross_thread.regs,
#endif
#if GFX_VER >= 12
      /* TODO: Check if we are missing workarounds and enable mid-thread
       * preemption.
       *
       * We still have issues with mid-thread preemption (it was already
       * disabled by the kernel on gfx11, due to missing workarounds). It's
       * possible that we are just missing some workarounds, and could enable
       * it later, but for now let's disable it to fix a GPU in compute in Car
       * Chase (and possibly more).
       */
      .ThreadPreemptionDisable = true,
#endif

      .NumberofThreadsinGPGPUThreadGroup = dispatch.threads,
   };
   GENX(INTERFACE_DESCRIPTOR_DATA_pack)(NULL,
                                        pipeline->interface_descriptor_data,
                                        &desc);
}

#endif /* #if GFX_VERx10 >= 125 */

static VkResult
compute_pipeline_create(
    VkDevice                                    _device,
    struct anv_pipeline_cache *                 cache,
    const VkComputePipelineCreateInfo*          pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipeline)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   struct anv_compute_pipeline *pipeline;
   VkResult result;

   assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO);

   /* Use the default pipeline cache if none is specified */
   if (cache == NULL && device->physical->instance->pipeline_cache_enabled)
      cache = &device->default_pipeline_cache;

   pipeline = vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*pipeline), 8,
                         VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (pipeline == NULL)
      return vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);

   result = anv_pipeline_init(&pipeline->base, device,
                              ANV_PIPELINE_COMPUTE, pCreateInfo->flags,
                              pAllocator);
   if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, pipeline);
      return result;
   }

   anv_batch_set_storage(&pipeline->base.batch, ANV_NULL_ADDRESS,
                         pipeline->batch_data, sizeof(pipeline->batch_data));

   assert(pCreateInfo->stage.stage == VK_SHADER_STAGE_COMPUTE_BIT);
   VK_FROM_HANDLE(vk_shader_module, module,  pCreateInfo->stage.module);
   result = anv_pipeline_compile_cs(pipeline, cache, pCreateInfo, module,
                                    pCreateInfo->stage.pName,
                                    pCreateInfo->stage.pSpecializationInfo);
   if (result != VK_SUCCESS) {
      anv_pipeline_finish(&pipeline->base, device, pAllocator);
      vk_free2(&device->vk.alloc, pAllocator, pipeline);
      if (result == VK_PIPELINE_COMPILE_REQUIRED_EXT)
         *pPipeline = VK_NULL_HANDLE;
      return result;
   }

   emit_compute_state(pipeline, device);

   *pPipeline = anv_pipeline_to_handle(&pipeline->base);

   return pipeline->base.batch.status;
}

VkResult genX(CreateGraphicsPipelines)(
    VkDevice                                    _device,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    count,
    const VkGraphicsPipelineCreateInfo*         pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipelines)
{
   ANV_FROM_HANDLE(anv_pipeline_cache, pipeline_cache, pipelineCache);

   VkResult result = VK_SUCCESS;

   unsigned i;
   for (i = 0; i < count; i++) {
      VkResult res = genX(graphics_pipeline_create)(_device,
                                                    pipeline_cache,
                                                    &pCreateInfos[i],
                                                    pAllocator, &pPipelines[i]);

      if (res == VK_SUCCESS)
         continue;

      /* Bail out on the first error != VK_PIPELINE_COMPILE_REQUIRED_EX as it
       * is not obvious what error should be report upon 2 different failures.
       * */
      result = res;
      if (res != VK_PIPELINE_COMPILE_REQUIRED_EXT)
         break;

      if (pCreateInfos[i].flags & VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT_EXT)
         break;
   }

   for (; i < count; i++)
      pPipelines[i] = VK_NULL_HANDLE;

   return result;
}

VkResult genX(CreateComputePipelines)(
    VkDevice                                    _device,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    count,
    const VkComputePipelineCreateInfo*          pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipelines)
{
   ANV_FROM_HANDLE(anv_pipeline_cache, pipeline_cache, pipelineCache);

   VkResult result = VK_SUCCESS;

   unsigned i;
   for (i = 0; i < count; i++) {
      VkResult res = compute_pipeline_create(_device, pipeline_cache,
                                             &pCreateInfos[i],
                                             pAllocator, &pPipelines[i]);

      if (res == VK_SUCCESS)
         continue;

      /* Bail out on the first error != VK_PIPELINE_COMPILE_REQUIRED_EX as it
       * is not obvious what error should be report upon 2 different failures.
       * */
      result = res;
      if (res != VK_PIPELINE_COMPILE_REQUIRED_EXT)
         break;

      if (pCreateInfos[i].flags & VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT_EXT)
         break;
   }

   for (; i < count; i++)
      pPipelines[i] = VK_NULL_HANDLE;

   return result;
}

#if GFX_VERx10 >= 125

static void
assert_rt_stage_index_valid(const VkRayTracingPipelineCreateInfoKHR* pCreateInfo,
                            uint32_t stage_idx,
                            VkShaderStageFlags valid_stages)
{
   if (stage_idx == VK_SHADER_UNUSED_KHR)
      return;

   assert(stage_idx <= pCreateInfo->stageCount);
   assert(util_bitcount(pCreateInfo->pStages[stage_idx].stage) == 1);
   assert(pCreateInfo->pStages[stage_idx].stage & valid_stages);
}

static VkResult
ray_tracing_pipeline_create(
    VkDevice                                    _device,
    struct anv_pipeline_cache *                 cache,
    const VkRayTracingPipelineCreateInfoKHR*    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipeline)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   VkResult result;

   assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR);

   /* Use the default pipeline cache if none is specified */
   if (cache == NULL && device->physical->instance->pipeline_cache_enabled)
      cache = &device->default_pipeline_cache;

   VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct anv_ray_tracing_pipeline, pipeline, 1);
   VK_MULTIALLOC_DECL(&ma, struct anv_rt_shader_group, groups, pCreateInfo->groupCount);
   if (!vk_multialloc_zalloc2(&ma, &device->vk.alloc, pAllocator,
                              VK_SYSTEM_ALLOCATION_SCOPE_DEVICE))
      return vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);

   result = anv_pipeline_init(&pipeline->base, device,
                              ANV_PIPELINE_RAY_TRACING, pCreateInfo->flags,
                              pAllocator);
   if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, pipeline);
      return result;
   }

   pipeline->group_count = pCreateInfo->groupCount;
   pipeline->groups = groups;

   ASSERTED const VkShaderStageFlags ray_tracing_stages =
      VK_SHADER_STAGE_RAYGEN_BIT_KHR |
      VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
      VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
      VK_SHADER_STAGE_MISS_BIT_KHR |
      VK_SHADER_STAGE_INTERSECTION_BIT_KHR |
      VK_SHADER_STAGE_CALLABLE_BIT_KHR;

   for (uint32_t i = 0; i < pCreateInfo->stageCount; i++)
      assert((pCreateInfo->pStages[i].stage & ~ray_tracing_stages) == 0);

   for (uint32_t i = 0; i < pCreateInfo->groupCount; i++) {
      const VkRayTracingShaderGroupCreateInfoKHR *ginfo =
         &pCreateInfo->pGroups[i];
      assert_rt_stage_index_valid(pCreateInfo, ginfo->generalShader,
                                  VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                                  VK_SHADER_STAGE_MISS_BIT_KHR |
                                  VK_SHADER_STAGE_CALLABLE_BIT_KHR);
      assert_rt_stage_index_valid(pCreateInfo, ginfo->closestHitShader,
                                  VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
      assert_rt_stage_index_valid(pCreateInfo, ginfo->anyHitShader,
                                  VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
      assert_rt_stage_index_valid(pCreateInfo, ginfo->intersectionShader,
                                  VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
      switch (ginfo->type) {
      case VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR:
         assert(ginfo->generalShader < pCreateInfo->stageCount);
         assert(ginfo->anyHitShader == VK_SHADER_UNUSED_KHR);
         assert(ginfo->closestHitShader == VK_SHADER_UNUSED_KHR);
         assert(ginfo->intersectionShader == VK_SHADER_UNUSED_KHR);
         break;

      case VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR:
         assert(ginfo->generalShader == VK_SHADER_UNUSED_KHR);
         assert(ginfo->intersectionShader == VK_SHADER_UNUSED_KHR);
         break;

      case VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR:
         assert(ginfo->generalShader == VK_SHADER_UNUSED_KHR);
         break;

      default:
         unreachable("Invalid ray-tracing shader group type");
      }
   }

   result = anv_ray_tracing_pipeline_init(pipeline, device, cache,
                                          pCreateInfo, pAllocator);
   if (result != VK_SUCCESS) {
      anv_pipeline_finish(&pipeline->base, device, pAllocator);
      vk_free2(&device->vk.alloc, pAllocator, pipeline);
      return result;
   }

   for (uint32_t i = 0; i < pipeline->group_count; i++) {
      struct anv_rt_shader_group *group = &pipeline->groups[i];

      switch (group->type) {
      case VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR: {
         struct GFX_RT_GENERAL_SBT_HANDLE sh = {};
         sh.General = anv_shader_bin_get_bsr(group->general, 32);
         GFX_RT_GENERAL_SBT_HANDLE_pack(NULL, group->handle, &sh);
         break;
      }

      case VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR: {
         struct GFX_RT_TRIANGLES_SBT_HANDLE sh = {};
         if (group->closest_hit)
            sh.ClosestHit = anv_shader_bin_get_bsr(group->closest_hit, 32);
         if (group->any_hit)
            sh.AnyHit = anv_shader_bin_get_bsr(group->any_hit, 24);
         GFX_RT_TRIANGLES_SBT_HANDLE_pack(NULL, group->handle, &sh);
         break;
      }

      case VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR: {
         struct GFX_RT_PROCEDURAL_SBT_HANDLE sh = {};
         if (group->closest_hit)
            sh.ClosestHit = anv_shader_bin_get_bsr(group->closest_hit, 32);
         sh.Intersection = anv_shader_bin_get_bsr(group->intersection, 24);
         GFX_RT_PROCEDURAL_SBT_HANDLE_pack(NULL, group->handle, &sh);
         break;
      }

      default:
         unreachable("Invalid shader group type");
      }
   }

   *pPipeline = anv_pipeline_to_handle(&pipeline->base);

   return pipeline->base.batch.status;
}

VkResult
genX(CreateRayTracingPipelinesKHR)(
    VkDevice                                    _device,
    VkDeferredOperationKHR                      deferredOperation,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    createInfoCount,
    const VkRayTracingPipelineCreateInfoKHR*    pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipelines)
{
   ANV_FROM_HANDLE(anv_pipeline_cache, pipeline_cache, pipelineCache);

   VkResult result = VK_SUCCESS;

   unsigned i;
   for (i = 0; i < createInfoCount; i++) {
      VkResult res = ray_tracing_pipeline_create(_device, pipeline_cache,
                                                 &pCreateInfos[i],
                                                 pAllocator, &pPipelines[i]);

      if (res == VK_SUCCESS)
         continue;

      /* Bail out on the first error as it is not obvious what error should be
       * report upon 2 different failures. */
      result = res;
      if (result != VK_PIPELINE_COMPILE_REQUIRED_EXT)
         break;

      if (pCreateInfos[i].flags & VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT_EXT)
         break;
   }

   for (; i < createInfoCount; i++)
      pPipelines[i] = VK_NULL_HANDLE;

   return result;
}
#endif /* GFX_VERx10 >= 125 */
