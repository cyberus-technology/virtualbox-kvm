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

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "util/mesa-sha1.h"
#include "util/os_time.h"
#include "common/intel_l3_config.h"
#include "common/intel_disasm.h"
#include "common/intel_sample_positions.h"
#include "anv_private.h"
#include "compiler/brw_nir.h"
#include "compiler/brw_nir_rt.h"
#include "anv_nir.h"
#include "nir/nir_xfb_info.h"
#include "spirv/nir_spirv.h"
#include "vk_util.h"

/* Needed for SWIZZLE macros */
#include "program/prog_instruction.h"

// Shader functions
#define SPIR_V_MAGIC_NUMBER 0x07230203

struct anv_spirv_debug_data {
   struct anv_device *device;
   const struct vk_shader_module *module;
};

static void anv_spirv_nir_debug(void *private_data,
                                enum nir_spirv_debug_level level,
                                size_t spirv_offset,
                                const char *message)
{
   struct anv_spirv_debug_data *debug_data = private_data;

   switch (level) {
   case NIR_SPIRV_DEBUG_LEVEL_INFO:
      vk_logi(VK_LOG_OBJS(&debug_data->module->base),
              "SPIR-V offset %lu: %s",
              (unsigned long) spirv_offset, message);
      break;
   case NIR_SPIRV_DEBUG_LEVEL_WARNING:
      vk_logw(VK_LOG_OBJS(&debug_data->module->base),
              "SPIR-V offset %lu: %s",
              (unsigned long) spirv_offset, message);
      break;
   case NIR_SPIRV_DEBUG_LEVEL_ERROR:
      vk_loge(VK_LOG_OBJS(&debug_data->module->base),
              "SPIR-V offset %lu: %s",
              (unsigned long) spirv_offset, message);
      break;
   default:
      break;
   }
}

/* Eventually, this will become part of anv_CreateShader.  Unfortunately,
 * we can't do that yet because we don't have the ability to copy nir.
 */
static nir_shader *
anv_shader_compile_to_nir(struct anv_device *device,
                          void *mem_ctx,
                          const struct vk_shader_module *module,
                          const char *entrypoint_name,
                          gl_shader_stage stage,
                          const VkSpecializationInfo *spec_info)
{
   const struct anv_physical_device *pdevice = device->physical;
   const struct brw_compiler *compiler = pdevice->compiler;
   const nir_shader_compiler_options *nir_options =
      compiler->glsl_compiler_options[stage].NirOptions;

   uint32_t *spirv = (uint32_t *) module->data;
   assert(spirv[0] == SPIR_V_MAGIC_NUMBER);
   assert(module->size % 4 == 0);

   uint32_t num_spec_entries = 0;
   struct nir_spirv_specialization *spec_entries =
      vk_spec_info_to_nir_spirv(spec_info, &num_spec_entries);

   struct anv_spirv_debug_data spirv_debug_data = {
      .device = device,
      .module = module,
   };
   struct spirv_to_nir_options spirv_options = {
      .caps = {
         .demote_to_helper_invocation = true,
         .derivative_group = true,
         .descriptor_array_dynamic_indexing = true,
         .descriptor_array_non_uniform_indexing = true,
         .descriptor_indexing = true,
         .device_group = true,
         .draw_parameters = true,
         .float16 = pdevice->info.ver >= 8,
         .float32_atomic_add = pdevice->info.has_lsc,
         .float32_atomic_min_max = pdevice->info.ver >= 9,
         .float64 = pdevice->info.ver >= 8,
         .float64_atomic_min_max = pdevice->info.has_lsc,
         .fragment_shader_sample_interlock = pdevice->info.ver >= 9,
         .fragment_shader_pixel_interlock = pdevice->info.ver >= 9,
         .geometry_streams = true,
         /* When KHR_format_feature_flags2 is enabled, the read/write without
          * format is per format, so just report true. It's up to the
          * application to check.
          */
         .image_read_without_format = device->vk.enabled_extensions.KHR_format_feature_flags2,
         .image_write_without_format = true,
         .int8 = pdevice->info.ver >= 8,
         .int16 = pdevice->info.ver >= 8,
         .int64 = pdevice->info.ver >= 8,
         .int64_atomics = pdevice->info.ver >= 9 && pdevice->use_softpin,
         .integer_functions2 = pdevice->info.ver >= 8,
         .min_lod = true,
         .multiview = true,
         .physical_storage_buffer_address = pdevice->has_a64_buffer_access,
         .post_depth_coverage = pdevice->info.ver >= 9,
         .runtime_descriptor_array = true,
         .float_controls = pdevice->info.ver >= 8,
         .ray_tracing = pdevice->info.has_ray_tracing,
         .shader_clock = true,
         .shader_viewport_index_layer = true,
         .stencil_export = pdevice->info.ver >= 9,
         .storage_8bit = pdevice->info.ver >= 8,
         .storage_16bit = pdevice->info.ver >= 8,
         .subgroup_arithmetic = true,
         .subgroup_basic = true,
         .subgroup_ballot = true,
         .subgroup_dispatch = true,
         .subgroup_quad = true,
         .subgroup_uniform_control_flow = true,
         .subgroup_shuffle = true,
         .subgroup_vote = true,
         .tessellation = true,
         .transform_feedback = pdevice->info.ver >= 8,
         .variable_pointers = true,
         .vk_memory_model = true,
         .vk_memory_model_device_scope = true,
         .workgroup_memory_explicit_layout = true,
         .fragment_shading_rate = pdevice->info.ver >= 11,
      },
      .ubo_addr_format =
         anv_nir_ubo_addr_format(pdevice, device->robust_buffer_access),
      .ssbo_addr_format =
          anv_nir_ssbo_addr_format(pdevice, device->robust_buffer_access),
      .phys_ssbo_addr_format = nir_address_format_64bit_global,
      .push_const_addr_format = nir_address_format_logical,

      /* TODO: Consider changing this to an address format that has the NULL
       * pointer equals to 0.  That might be a better format to play nice
       * with certain code / code generators.
       */
      .shared_addr_format = nir_address_format_32bit_offset,
      .debug = {
         .func = anv_spirv_nir_debug,
         .private_data = &spirv_debug_data,
      },
   };


   nir_shader *nir =
      spirv_to_nir(spirv, module->size / 4,
                   spec_entries, num_spec_entries,
                   stage, entrypoint_name, &spirv_options, nir_options);
   if (!nir) {
      free(spec_entries);
      return NULL;
   }

   assert(nir->info.stage == stage);
   nir_validate_shader(nir, "after spirv_to_nir");
   nir_validate_ssa_dominance(nir, "after spirv_to_nir");
   ralloc_steal(mem_ctx, nir);

   free(spec_entries);

   const struct nir_lower_sysvals_to_varyings_options sysvals_to_varyings = {
      .point_coord = true,
   };
   NIR_PASS_V(nir, nir_lower_sysvals_to_varyings, &sysvals_to_varyings);

   if (INTEL_DEBUG(intel_debug_flag_for_shader_stage(stage))) {
      fprintf(stderr, "NIR (from SPIR-V) for %s shader:\n",
              gl_shader_stage_name(stage));
      nir_print_shader(nir, stderr);
   }

   /* We have to lower away local constant initializers right before we
    * inline functions.  That way they get properly initialized at the top
    * of the function and not at the top of its caller.
    */
   NIR_PASS_V(nir, nir_lower_variable_initializers, nir_var_function_temp);
   NIR_PASS_V(nir, nir_lower_returns);
   NIR_PASS_V(nir, nir_inline_functions);
   NIR_PASS_V(nir, nir_copy_prop);
   NIR_PASS_V(nir, nir_opt_deref);

   /* Pick off the single entrypoint that we want */
   foreach_list_typed_safe(nir_function, func, node, &nir->functions) {
      if (!func->is_entrypoint)
         exec_node_remove(&func->node);
   }
   assert(exec_list_length(&nir->functions) == 1);

   /* Now that we've deleted all but the main function, we can go ahead and
    * lower the rest of the constant initializers.  We do this here so that
    * nir_remove_dead_variables and split_per_member_structs below see the
    * corresponding stores.
    */
   NIR_PASS_V(nir, nir_lower_variable_initializers, ~0);

   const nir_opt_access_options opt_access_options = {
      .is_vulkan = true,
      .infer_non_readable = true,
   };
   NIR_PASS_V(nir, nir_opt_access, &opt_access_options);

   /* Split member structs.  We do this before lower_io_to_temporaries so that
    * it doesn't lower system values to temporaries by accident.
    */
   NIR_PASS_V(nir, nir_split_var_copies);
   NIR_PASS_V(nir, nir_split_per_member_structs);

   NIR_PASS_V(nir, nir_remove_dead_variables,
              nir_var_shader_in | nir_var_shader_out | nir_var_system_value |
              nir_var_shader_call_data | nir_var_ray_hit_attrib,
              NULL);

   NIR_PASS_V(nir, nir_propagate_invariant, false);
   NIR_PASS_V(nir, nir_lower_io_to_temporaries,
              nir_shader_get_entrypoint(nir), true, false);

   NIR_PASS_V(nir, nir_lower_frexp);

   /* Vulkan uses the separate-shader linking model */
   nir->info.separate_shader = true;

   brw_preprocess_nir(compiler, nir, NULL);

   return nir;
}

VkResult
anv_pipeline_init(struct anv_pipeline *pipeline,
                  struct anv_device *device,
                  enum anv_pipeline_type type,
                  VkPipelineCreateFlags flags,
                  const VkAllocationCallbacks *pAllocator)
{
   VkResult result;

   memset(pipeline, 0, sizeof(*pipeline));

   vk_object_base_init(&device->vk, &pipeline->base,
                       VK_OBJECT_TYPE_PIPELINE);
   pipeline->device = device;

   /* It's the job of the child class to provide actual backing storage for
    * the batch by setting batch.start, batch.next, and batch.end.
    */
   pipeline->batch.alloc = pAllocator ? pAllocator : &device->vk.alloc;
   pipeline->batch.relocs = &pipeline->batch_relocs;
   pipeline->batch.status = VK_SUCCESS;

   result = anv_reloc_list_init(&pipeline->batch_relocs,
                                pipeline->batch.alloc);
   if (result != VK_SUCCESS)
      return result;

   pipeline->mem_ctx = ralloc_context(NULL);

   pipeline->type = type;
   pipeline->flags = flags;

   util_dynarray_init(&pipeline->executables, pipeline->mem_ctx);

   return VK_SUCCESS;
}

void
anv_pipeline_finish(struct anv_pipeline *pipeline,
                    struct anv_device *device,
                    const VkAllocationCallbacks *pAllocator)
{
   anv_reloc_list_finish(&pipeline->batch_relocs,
                         pAllocator ? pAllocator : &device->vk.alloc);
   ralloc_free(pipeline->mem_ctx);
   vk_object_base_finish(&pipeline->base);
}

void anv_DestroyPipeline(
    VkDevice                                    _device,
    VkPipeline                                  _pipeline,
    const VkAllocationCallbacks*                pAllocator)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_pipeline, pipeline, _pipeline);

   if (!pipeline)
      return;

   switch (pipeline->type) {
   case ANV_PIPELINE_GRAPHICS: {
      struct anv_graphics_pipeline *gfx_pipeline =
         anv_pipeline_to_graphics(pipeline);

      if (gfx_pipeline->blend_state.map)
         anv_state_pool_free(&device->dynamic_state_pool, gfx_pipeline->blend_state);
      if (gfx_pipeline->cps_state.map)
         anv_state_pool_free(&device->dynamic_state_pool, gfx_pipeline->cps_state);

      for (unsigned s = 0; s < ARRAY_SIZE(gfx_pipeline->shaders); s++) {
         if (gfx_pipeline->shaders[s])
            anv_shader_bin_unref(device, gfx_pipeline->shaders[s]);
      }
      break;
   }

   case ANV_PIPELINE_COMPUTE: {
      struct anv_compute_pipeline *compute_pipeline =
         anv_pipeline_to_compute(pipeline);

      if (compute_pipeline->cs)
         anv_shader_bin_unref(device, compute_pipeline->cs);

      break;
   }

   case ANV_PIPELINE_RAY_TRACING: {
      struct anv_ray_tracing_pipeline *rt_pipeline =
         anv_pipeline_to_ray_tracing(pipeline);

      util_dynarray_foreach(&rt_pipeline->shaders,
                            struct anv_shader_bin *, shader) {
         anv_shader_bin_unref(device, *shader);
      }
      break;
   }

   default:
      unreachable("invalid pipeline type");
   }

   anv_pipeline_finish(pipeline, device, pAllocator);
   vk_free2(&device->vk.alloc, pAllocator, pipeline);
}

static const uint32_t vk_to_intel_primitive_type[] = {
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

static void
populate_sampler_prog_key(const struct intel_device_info *devinfo,
                          struct brw_sampler_prog_key_data *key)
{
   /* Almost all multisampled textures are compressed.  The only time when we
    * don't compress a multisampled texture is for 16x MSAA with a surface
    * width greater than 8k which is a bit of an edge case.  Since the sampler
    * just ignores the MCS parameter to ld2ms when MCS is disabled, it's safe
    * to tell the compiler to always assume compression.
    */
   key->compressed_multisample_layout_mask = ~0;

   /* SkyLake added support for 16x MSAA.  With this came a new message for
    * reading from a 16x MSAA surface with compression.  The new message was
    * needed because now the MCS data is 64 bits instead of 32 or lower as is
    * the case for 8x, 4x, and 2x.  The key->msaa_16 bit-field controls which
    * message we use.  Fortunately, the 16x message works for 8x, 4x, and 2x
    * so we can just use it unconditionally.  This may not be quite as
    * efficient but it saves us from recompiling.
    */
   if (devinfo->ver >= 9)
      key->msaa_16 = ~0;

   /* XXX: Handle texture swizzle on HSW- */
   for (int i = 0; i < MAX_SAMPLERS; i++) {
      /* Assume color sampler, no swizzling. (Works for BDW+) */
      key->swizzles[i] = SWIZZLE_XYZW;
   }
}

static void
populate_base_prog_key(const struct intel_device_info *devinfo,
                       enum brw_subgroup_size_type subgroup_size_type,
                       bool robust_buffer_acccess,
                       struct brw_base_prog_key *key)
{
   key->subgroup_size_type = subgroup_size_type;
   key->robust_buffer_access = robust_buffer_acccess;

   populate_sampler_prog_key(devinfo, &key->tex);
}

static void
populate_vs_prog_key(const struct intel_device_info *devinfo,
                     enum brw_subgroup_size_type subgroup_size_type,
                     bool robust_buffer_acccess,
                     struct brw_vs_prog_key *key)
{
   memset(key, 0, sizeof(*key));

   populate_base_prog_key(devinfo, subgroup_size_type,
                          robust_buffer_acccess, &key->base);

   /* XXX: Handle vertex input work-arounds */

   /* XXX: Handle sampler_prog_key */
}

static void
populate_tcs_prog_key(const struct intel_device_info *devinfo,
                      enum brw_subgroup_size_type subgroup_size_type,
                      bool robust_buffer_acccess,
                      unsigned input_vertices,
                      struct brw_tcs_prog_key *key)
{
   memset(key, 0, sizeof(*key));

   populate_base_prog_key(devinfo, subgroup_size_type,
                          robust_buffer_acccess, &key->base);

   key->input_vertices = input_vertices;
}

static void
populate_tes_prog_key(const struct intel_device_info *devinfo,
                      enum brw_subgroup_size_type subgroup_size_type,
                      bool robust_buffer_acccess,
                      struct brw_tes_prog_key *key)
{
   memset(key, 0, sizeof(*key));

   populate_base_prog_key(devinfo, subgroup_size_type,
                          robust_buffer_acccess, &key->base);
}

static void
populate_gs_prog_key(const struct intel_device_info *devinfo,
                     enum brw_subgroup_size_type subgroup_size_type,
                     bool robust_buffer_acccess,
                     struct brw_gs_prog_key *key)
{
   memset(key, 0, sizeof(*key));

   populate_base_prog_key(devinfo, subgroup_size_type,
                          robust_buffer_acccess, &key->base);
}

static bool
pipeline_has_coarse_pixel(const struct anv_graphics_pipeline *pipeline,
                          const VkPipelineFragmentShadingRateStateCreateInfoKHR *fsr_info)
{
   if (pipeline->sample_shading_enable)
      return false;

   /* Not dynamic & not specified for the pipeline. */
   if ((pipeline->dynamic_states & ANV_CMD_DIRTY_DYNAMIC_SHADING_RATE) == 0 && !fsr_info)
      return false;

   /* Not dynamic & pipeline has a 1x1 fragment shading rate with no
    * possibility for element of the pipeline to change the value.
    */
   if ((pipeline->dynamic_states & ANV_CMD_DIRTY_DYNAMIC_SHADING_RATE) == 0 &&
       fsr_info->fragmentSize.width <= 1 &&
       fsr_info->fragmentSize.height <= 1 &&
       fsr_info->combinerOps[0] == VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR &&
       fsr_info->combinerOps[1] == VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR)
      return false;

   return true;
}

static void
populate_wm_prog_key(const struct anv_graphics_pipeline *pipeline,
                     VkPipelineShaderStageCreateFlags flags,
                     bool robust_buffer_acccess,
                     const struct anv_subpass *subpass,
                     const VkPipelineMultisampleStateCreateInfo *ms_info,
                     const VkPipelineFragmentShadingRateStateCreateInfoKHR *fsr_info,
                     struct brw_wm_prog_key *key)
{
   const struct anv_device *device = pipeline->base.device;
   const struct intel_device_info *devinfo = &device->info;

   memset(key, 0, sizeof(*key));

   populate_base_prog_key(devinfo, flags, robust_buffer_acccess, &key->base);

   /* We set this to 0 here and set to the actual value before we call
    * brw_compile_fs.
    */
   key->input_slots_valid = 0;

   /* Vulkan doesn't specify a default */
   key->high_quality_derivatives = false;

   /* XXX Vulkan doesn't appear to specify */
   key->clamp_fragment_color = false;

   key->ignore_sample_mask_out = false;

   assert(subpass->color_count <= MAX_RTS);
   for (uint32_t i = 0; i < subpass->color_count; i++) {
      if (subpass->color_attachments[i].attachment != VK_ATTACHMENT_UNUSED)
         key->color_outputs_valid |= (1 << i);
   }

   key->nr_color_regions = subpass->color_count;

   /* To reduce possible shader recompilations we would need to know if
    * there is a SampleMask output variable to compute if we should emit
    * code to workaround the issue that hardware disables alpha to coverage
    * when there is SampleMask output.
    */
   key->alpha_to_coverage = ms_info && ms_info->alphaToCoverageEnable;

   /* Vulkan doesn't support fixed-function alpha test */
   key->alpha_test_replicate_alpha = false;

   if (ms_info) {
      /* We should probably pull this out of the shader, but it's fairly
       * harmless to compute it and then let dead-code take care of it.
       */
      if (ms_info->rasterizationSamples > 1) {
         key->persample_interp = ms_info->sampleShadingEnable &&
            (ms_info->minSampleShading * ms_info->rasterizationSamples) > 1;
         key->multisample_fbo = true;
      }

      key->frag_coord_adds_sample_pos = key->persample_interp;
   }

   key->coarse_pixel =
      device->vk.enabled_extensions.KHR_fragment_shading_rate &&
      pipeline_has_coarse_pixel(pipeline, fsr_info);
}

static void
populate_cs_prog_key(const struct intel_device_info *devinfo,
                     enum brw_subgroup_size_type subgroup_size_type,
                     bool robust_buffer_acccess,
                     struct brw_cs_prog_key *key)
{
   memset(key, 0, sizeof(*key));

   populate_base_prog_key(devinfo, subgroup_size_type,
                          robust_buffer_acccess, &key->base);
}

static void
populate_bs_prog_key(const struct intel_device_info *devinfo,
                     VkPipelineShaderStageCreateFlags flags,
                     bool robust_buffer_access,
                     struct brw_bs_prog_key *key)
{
   memset(key, 0, sizeof(*key));

   populate_base_prog_key(devinfo, flags, robust_buffer_access, &key->base);
}

struct anv_pipeline_stage {
   gl_shader_stage stage;

   const struct vk_shader_module *module;
   const char *entrypoint;
   const VkSpecializationInfo *spec_info;

   unsigned char shader_sha1[20];

   union brw_any_prog_key key;

   struct {
      gl_shader_stage stage;
      unsigned char sha1[20];
   } cache_key;

   nir_shader *nir;

   struct anv_pipeline_binding surface_to_descriptor[256];
   struct anv_pipeline_binding sampler_to_descriptor[256];
   struct anv_pipeline_bind_map bind_map;

   union brw_any_prog_data prog_data;

   uint32_t num_stats;
   struct brw_compile_stats stats[3];
   char *disasm[3];

   VkPipelineCreationFeedbackEXT feedback;

   const unsigned *code;

   struct anv_shader_bin *bin;
};

static void
anv_pipeline_hash_shader(const struct vk_shader_module *module,
                         const char *entrypoint,
                         gl_shader_stage stage,
                         const VkSpecializationInfo *spec_info,
                         unsigned char *sha1_out)
{
   struct mesa_sha1 ctx;
   _mesa_sha1_init(&ctx);

   _mesa_sha1_update(&ctx, module->sha1, sizeof(module->sha1));
   _mesa_sha1_update(&ctx, entrypoint, strlen(entrypoint));
   _mesa_sha1_update(&ctx, &stage, sizeof(stage));
   if (spec_info) {
      _mesa_sha1_update(&ctx, spec_info->pMapEntries,
                        spec_info->mapEntryCount *
                        sizeof(*spec_info->pMapEntries));
      _mesa_sha1_update(&ctx, spec_info->pData,
                        spec_info->dataSize);
   }

   _mesa_sha1_final(&ctx, sha1_out);
}

static void
anv_pipeline_hash_graphics(struct anv_graphics_pipeline *pipeline,
                           struct anv_pipeline_layout *layout,
                           struct anv_pipeline_stage *stages,
                           unsigned char *sha1_out)
{
   struct mesa_sha1 ctx;
   _mesa_sha1_init(&ctx);

   _mesa_sha1_update(&ctx, &pipeline->subpass->view_mask,
                     sizeof(pipeline->subpass->view_mask));

   if (layout)
      _mesa_sha1_update(&ctx, layout->sha1, sizeof(layout->sha1));

   const bool rba = pipeline->base.device->robust_buffer_access;
   _mesa_sha1_update(&ctx, &rba, sizeof(rba));

   for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
      if (stages[s].entrypoint) {
         _mesa_sha1_update(&ctx, stages[s].shader_sha1,
                           sizeof(stages[s].shader_sha1));
         _mesa_sha1_update(&ctx, &stages[s].key, brw_prog_key_size(s));
      }
   }

   _mesa_sha1_final(&ctx, sha1_out);
}

static void
anv_pipeline_hash_compute(struct anv_compute_pipeline *pipeline,
                          struct anv_pipeline_layout *layout,
                          struct anv_pipeline_stage *stage,
                          unsigned char *sha1_out)
{
   struct mesa_sha1 ctx;
   _mesa_sha1_init(&ctx);

   if (layout)
      _mesa_sha1_update(&ctx, layout->sha1, sizeof(layout->sha1));

   const bool rba = pipeline->base.device->robust_buffer_access;
   _mesa_sha1_update(&ctx, &rba, sizeof(rba));

   _mesa_sha1_update(&ctx, stage->shader_sha1,
                     sizeof(stage->shader_sha1));
   _mesa_sha1_update(&ctx, &stage->key.cs, sizeof(stage->key.cs));

   _mesa_sha1_final(&ctx, sha1_out);
}

static void
anv_pipeline_hash_ray_tracing_shader(struct anv_ray_tracing_pipeline *pipeline,
                                     struct anv_pipeline_layout *layout,
                                     struct anv_pipeline_stage *stage,
                                     unsigned char *sha1_out)
{
   struct mesa_sha1 ctx;
   _mesa_sha1_init(&ctx);

   if (layout != NULL)
      _mesa_sha1_update(&ctx, layout->sha1, sizeof(layout->sha1));

   const bool rba = pipeline->base.device->robust_buffer_access;
   _mesa_sha1_update(&ctx, &rba, sizeof(rba));

   _mesa_sha1_update(&ctx, stage->shader_sha1, sizeof(stage->shader_sha1));
   _mesa_sha1_update(&ctx, &stage->key, sizeof(stage->key.bs));

   _mesa_sha1_final(&ctx, sha1_out);
}

static void
anv_pipeline_hash_ray_tracing_combined_shader(struct anv_ray_tracing_pipeline *pipeline,
                                              struct anv_pipeline_layout *layout,
                                              struct anv_pipeline_stage *intersection,
                                              struct anv_pipeline_stage *any_hit,
                                              unsigned char *sha1_out)
{
   struct mesa_sha1 ctx;
   _mesa_sha1_init(&ctx);

   if (layout != NULL)
      _mesa_sha1_update(&ctx, layout->sha1, sizeof(layout->sha1));

   const bool rba = pipeline->base.device->robust_buffer_access;
   _mesa_sha1_update(&ctx, &rba, sizeof(rba));

   _mesa_sha1_update(&ctx, intersection->shader_sha1, sizeof(intersection->shader_sha1));
   _mesa_sha1_update(&ctx, &intersection->key, sizeof(intersection->key.bs));
   _mesa_sha1_update(&ctx, any_hit->shader_sha1, sizeof(any_hit->shader_sha1));
   _mesa_sha1_update(&ctx, &any_hit->key, sizeof(any_hit->key.bs));

   _mesa_sha1_final(&ctx, sha1_out);
}

static nir_shader *
anv_pipeline_stage_get_nir(struct anv_pipeline *pipeline,
                           struct anv_pipeline_cache *cache,
                           void *mem_ctx,
                           struct anv_pipeline_stage *stage)
{
   const struct brw_compiler *compiler =
      pipeline->device->physical->compiler;
   const nir_shader_compiler_options *nir_options =
      compiler->glsl_compiler_options[stage->stage].NirOptions;
   nir_shader *nir;

   nir = anv_device_search_for_nir(pipeline->device, cache,
                                   nir_options,
                                   stage->shader_sha1,
                                   mem_ctx);
   if (nir) {
      assert(nir->info.stage == stage->stage);
      return nir;
   }

   nir = anv_shader_compile_to_nir(pipeline->device,
                                   mem_ctx,
                                   stage->module,
                                   stage->entrypoint,
                                   stage->stage,
                                   stage->spec_info);
   if (nir) {
      anv_device_upload_nir(pipeline->device, cache, nir, stage->shader_sha1);
      return nir;
   }

   return NULL;
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

static void
anv_pipeline_lower_nir(struct anv_pipeline *pipeline,
                       void *mem_ctx,
                       struct anv_pipeline_stage *stage,
                       struct anv_pipeline_layout *layout)
{
   const struct anv_physical_device *pdevice = pipeline->device->physical;
   const struct brw_compiler *compiler = pdevice->compiler;

   struct brw_stage_prog_data *prog_data = &stage->prog_data.base;
   nir_shader *nir = stage->nir;

   if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      /* Check if sample shading is enabled in the shader and toggle
       * it on for the pipeline independent if sampleShadingEnable is set.
       */
      nir_shader_gather_info(nir, nir_shader_get_entrypoint(nir));
      if (nir->info.fs.uses_sample_shading)
         anv_pipeline_to_graphics(pipeline)->sample_shading_enable = true;

      NIR_PASS_V(nir, nir_lower_wpos_center,
                 anv_pipeline_to_graphics(pipeline)->sample_shading_enable);
      NIR_PASS_V(nir, nir_lower_input_attachments,
                 &(nir_input_attachment_options) {
                     .use_fragcoord_sysval = true,
                     .use_layer_id_sysval = true,
                 });
   }

   NIR_PASS_V(nir, anv_nir_lower_ycbcr_textures, layout);

   if (pipeline->type == ANV_PIPELINE_GRAPHICS) {
      NIR_PASS_V(nir, anv_nir_lower_multiview,
                 anv_pipeline_to_graphics(pipeline));
   }

   nir_shader_gather_info(nir, nir_shader_get_entrypoint(nir));

   NIR_PASS_V(nir, brw_nir_lower_storage_image, compiler->devinfo);

   NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_global,
              nir_address_format_64bit_global);
   NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_push_const,
              nir_address_format_32bit_offset);

   /* Apply the actual pipeline layout to UBOs, SSBOs, and textures */
   anv_nir_apply_pipeline_layout(pdevice,
                                 pipeline->device->robust_buffer_access,
                                 layout, nir, &stage->bind_map);

   NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_ubo,
              anv_nir_ubo_addr_format(pdevice,
                 pipeline->device->robust_buffer_access));
   NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_ssbo,
              anv_nir_ssbo_addr_format(pdevice,
                 pipeline->device->robust_buffer_access));

   /* First run copy-prop to get rid of all of the vec() that address
    * calculations often create and then constant-fold so that, when we
    * get to anv_nir_lower_ubo_loads, we can detect constant offsets.
    */
   NIR_PASS_V(nir, nir_copy_prop);
   NIR_PASS_V(nir, nir_opt_constant_folding);

   NIR_PASS_V(nir, anv_nir_lower_ubo_loads);

   /* We don't support non-uniform UBOs and non-uniform SSBO access is
    * handled naturally by falling back to A64 messages.
    */
   NIR_PASS_V(nir, nir_lower_non_uniform_access,
              &(nir_lower_non_uniform_access_options) {
                  .types = nir_lower_non_uniform_texture_access |
                           nir_lower_non_uniform_image_access,
                  .callback = NULL,
              });

   anv_nir_compute_push_layout(pdevice, pipeline->device->robust_buffer_access,
                               nir, prog_data, &stage->bind_map, mem_ctx);

   if (gl_shader_stage_uses_workgroup(nir->info.stage)) {
      if (!nir->info.shared_memory_explicit_layout) {
         NIR_PASS_V(nir, nir_lower_vars_to_explicit_types,
                    nir_var_mem_shared, shared_type_info);
      }

      NIR_PASS_V(nir, nir_lower_explicit_io,
                 nir_var_mem_shared, nir_address_format_32bit_offset);

      if (nir->info.zero_initialize_shared_memory &&
          nir->info.shared_size > 0) {
         /* The effective Shared Local Memory size is at least 1024 bytes and
          * is always rounded to a power of two, so it is OK to align the size
          * used by the shader to chunk_size -- which does simplify the logic.
          */
         const unsigned chunk_size = 16;
         const unsigned shared_size = ALIGN(nir->info.shared_size, chunk_size);
         assert(shared_size <=
                intel_calculate_slm_size(compiler->devinfo->ver, nir->info.shared_size));

         NIR_PASS_V(nir, nir_zero_initialize_shared_memory,
                    shared_size, chunk_size);
      }
   }

   stage->nir = nir;
}

static void
anv_pipeline_link_vs(const struct brw_compiler *compiler,
                     struct anv_pipeline_stage *vs_stage,
                     struct anv_pipeline_stage *next_stage)
{
   if (next_stage)
      brw_nir_link_shaders(compiler, vs_stage->nir, next_stage->nir);
}

static void
anv_pipeline_compile_vs(const struct brw_compiler *compiler,
                        void *mem_ctx,
                        struct anv_graphics_pipeline *pipeline,
                        struct anv_pipeline_stage *vs_stage)
{
   /* When using Primitive Replication for multiview, each view gets its own
    * position slot.
    */
   uint32_t pos_slots = pipeline->use_primitive_replication ?
      anv_subpass_view_count(pipeline->subpass) : 1;

   brw_compute_vue_map(compiler->devinfo,
                       &vs_stage->prog_data.vs.base.vue_map,
                       vs_stage->nir->info.outputs_written,
                       vs_stage->nir->info.separate_shader,
                       pos_slots);

   vs_stage->num_stats = 1;

   struct brw_compile_vs_params params = {
      .nir = vs_stage->nir,
      .key = &vs_stage->key.vs,
      .prog_data = &vs_stage->prog_data.vs,
      .stats = vs_stage->stats,
      .log_data = pipeline->base.device,
   };

   vs_stage->code = brw_compile_vs(compiler, mem_ctx, &params);
}

static void
merge_tess_info(struct shader_info *tes_info,
                const struct shader_info *tcs_info)
{
   /* The Vulkan 1.0.38 spec, section 21.1 Tessellator says:
    *
    *    "PointMode. Controls generation of points rather than triangles
    *     or lines. This functionality defaults to disabled, and is
    *     enabled if either shader stage includes the execution mode.
    *
    * and about Triangles, Quads, IsoLines, VertexOrderCw, VertexOrderCcw,
    * PointMode, SpacingEqual, SpacingFractionalEven, SpacingFractionalOdd,
    * and OutputVertices, it says:
    *
    *    "One mode must be set in at least one of the tessellation
    *     shader stages."
    *
    * So, the fields can be set in either the TCS or TES, but they must
    * agree if set in both.  Our backend looks at TES, so bitwise-or in
    * the values from the TCS.
    */
   assert(tcs_info->tess.tcs_vertices_out == 0 ||
          tes_info->tess.tcs_vertices_out == 0 ||
          tcs_info->tess.tcs_vertices_out == tes_info->tess.tcs_vertices_out);
   tes_info->tess.tcs_vertices_out |= tcs_info->tess.tcs_vertices_out;

   assert(tcs_info->tess.spacing == TESS_SPACING_UNSPECIFIED ||
          tes_info->tess.spacing == TESS_SPACING_UNSPECIFIED ||
          tcs_info->tess.spacing == tes_info->tess.spacing);
   tes_info->tess.spacing |= tcs_info->tess.spacing;

   assert(tcs_info->tess.primitive_mode == 0 ||
          tes_info->tess.primitive_mode == 0 ||
          tcs_info->tess.primitive_mode == tes_info->tess.primitive_mode);
   tes_info->tess.primitive_mode |= tcs_info->tess.primitive_mode;
   tes_info->tess.ccw |= tcs_info->tess.ccw;
   tes_info->tess.point_mode |= tcs_info->tess.point_mode;
}

static void
anv_pipeline_link_tcs(const struct brw_compiler *compiler,
                      struct anv_pipeline_stage *tcs_stage,
                      struct anv_pipeline_stage *tes_stage)
{
   assert(tes_stage && tes_stage->stage == MESA_SHADER_TESS_EVAL);

   brw_nir_link_shaders(compiler, tcs_stage->nir, tes_stage->nir);

   nir_lower_patch_vertices(tes_stage->nir,
                            tcs_stage->nir->info.tess.tcs_vertices_out,
                            NULL);

   /* Copy TCS info into the TES info */
   merge_tess_info(&tes_stage->nir->info, &tcs_stage->nir->info);

   /* Whacking the key after cache lookup is a bit sketchy, but all of
    * this comes from the SPIR-V, which is part of the hash used for the
    * pipeline cache.  So it should be safe.
    */
   tcs_stage->key.tcs.tes_primitive_mode =
      tes_stage->nir->info.tess.primitive_mode;
   tcs_stage->key.tcs.quads_workaround =
      compiler->devinfo->ver < 9 &&
      tes_stage->nir->info.tess.primitive_mode == 7 /* GL_QUADS */ &&
      tes_stage->nir->info.tess.spacing == TESS_SPACING_EQUAL;
}

static void
anv_pipeline_compile_tcs(const struct brw_compiler *compiler,
                         void *mem_ctx,
                         struct anv_device *device,
                         struct anv_pipeline_stage *tcs_stage,
                         struct anv_pipeline_stage *prev_stage)
{
   tcs_stage->key.tcs.outputs_written =
      tcs_stage->nir->info.outputs_written;
   tcs_stage->key.tcs.patch_outputs_written =
      tcs_stage->nir->info.patch_outputs_written;

   tcs_stage->num_stats = 1;
   tcs_stage->code = brw_compile_tcs(compiler, device, mem_ctx,
                                     &tcs_stage->key.tcs,
                                     &tcs_stage->prog_data.tcs,
                                     tcs_stage->nir, -1,
                                     tcs_stage->stats, NULL);
}

static void
anv_pipeline_link_tes(const struct brw_compiler *compiler,
                      struct anv_pipeline_stage *tes_stage,
                      struct anv_pipeline_stage *next_stage)
{
   if (next_stage)
      brw_nir_link_shaders(compiler, tes_stage->nir, next_stage->nir);
}

static void
anv_pipeline_compile_tes(const struct brw_compiler *compiler,
                         void *mem_ctx,
                         struct anv_device *device,
                         struct anv_pipeline_stage *tes_stage,
                         struct anv_pipeline_stage *tcs_stage)
{
   tes_stage->key.tes.inputs_read =
      tcs_stage->nir->info.outputs_written;
   tes_stage->key.tes.patch_inputs_read =
      tcs_stage->nir->info.patch_outputs_written;

   tes_stage->num_stats = 1;
   tes_stage->code = brw_compile_tes(compiler, device, mem_ctx,
                                     &tes_stage->key.tes,
                                     &tcs_stage->prog_data.tcs.base.vue_map,
                                     &tes_stage->prog_data.tes,
                                     tes_stage->nir, -1,
                                     tes_stage->stats, NULL);
}

static void
anv_pipeline_link_gs(const struct brw_compiler *compiler,
                     struct anv_pipeline_stage *gs_stage,
                     struct anv_pipeline_stage *next_stage)
{
   if (next_stage)
      brw_nir_link_shaders(compiler, gs_stage->nir, next_stage->nir);
}

static void
anv_pipeline_compile_gs(const struct brw_compiler *compiler,
                        void *mem_ctx,
                        struct anv_device *device,
                        struct anv_pipeline_stage *gs_stage,
                        struct anv_pipeline_stage *prev_stage)
{
   brw_compute_vue_map(compiler->devinfo,
                       &gs_stage->prog_data.gs.base.vue_map,
                       gs_stage->nir->info.outputs_written,
                       gs_stage->nir->info.separate_shader, 1);

   gs_stage->num_stats = 1;
   gs_stage->code = brw_compile_gs(compiler, device, mem_ctx,
                                   &gs_stage->key.gs,
                                   &gs_stage->prog_data.gs,
                                   gs_stage->nir, -1,
                                   gs_stage->stats, NULL);
}

static void
anv_pipeline_link_fs(const struct brw_compiler *compiler,
                     struct anv_pipeline_stage *stage)
{
   unsigned num_rt_bindings;
   struct anv_pipeline_binding rt_bindings[MAX_RTS];
   if (stage->key.wm.nr_color_regions > 0) {
      assert(stage->key.wm.nr_color_regions <= MAX_RTS);
      for (unsigned rt = 0; rt < stage->key.wm.nr_color_regions; rt++) {
         if (stage->key.wm.color_outputs_valid & BITFIELD_BIT(rt)) {
            rt_bindings[rt] = (struct anv_pipeline_binding) {
               .set = ANV_DESCRIPTOR_SET_COLOR_ATTACHMENTS,
               .index = rt,
            };
         } else {
            /* Setup a null render target */
            rt_bindings[rt] = (struct anv_pipeline_binding) {
               .set = ANV_DESCRIPTOR_SET_COLOR_ATTACHMENTS,
               .index = UINT32_MAX,
            };
         }
      }
      num_rt_bindings = stage->key.wm.nr_color_regions;
   } else {
      /* Setup a null render target */
      rt_bindings[0] = (struct anv_pipeline_binding) {
         .set = ANV_DESCRIPTOR_SET_COLOR_ATTACHMENTS,
         .index = UINT32_MAX,
      };
      num_rt_bindings = 1;
   }

   assert(num_rt_bindings <= MAX_RTS);
   assert(stage->bind_map.surface_count == 0);
   typed_memcpy(stage->bind_map.surface_to_descriptor,
                rt_bindings, num_rt_bindings);
   stage->bind_map.surface_count += num_rt_bindings;

   /* Now that we've set up the color attachments, we can go through and
    * eliminate any shader outputs that map to VK_ATTACHMENT_UNUSED in the
    * hopes that dead code can clean them up in this and any earlier shader
    * stages.
    */
   nir_function_impl *impl = nir_shader_get_entrypoint(stage->nir);
   bool deleted_output = false;
   nir_foreach_shader_out_variable_safe(var, stage->nir) {
      /* TODO: We don't delete depth/stencil writes.  We probably could if the
       * subpass doesn't have a depth/stencil attachment.
       */
      if (var->data.location < FRAG_RESULT_DATA0)
         continue;

      const unsigned rt = var->data.location - FRAG_RESULT_DATA0;

      /* If this is the RT at location 0 and we have alpha to coverage
       * enabled we still need that write because it will affect the coverage
       * mask even if it's never written to a color target.
       */
      if (rt == 0 && stage->key.wm.alpha_to_coverage)
         continue;

      const unsigned array_len =
         glsl_type_is_array(var->type) ? glsl_get_length(var->type) : 1;
      assert(rt + array_len <= MAX_RTS);

      if (rt >= MAX_RTS || !(stage->key.wm.color_outputs_valid &
                             BITFIELD_RANGE(rt, array_len))) {
         deleted_output = true;
         var->data.mode = nir_var_function_temp;
         exec_node_remove(&var->node);
         exec_list_push_tail(&impl->locals, &var->node);
      }
   }

   if (deleted_output)
      nir_fixup_deref_modes(stage->nir);

   /* Initially the valid outputs value is based off the renderpass color
    * attachments (see populate_wm_prog_key()), now that we've potentially
    * deleted variables that map to unused attachments, we need to update the
    * valid outputs for the backend compiler based on what output variables
    * are actually used. */
   stage->key.wm.color_outputs_valid = 0;
   nir_foreach_shader_out_variable_safe(var, stage->nir) {
      if (var->data.location < FRAG_RESULT_DATA0)
         continue;

      const unsigned rt = var->data.location - FRAG_RESULT_DATA0;
      const unsigned array_len =
         glsl_type_is_array(var->type) ? glsl_get_length(var->type) : 1;
      assert(rt + array_len <= MAX_RTS);

      stage->key.wm.color_outputs_valid |= BITFIELD_RANGE(rt, array_len);
   }

   /* We stored the number of subpass color attachments in nr_color_regions
    * when calculating the key for caching.  Now that we've computed the bind
    * map, we can reduce this to the actual max before we go into the back-end
    * compiler.
    */
   stage->key.wm.nr_color_regions =
      util_last_bit(stage->key.wm.color_outputs_valid);
}

static void
anv_pipeline_compile_fs(const struct brw_compiler *compiler,
                        void *mem_ctx,
                        struct anv_device *device,
                        struct anv_pipeline_stage *fs_stage,
                        struct anv_pipeline_stage *prev_stage)
{
   /* TODO: we could set this to 0 based on the information in nir_shader, but
    * we need this before we call spirv_to_nir.
    */
   assert(prev_stage);
   fs_stage->key.wm.input_slots_valid =
      prev_stage->prog_data.vue.vue_map.slots_valid;

   struct brw_compile_fs_params params = {
      .nir = fs_stage->nir,
      .key = &fs_stage->key.wm,
      .prog_data = &fs_stage->prog_data.wm,

      .allow_spilling = true,
      .stats = fs_stage->stats,
      .log_data = device,
   };

   fs_stage->code = brw_compile_fs(compiler, mem_ctx, &params);

   fs_stage->num_stats = (uint32_t)fs_stage->prog_data.wm.dispatch_8 +
                         (uint32_t)fs_stage->prog_data.wm.dispatch_16 +
                         (uint32_t)fs_stage->prog_data.wm.dispatch_32;

   if (fs_stage->key.wm.color_outputs_valid == 0 &&
       !fs_stage->prog_data.wm.has_side_effects &&
       !fs_stage->prog_data.wm.uses_omask &&
       !fs_stage->key.wm.alpha_to_coverage &&
       !fs_stage->prog_data.wm.uses_kill &&
       fs_stage->prog_data.wm.computed_depth_mode == BRW_PSCDEPTH_OFF &&
       !fs_stage->prog_data.wm.computed_stencil) {
      /* This fragment shader has no outputs and no side effects.  Go ahead
       * and return the code pointer so we don't accidentally think the
       * compile failed but zero out prog_data which will set program_size to
       * zero and disable the stage.
       */
      memset(&fs_stage->prog_data, 0, sizeof(fs_stage->prog_data));
   }
}

static void
anv_pipeline_add_executable(struct anv_pipeline *pipeline,
                            struct anv_pipeline_stage *stage,
                            struct brw_compile_stats *stats,
                            uint32_t code_offset)
{
   char *nir = NULL;
   if (stage->nir &&
       (pipeline->flags &
        VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR)) {
      nir = nir_shader_as_str(stage->nir, pipeline->mem_ctx);
   }

   char *disasm = NULL;
   if (stage->code &&
       (pipeline->flags &
        VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR)) {
      char *stream_data = NULL;
      size_t stream_size = 0;
      FILE *stream = open_memstream(&stream_data, &stream_size);

      uint32_t push_size = 0;
      for (unsigned i = 0; i < 4; i++)
         push_size += stage->bind_map.push_ranges[i].length;
      if (push_size > 0) {
         fprintf(stream, "Push constant ranges:\n");
         for (unsigned i = 0; i < 4; i++) {
            if (stage->bind_map.push_ranges[i].length == 0)
               continue;

            fprintf(stream, "    RANGE%d (%dB): ", i,
                    stage->bind_map.push_ranges[i].length * 32);

            switch (stage->bind_map.push_ranges[i].set) {
            case ANV_DESCRIPTOR_SET_NULL:
               fprintf(stream, "NULL");
               break;

            case ANV_DESCRIPTOR_SET_PUSH_CONSTANTS:
               fprintf(stream, "Vulkan push constants and API params");
               break;

            case ANV_DESCRIPTOR_SET_DESCRIPTORS:
               fprintf(stream, "Descriptor buffer for set %d (start=%dB)",
                       stage->bind_map.push_ranges[i].index,
                       stage->bind_map.push_ranges[i].start * 32);
               break;

            case ANV_DESCRIPTOR_SET_NUM_WORK_GROUPS:
               unreachable("gl_NumWorkgroups is never pushed");

            case ANV_DESCRIPTOR_SET_SHADER_CONSTANTS:
               fprintf(stream, "Inline shader constant data (start=%dB)",
                       stage->bind_map.push_ranges[i].start * 32);
               break;

            case ANV_DESCRIPTOR_SET_COLOR_ATTACHMENTS:
               unreachable("Color attachments can't be pushed");

            default:
               fprintf(stream, "UBO (set=%d binding=%d start=%dB)",
                       stage->bind_map.push_ranges[i].set,
                       stage->bind_map.push_ranges[i].index,
                       stage->bind_map.push_ranges[i].start * 32);
               break;
            }
            fprintf(stream, "\n");
         }
         fprintf(stream, "\n");
      }

      /* Creating this is far cheaper than it looks.  It's perfectly fine to
       * do it for every binary.
       */
      intel_disassemble(&pipeline->device->info,
                        stage->code, code_offset, stream);

      fclose(stream);

      /* Copy it to a ralloc'd thing */
      disasm = ralloc_size(pipeline->mem_ctx, stream_size + 1);
      memcpy(disasm, stream_data, stream_size);
      disasm[stream_size] = 0;

      free(stream_data);
   }

   const struct anv_pipeline_executable exe = {
      .stage = stage->stage,
      .stats = *stats,
      .nir = nir,
      .disasm = disasm,
   };
   util_dynarray_append(&pipeline->executables,
                        struct anv_pipeline_executable, exe);
}

static void
anv_pipeline_add_executables(struct anv_pipeline *pipeline,
                             struct anv_pipeline_stage *stage,
                             struct anv_shader_bin *bin)
{
   if (stage->stage == MESA_SHADER_FRAGMENT) {
      /* We pull the prog data and stats out of the anv_shader_bin because
       * the anv_pipeline_stage may not be fully populated if we successfully
       * looked up the shader in a cache.
       */
      const struct brw_wm_prog_data *wm_prog_data =
         (const struct brw_wm_prog_data *)bin->prog_data;
      struct brw_compile_stats *stats = bin->stats;

      if (wm_prog_data->dispatch_8) {
         anv_pipeline_add_executable(pipeline, stage, stats++, 0);
      }

      if (wm_prog_data->dispatch_16) {
         anv_pipeline_add_executable(pipeline, stage, stats++,
                                     wm_prog_data->prog_offset_16);
      }

      if (wm_prog_data->dispatch_32) {
         anv_pipeline_add_executable(pipeline, stage, stats++,
                                     wm_prog_data->prog_offset_32);
      }
   } else {
      anv_pipeline_add_executable(pipeline, stage, bin->stats, 0);
   }
}

static enum brw_subgroup_size_type
anv_subgroup_size_type(gl_shader_stage stage,
                       VkPipelineShaderStageCreateFlags flags,
                       const VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT *rss_info)
{
   enum brw_subgroup_size_type subgroup_size_type;

   if (rss_info) {
      assert(stage == MESA_SHADER_COMPUTE);
      /* These enum values are expressly chosen to be equal to the subgroup
       * size that they require.
       */
      assert(rss_info->requiredSubgroupSize == 8 ||
             rss_info->requiredSubgroupSize == 16 ||
             rss_info->requiredSubgroupSize == 32);
      subgroup_size_type = rss_info->requiredSubgroupSize;
   } else if (flags & VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT_EXT) {
      subgroup_size_type = BRW_SUBGROUP_SIZE_VARYING;
   } else if (flags & VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT_EXT) {
      assert(stage == MESA_SHADER_COMPUTE);
      /* If the client expressly requests full subgroups and they don't
       * specify a subgroup size neither allow varying subgroups, we need to
       * pick one.  So we specify the API value of 32.  Performance will
       * likely be terrible in this case but there's nothing we can do about
       * that.  The client should have chosen a size.
       */
      subgroup_size_type = BRW_SUBGROUP_SIZE_REQUIRE_32;
   } else {
      subgroup_size_type = BRW_SUBGROUP_SIZE_API_CONSTANT;
   }

   return subgroup_size_type;
}

static void
anv_pipeline_init_from_cached_graphics(struct anv_graphics_pipeline *pipeline)
{
   /* TODO: Cache this pipeline-wide information. */

   if (anv_pipeline_is_primitive(pipeline)) {
      /* Primitive replication depends on information from all the shaders.
       * Recover this bit from the fact that we have more than one position slot
       * in the vertex shader when using it.
       */
      assert(pipeline->active_stages & VK_SHADER_STAGE_VERTEX_BIT);
      int pos_slots = 0;
      const struct brw_vue_prog_data *vue_prog_data =
         (const void *) pipeline->shaders[MESA_SHADER_VERTEX]->prog_data;
      const struct brw_vue_map *vue_map = &vue_prog_data->vue_map;
      for (int i = 0; i < vue_map->num_slots; i++) {
         if (vue_map->slot_to_varying[i] == VARYING_SLOT_POS)
            pos_slots++;
      }
      pipeline->use_primitive_replication = pos_slots > 1;
   }
}

static VkResult
anv_pipeline_compile_graphics(struct anv_graphics_pipeline *pipeline,
                              struct anv_pipeline_cache *cache,
                              const VkGraphicsPipelineCreateInfo *info)
{
   VkPipelineCreationFeedbackEXT pipeline_feedback = {
      .flags = VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT_EXT,
   };
   int64_t pipeline_start = os_time_get_nano();

   const struct brw_compiler *compiler = pipeline->base.device->physical->compiler;
   struct anv_pipeline_stage stages[MESA_SHADER_STAGES] = {};

   /* Information on which states are considered dynamic. */
   const VkPipelineDynamicStateCreateInfo *dyn_info =
      info->pDynamicState;
   uint32_t dynamic_states = 0;
   if (dyn_info) {
      for (unsigned i = 0; i < dyn_info->dynamicStateCount; i++)
         dynamic_states |=
            anv_cmd_dirty_bit_for_vk_dynamic_state(dyn_info->pDynamicStates[i]);
   }

   VkResult result;
   for (uint32_t i = 0; i < info->stageCount; i++) {
      const VkPipelineShaderStageCreateInfo *sinfo = &info->pStages[i];
      gl_shader_stage stage = vk_to_mesa_shader_stage(sinfo->stage);

      int64_t stage_start = os_time_get_nano();

      stages[stage].stage = stage;
      stages[stage].module = vk_shader_module_from_handle(sinfo->module);
      stages[stage].entrypoint = sinfo->pName;
      stages[stage].spec_info = sinfo->pSpecializationInfo;
      anv_pipeline_hash_shader(stages[stage].module,
                               stages[stage].entrypoint,
                               stage,
                               stages[stage].spec_info,
                               stages[stage].shader_sha1);

      enum brw_subgroup_size_type subgroup_size_type =
         anv_subgroup_size_type(stage, sinfo->flags, NULL);

      const struct intel_device_info *devinfo = &pipeline->base.device->info;
      switch (stage) {
      case MESA_SHADER_VERTEX:
         populate_vs_prog_key(devinfo, subgroup_size_type,
                              pipeline->base.device->robust_buffer_access,
                              &stages[stage].key.vs);
         break;
      case MESA_SHADER_TESS_CTRL:
         populate_tcs_prog_key(devinfo, subgroup_size_type,
                               pipeline->base.device->robust_buffer_access,
                               info->pTessellationState->patchControlPoints,
                               &stages[stage].key.tcs);
         break;
      case MESA_SHADER_TESS_EVAL:
         populate_tes_prog_key(devinfo, subgroup_size_type,
                               pipeline->base.device->robust_buffer_access,
                               &stages[stage].key.tes);
         break;
      case MESA_SHADER_GEOMETRY:
         populate_gs_prog_key(devinfo, subgroup_size_type,
                              pipeline->base.device->robust_buffer_access,
                              &stages[stage].key.gs);
         break;
      case MESA_SHADER_FRAGMENT: {
         const bool raster_enabled =
            !info->pRasterizationState->rasterizerDiscardEnable ||
            dynamic_states & ANV_CMD_DIRTY_DYNAMIC_RASTERIZER_DISCARD_ENABLE;
         populate_wm_prog_key(pipeline, subgroup_size_type,
                              pipeline->base.device->robust_buffer_access,
                              pipeline->subpass,
                              raster_enabled ? info->pMultisampleState : NULL,
                              vk_find_struct_const(info->pNext,
                                                   PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR),
                              &stages[stage].key.wm);
         break;
      }
      default:
         unreachable("Invalid graphics shader stage");
      }

      stages[stage].feedback.duration += os_time_get_nano() - stage_start;
      stages[stage].feedback.flags |= VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT_EXT;
   }

   assert(pipeline->active_stages & VK_SHADER_STAGE_VERTEX_BIT);

   ANV_FROM_HANDLE(anv_pipeline_layout, layout, info->layout);

   unsigned char sha1[20];
   anv_pipeline_hash_graphics(pipeline, layout, stages, sha1);

   for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
      if (!stages[s].entrypoint)
         continue;

      stages[s].cache_key.stage = s;
      memcpy(stages[s].cache_key.sha1, sha1, sizeof(sha1));
   }

   const bool skip_cache_lookup =
      (pipeline->base.flags & VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR);

   if (!skip_cache_lookup) {
      unsigned found = 0;
      unsigned cache_hits = 0;
      for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
         if (!stages[s].entrypoint)
            continue;

         int64_t stage_start = os_time_get_nano();

         bool cache_hit;
         struct anv_shader_bin *bin =
            anv_device_search_for_kernel(pipeline->base.device, cache,
                                         &stages[s].cache_key,
                                         sizeof(stages[s].cache_key), &cache_hit);
         if (bin) {
            found++;
            pipeline->shaders[s] = bin;
         }

         if (cache_hit) {
            cache_hits++;
            stages[s].feedback.flags |=
               VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT_EXT;
         }
         stages[s].feedback.duration += os_time_get_nano() - stage_start;
      }

      if (found == __builtin_popcount(pipeline->active_stages)) {
         if (cache_hits == found) {
            pipeline_feedback.flags |=
               VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT_EXT;
         }
         /* We found all our shaders in the cache.  We're done. */
         for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
            if (!stages[s].entrypoint)
               continue;

            anv_pipeline_add_executables(&pipeline->base, &stages[s],
                                         pipeline->shaders[s]);
         }
         anv_pipeline_init_from_cached_graphics(pipeline);
         goto done;
      } else if (found > 0) {
         /* We found some but not all of our shaders.  This shouldn't happen
          * most of the time but it can if we have a partially populated
          * pipeline cache.
          */
         assert(found < __builtin_popcount(pipeline->active_stages));

         vk_perf(VK_LOG_OBJS(&cache->base),
                 "Found a partial pipeline in the cache.  This is "
                 "most likely caused by an incomplete pipeline cache "
                 "import or export");

         /* We're going to have to recompile anyway, so just throw away our
          * references to the shaders in the cache.  We'll get them out of the
          * cache again as part of the compilation process.
          */
         for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
            stages[s].feedback.flags = 0;
            if (pipeline->shaders[s]) {
               anv_shader_bin_unref(pipeline->base.device, pipeline->shaders[s]);
               pipeline->shaders[s] = NULL;
            }
         }
      }
   }

   if (info->flags & VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_EXT)
      return VK_PIPELINE_COMPILE_REQUIRED_EXT;

   void *pipeline_ctx = ralloc_context(NULL);

   for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
      if (!stages[s].entrypoint)
         continue;

      int64_t stage_start = os_time_get_nano();

      assert(stages[s].stage == s);
      assert(pipeline->shaders[s] == NULL);

      stages[s].bind_map = (struct anv_pipeline_bind_map) {
         .surface_to_descriptor = stages[s].surface_to_descriptor,
         .sampler_to_descriptor = stages[s].sampler_to_descriptor
      };

      stages[s].nir = anv_pipeline_stage_get_nir(&pipeline->base, cache,
                                                 pipeline_ctx,
                                                 &stages[s]);
      if (stages[s].nir == NULL) {
         result = vk_error(pipeline, VK_ERROR_UNKNOWN);
         goto fail;
      }

      /* This is rather ugly.
       *
       * Any variable annotated as interpolated by sample essentially disables
       * coarse pixel shading. Unfortunately the CTS tests exercising this set
       * the varying value in the previous stage using a constant. Our NIR
       * infrastructure is clever enough to lookup variables across stages and
       * constant fold, removing the variable. So in order to comply with CTS
       * we have check variables here.
       */
      if (s == MESA_SHADER_FRAGMENT) {
         nir_foreach_variable_in_list(var, &stages[s].nir->variables) {
            if (var->data.sample) {
               stages[s].key.wm.coarse_pixel = false;
               break;
            }
         }
      }

      stages[s].feedback.duration += os_time_get_nano() - stage_start;
   }

   /* Walk backwards to link */
   struct anv_pipeline_stage *next_stage = NULL;
   for (int s = ARRAY_SIZE(pipeline->shaders) - 1; s >= 0; s--) {
      if (!stages[s].entrypoint)
         continue;

      switch (s) {
      case MESA_SHADER_VERTEX:
         anv_pipeline_link_vs(compiler, &stages[s], next_stage);
         break;
      case MESA_SHADER_TESS_CTRL:
         anv_pipeline_link_tcs(compiler, &stages[s], next_stage);
         break;
      case MESA_SHADER_TESS_EVAL:
         anv_pipeline_link_tes(compiler, &stages[s], next_stage);
         break;
      case MESA_SHADER_GEOMETRY:
         anv_pipeline_link_gs(compiler, &stages[s], next_stage);
         break;
      case MESA_SHADER_FRAGMENT:
         anv_pipeline_link_fs(compiler, &stages[s]);
         break;
      default:
         unreachable("Invalid graphics shader stage");
      }

      next_stage = &stages[s];
   }

   if (pipeline->base.device->info.ver >= 12 &&
       pipeline->subpass->view_mask != 0) {
      /* For some pipelines HW Primitive Replication can be used instead of
       * instancing to implement Multiview.  This depend on how viewIndex is
       * used in all the active shaders, so this check can't be done per
       * individual shaders.
       */
      nir_shader *shaders[MESA_SHADER_STAGES] = {};
      for (unsigned s = 0; s < MESA_SHADER_STAGES; s++)
         shaders[s] = stages[s].nir;

      pipeline->use_primitive_replication =
         anv_check_for_primitive_replication(shaders, pipeline);
   } else {
      pipeline->use_primitive_replication = false;
   }

   struct anv_pipeline_stage *prev_stage = NULL;
   for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
      if (!stages[s].entrypoint)
         continue;

      int64_t stage_start = os_time_get_nano();

      void *stage_ctx = ralloc_context(NULL);

      anv_pipeline_lower_nir(&pipeline->base, stage_ctx, &stages[s], layout);

      if (prev_stage && compiler->glsl_compiler_options[s].NirOptions->unify_interfaces) {
         prev_stage->nir->info.outputs_written |= stages[s].nir->info.inputs_read &
                  ~(VARYING_BIT_TESS_LEVEL_INNER | VARYING_BIT_TESS_LEVEL_OUTER);
         stages[s].nir->info.inputs_read |= prev_stage->nir->info.outputs_written &
                  ~(VARYING_BIT_TESS_LEVEL_INNER | VARYING_BIT_TESS_LEVEL_OUTER);
         prev_stage->nir->info.patch_outputs_written |= stages[s].nir->info.patch_inputs_read;
         stages[s].nir->info.patch_inputs_read |= prev_stage->nir->info.patch_outputs_written;
      }

      ralloc_free(stage_ctx);

      stages[s].feedback.duration += os_time_get_nano() - stage_start;

      prev_stage = &stages[s];
   }

   prev_stage = NULL;
   for (unsigned s = 0; s < MESA_SHADER_STAGES; s++) {
      if (!stages[s].entrypoint)
         continue;

      int64_t stage_start = os_time_get_nano();

      void *stage_ctx = ralloc_context(NULL);

      nir_xfb_info *xfb_info = NULL;
      if (s == MESA_SHADER_VERTEX ||
          s == MESA_SHADER_TESS_EVAL ||
          s == MESA_SHADER_GEOMETRY)
         xfb_info = nir_gather_xfb_info(stages[s].nir, stage_ctx);

      switch (s) {
      case MESA_SHADER_VERTEX:
         anv_pipeline_compile_vs(compiler, stage_ctx, pipeline,
                                 &stages[s]);
         break;
      case MESA_SHADER_TESS_CTRL:
         anv_pipeline_compile_tcs(compiler, stage_ctx, pipeline->base.device,
                                  &stages[s], prev_stage);
         break;
      case MESA_SHADER_TESS_EVAL:
         anv_pipeline_compile_tes(compiler, stage_ctx, pipeline->base.device,
                                  &stages[s], prev_stage);
         break;
      case MESA_SHADER_GEOMETRY:
         anv_pipeline_compile_gs(compiler, stage_ctx, pipeline->base.device,
                                 &stages[s], prev_stage);
         break;
      case MESA_SHADER_FRAGMENT:
         anv_pipeline_compile_fs(compiler, stage_ctx, pipeline->base.device,
                                 &stages[s], prev_stage);
         break;
      default:
         unreachable("Invalid graphics shader stage");
      }
      if (stages[s].code == NULL) {
         ralloc_free(stage_ctx);
         result = vk_error(pipeline->base.device, VK_ERROR_OUT_OF_HOST_MEMORY);
         goto fail;
      }

      anv_nir_validate_push_layout(&stages[s].prog_data.base,
                                   &stages[s].bind_map);

      struct anv_shader_bin *bin =
         anv_device_upload_kernel(pipeline->base.device, cache, s,
                                  &stages[s].cache_key,
                                  sizeof(stages[s].cache_key),
                                  stages[s].code,
                                  stages[s].prog_data.base.program_size,
                                  &stages[s].prog_data.base,
                                  brw_prog_data_size(s),
                                  stages[s].stats, stages[s].num_stats,
                                  xfb_info, &stages[s].bind_map);
      if (!bin) {
         ralloc_free(stage_ctx);
         result = vk_error(pipeline, VK_ERROR_OUT_OF_HOST_MEMORY);
         goto fail;
      }

      anv_pipeline_add_executables(&pipeline->base, &stages[s], bin);

      pipeline->shaders[s] = bin;
      ralloc_free(stage_ctx);

      stages[s].feedback.duration += os_time_get_nano() - stage_start;

      prev_stage = &stages[s];
   }

   ralloc_free(pipeline_ctx);

done:

   if (pipeline->shaders[MESA_SHADER_FRAGMENT] &&
       pipeline->shaders[MESA_SHADER_FRAGMENT]->prog_data->program_size == 0) {
      /* This can happen if we decided to implicitly disable the fragment
       * shader.  See anv_pipeline_compile_fs().
       */
      anv_shader_bin_unref(pipeline->base.device,
                           pipeline->shaders[MESA_SHADER_FRAGMENT]);
      pipeline->shaders[MESA_SHADER_FRAGMENT] = NULL;
      pipeline->active_stages &= ~VK_SHADER_STAGE_FRAGMENT_BIT;
   }

   pipeline_feedback.duration = os_time_get_nano() - pipeline_start;

   const VkPipelineCreationFeedbackCreateInfoEXT *create_feedback =
      vk_find_struct_const(info->pNext, PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT);
   if (create_feedback) {
      *create_feedback->pPipelineCreationFeedback = pipeline_feedback;

      assert(info->stageCount == create_feedback->pipelineStageCreationFeedbackCount);
      for (uint32_t i = 0; i < info->stageCount; i++) {
         gl_shader_stage s = vk_to_mesa_shader_stage(info->pStages[i].stage);
         create_feedback->pPipelineStageCreationFeedbacks[i] = stages[s].feedback;
      }
   }

   return VK_SUCCESS;

fail:
   ralloc_free(pipeline_ctx);

   for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
      if (pipeline->shaders[s])
         anv_shader_bin_unref(pipeline->base.device, pipeline->shaders[s]);
   }

   return result;
}

VkResult
anv_pipeline_compile_cs(struct anv_compute_pipeline *pipeline,
                        struct anv_pipeline_cache *cache,
                        const VkComputePipelineCreateInfo *info,
                        const struct vk_shader_module *module,
                        const char *entrypoint,
                        const VkSpecializationInfo *spec_info)
{
   VkPipelineCreationFeedbackEXT pipeline_feedback = {
      .flags = VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT_EXT,
   };
   int64_t pipeline_start = os_time_get_nano();

   const struct brw_compiler *compiler = pipeline->base.device->physical->compiler;

   struct anv_pipeline_stage stage = {
      .stage = MESA_SHADER_COMPUTE,
      .module = module,
      .entrypoint = entrypoint,
      .spec_info = spec_info,
      .cache_key = {
         .stage = MESA_SHADER_COMPUTE,
      },
      .feedback = {
         .flags = VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT_EXT,
      },
   };
   anv_pipeline_hash_shader(stage.module,
                            stage.entrypoint,
                            MESA_SHADER_COMPUTE,
                            stage.spec_info,
                            stage.shader_sha1);

   struct anv_shader_bin *bin = NULL;

   const VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT *rss_info =
      vk_find_struct_const(info->stage.pNext,
                           PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT);

   const enum brw_subgroup_size_type subgroup_size_type =
      anv_subgroup_size_type(MESA_SHADER_COMPUTE, info->stage.flags, rss_info);

   populate_cs_prog_key(&pipeline->base.device->info, subgroup_size_type,
                        pipeline->base.device->robust_buffer_access,
                        &stage.key.cs);

   ANV_FROM_HANDLE(anv_pipeline_layout, layout, info->layout);

   const bool skip_cache_lookup =
      (pipeline->base.flags & VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR);

   anv_pipeline_hash_compute(pipeline, layout, &stage, stage.cache_key.sha1);

   bool cache_hit = false;
   if (!skip_cache_lookup) {
      bin = anv_device_search_for_kernel(pipeline->base.device, cache,
                                         &stage.cache_key,
                                         sizeof(stage.cache_key),
                                         &cache_hit);
   }

   if (bin == NULL &&
       (info->flags & VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_EXT))
      return VK_PIPELINE_COMPILE_REQUIRED_EXT;

   void *mem_ctx = ralloc_context(NULL);
   if (bin == NULL) {
      int64_t stage_start = os_time_get_nano();

      stage.bind_map = (struct anv_pipeline_bind_map) {
         .surface_to_descriptor = stage.surface_to_descriptor,
         .sampler_to_descriptor = stage.sampler_to_descriptor
      };

      /* Set up a binding for the gl_NumWorkGroups */
      stage.bind_map.surface_count = 1;
      stage.bind_map.surface_to_descriptor[0] = (struct anv_pipeline_binding) {
         .set = ANV_DESCRIPTOR_SET_NUM_WORK_GROUPS,
      };

      stage.nir = anv_pipeline_stage_get_nir(&pipeline->base, cache, mem_ctx, &stage);
      if (stage.nir == NULL) {
         ralloc_free(mem_ctx);
         return vk_error(pipeline, VK_ERROR_UNKNOWN);
      }

      NIR_PASS_V(stage.nir, anv_nir_add_base_work_group_id);

      anv_pipeline_lower_nir(&pipeline->base, mem_ctx, &stage, layout);

      NIR_PASS_V(stage.nir, brw_nir_lower_cs_intrinsics);

      stage.num_stats = 1;

      struct brw_compile_cs_params params = {
         .nir = stage.nir,
         .key = &stage.key.cs,
         .prog_data = &stage.prog_data.cs,
         .stats = stage.stats,
         .log_data = pipeline->base.device,
      };

      stage.code = brw_compile_cs(compiler, mem_ctx, &params);
      if (stage.code == NULL) {
         ralloc_free(mem_ctx);
         return vk_error(pipeline, VK_ERROR_OUT_OF_HOST_MEMORY);
      }

      anv_nir_validate_push_layout(&stage.prog_data.base, &stage.bind_map);

      if (!stage.prog_data.cs.uses_num_work_groups) {
         assert(stage.bind_map.surface_to_descriptor[0].set ==
                ANV_DESCRIPTOR_SET_NUM_WORK_GROUPS);
         stage.bind_map.surface_to_descriptor[0].set = ANV_DESCRIPTOR_SET_NULL;
      }

      const unsigned code_size = stage.prog_data.base.program_size;
      bin = anv_device_upload_kernel(pipeline->base.device, cache,
                                     MESA_SHADER_COMPUTE,
                                     &stage.cache_key, sizeof(stage.cache_key),
                                     stage.code, code_size,
                                     &stage.prog_data.base,
                                     sizeof(stage.prog_data.cs),
                                     stage.stats, stage.num_stats,
                                     NULL, &stage.bind_map);
      if (!bin) {
         ralloc_free(mem_ctx);
         return vk_error(pipeline, VK_ERROR_OUT_OF_HOST_MEMORY);
      }

      stage.feedback.duration = os_time_get_nano() - stage_start;
   }

   anv_pipeline_add_executables(&pipeline->base, &stage, bin);

   ralloc_free(mem_ctx);

   if (cache_hit) {
      stage.feedback.flags |=
         VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT_EXT;
      pipeline_feedback.flags |=
         VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT_EXT;
   }
   pipeline_feedback.duration = os_time_get_nano() - pipeline_start;

   const VkPipelineCreationFeedbackCreateInfoEXT *create_feedback =
      vk_find_struct_const(info->pNext, PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT);
   if (create_feedback) {
      *create_feedback->pPipelineCreationFeedback = pipeline_feedback;

      assert(create_feedback->pipelineStageCreationFeedbackCount == 1);
      create_feedback->pPipelineStageCreationFeedbacks[0] = stage.feedback;
   }

   pipeline->cs = bin;

   return VK_SUCCESS;
}

/**
 * Copy pipeline state not marked as dynamic.
 * Dynamic state is pipeline state which hasn't been provided at pipeline
 * creation time, but is dynamically provided afterwards using various
 * vkCmdSet* functions.
 *
 * The set of state considered "non_dynamic" is determined by the pieces of
 * state that have their corresponding VkDynamicState enums omitted from
 * VkPipelineDynamicStateCreateInfo::pDynamicStates.
 *
 * @param[out] pipeline    Destination non_dynamic state.
 * @param[in]  pCreateInfo Source of non_dynamic state to be copied.
 */
static void
copy_non_dynamic_state(struct anv_graphics_pipeline *pipeline,
                       const VkGraphicsPipelineCreateInfo *pCreateInfo)
{
   anv_cmd_dirty_mask_t states = ANV_CMD_DIRTY_DYNAMIC_ALL;
   struct anv_subpass *subpass = pipeline->subpass;

   pipeline->dynamic_state = default_dynamic_state;

   states &= ~pipeline->dynamic_states;

   struct anv_dynamic_state *dynamic = &pipeline->dynamic_state;

   bool raster_discard =
      pCreateInfo->pRasterizationState->rasterizerDiscardEnable &&
      !(pipeline->dynamic_states & ANV_CMD_DIRTY_DYNAMIC_RASTERIZER_DISCARD_ENABLE);

   /* Section 9.2 of the Vulkan 1.0.15 spec says:
    *
    *    pViewportState is [...] NULL if the pipeline
    *    has rasterization disabled.
    */
   if (!raster_discard) {
      assert(pCreateInfo->pViewportState);

      dynamic->viewport.count = pCreateInfo->pViewportState->viewportCount;
      if (states & ANV_CMD_DIRTY_DYNAMIC_VIEWPORT) {
         typed_memcpy(dynamic->viewport.viewports,
                     pCreateInfo->pViewportState->pViewports,
                     pCreateInfo->pViewportState->viewportCount);
      }

      dynamic->scissor.count = pCreateInfo->pViewportState->scissorCount;
      if (states & ANV_CMD_DIRTY_DYNAMIC_SCISSOR) {
         typed_memcpy(dynamic->scissor.scissors,
                     pCreateInfo->pViewportState->pScissors,
                     pCreateInfo->pViewportState->scissorCount);
      }
   }

   if (states & ANV_CMD_DIRTY_DYNAMIC_LINE_WIDTH) {
      assert(pCreateInfo->pRasterizationState);
      dynamic->line_width = pCreateInfo->pRasterizationState->lineWidth;
   }

   if (states & ANV_CMD_DIRTY_DYNAMIC_DEPTH_BIAS) {
      assert(pCreateInfo->pRasterizationState);
      dynamic->depth_bias.bias =
         pCreateInfo->pRasterizationState->depthBiasConstantFactor;
      dynamic->depth_bias.clamp =
         pCreateInfo->pRasterizationState->depthBiasClamp;
      dynamic->depth_bias.slope =
         pCreateInfo->pRasterizationState->depthBiasSlopeFactor;
   }

   if (states & ANV_CMD_DIRTY_DYNAMIC_CULL_MODE) {
      assert(pCreateInfo->pRasterizationState);
      dynamic->cull_mode =
         pCreateInfo->pRasterizationState->cullMode;
   }

   if (states & ANV_CMD_DIRTY_DYNAMIC_FRONT_FACE) {
      assert(pCreateInfo->pRasterizationState);
      dynamic->front_face =
         pCreateInfo->pRasterizationState->frontFace;
   }

   if ((states & ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY) &&
         (pipeline->active_stages & VK_SHADER_STAGE_VERTEX_BIT)) {
      assert(pCreateInfo->pInputAssemblyState);
      dynamic->primitive_topology = pCreateInfo->pInputAssemblyState->topology;
   }

   if (states & ANV_CMD_DIRTY_DYNAMIC_RASTERIZER_DISCARD_ENABLE) {
      assert(pCreateInfo->pRasterizationState);
      dynamic->raster_discard =
         pCreateInfo->pRasterizationState->rasterizerDiscardEnable;
   }

   if (states & ANV_CMD_DIRTY_DYNAMIC_DEPTH_BIAS_ENABLE) {
      assert(pCreateInfo->pRasterizationState);
      dynamic->depth_bias_enable =
         pCreateInfo->pRasterizationState->depthBiasEnable;
   }

   if ((states & ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_RESTART_ENABLE) &&
         (pipeline->active_stages & VK_SHADER_STAGE_VERTEX_BIT)) {
      assert(pCreateInfo->pInputAssemblyState);
      dynamic->primitive_restart_enable =
         pCreateInfo->pInputAssemblyState->primitiveRestartEnable;
   }

   /* Section 9.2 of the Vulkan 1.0.15 spec says:
    *
    *    pColorBlendState is [...] NULL if the pipeline has rasterization
    *    disabled or if the subpass of the render pass the pipeline is
    *    created against does not use any color attachments.
    */
   bool uses_color_att = false;
   for (unsigned i = 0; i < subpass->color_count; ++i) {
      if (subpass->color_attachments[i].attachment != VK_ATTACHMENT_UNUSED) {
         uses_color_att = true;
         break;
      }
   }

   if (uses_color_att && !raster_discard) {
      assert(pCreateInfo->pColorBlendState);

      if (states & ANV_CMD_DIRTY_DYNAMIC_BLEND_CONSTANTS)
         typed_memcpy(dynamic->blend_constants,
                     pCreateInfo->pColorBlendState->blendConstants, 4);
   }

   /* If there is no depthstencil attachment, then don't read
    * pDepthStencilState. The Vulkan spec states that pDepthStencilState may
    * be NULL in this case. Even if pDepthStencilState is non-NULL, there is
    * no need to override the depthstencil defaults in
    * anv_pipeline::dynamic_state when there is no depthstencil attachment.
    *
    * Section 9.2 of the Vulkan 1.0.15 spec says:
    *
    *    pDepthStencilState is [...] NULL if the pipeline has rasterization
    *    disabled or if the subpass of the render pass the pipeline is created
    *    against does not use a depth/stencil attachment.
    */
   if (!raster_discard && subpass->depth_stencil_attachment) {
      assert(pCreateInfo->pDepthStencilState);

      if (states & ANV_CMD_DIRTY_DYNAMIC_DEPTH_BOUNDS) {
         dynamic->depth_bounds.min =
            pCreateInfo->pDepthStencilState->minDepthBounds;
         dynamic->depth_bounds.max =
            pCreateInfo->pDepthStencilState->maxDepthBounds;
      }

      if (states & ANV_CMD_DIRTY_DYNAMIC_STENCIL_COMPARE_MASK) {
         dynamic->stencil_compare_mask.front =
            pCreateInfo->pDepthStencilState->front.compareMask;
         dynamic->stencil_compare_mask.back =
            pCreateInfo->pDepthStencilState->back.compareMask;
      }

      if (states & ANV_CMD_DIRTY_DYNAMIC_STENCIL_WRITE_MASK) {
         dynamic->stencil_write_mask.front =
            pCreateInfo->pDepthStencilState->front.writeMask;
         dynamic->stencil_write_mask.back =
            pCreateInfo->pDepthStencilState->back.writeMask;
      }

      if (states & ANV_CMD_DIRTY_DYNAMIC_STENCIL_REFERENCE) {
         dynamic->stencil_reference.front =
            pCreateInfo->pDepthStencilState->front.reference;
         dynamic->stencil_reference.back =
            pCreateInfo->pDepthStencilState->back.reference;
      }

      if (states & ANV_CMD_DIRTY_DYNAMIC_DEPTH_TEST_ENABLE) {
         dynamic->depth_test_enable =
            pCreateInfo->pDepthStencilState->depthTestEnable;
      }

      if (states & ANV_CMD_DIRTY_DYNAMIC_DEPTH_WRITE_ENABLE) {
         dynamic->depth_write_enable =
            pCreateInfo->pDepthStencilState->depthWriteEnable;
      }

      if (states & ANV_CMD_DIRTY_DYNAMIC_DEPTH_COMPARE_OP) {
         dynamic->depth_compare_op =
            pCreateInfo->pDepthStencilState->depthCompareOp;
      }

      if (states & ANV_CMD_DIRTY_DYNAMIC_DEPTH_BOUNDS_TEST_ENABLE) {
         dynamic->depth_bounds_test_enable =
            pCreateInfo->pDepthStencilState->depthBoundsTestEnable;
      }

      if (states & ANV_CMD_DIRTY_DYNAMIC_STENCIL_TEST_ENABLE) {
         dynamic->stencil_test_enable =
            pCreateInfo->pDepthStencilState->stencilTestEnable;
      }

      if (states & ANV_CMD_DIRTY_DYNAMIC_STENCIL_OP) {
         const VkPipelineDepthStencilStateCreateInfo *info =
            pCreateInfo->pDepthStencilState;
         memcpy(&dynamic->stencil_op.front, &info->front,
                sizeof(dynamic->stencil_op.front));
         memcpy(&dynamic->stencil_op.back, &info->back,
                sizeof(dynamic->stencil_op.back));
      }
   }

   const VkPipelineRasterizationLineStateCreateInfoEXT *line_state =
      vk_find_struct_const(pCreateInfo->pRasterizationState->pNext,
                           PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT);
   if (!raster_discard && line_state && line_state->stippledLineEnable) {
      if (states & ANV_CMD_DIRTY_DYNAMIC_LINE_STIPPLE) {
         dynamic->line_stipple.factor = line_state->lineStippleFactor;
         dynamic->line_stipple.pattern = line_state->lineStipplePattern;
      }
   }

   const VkPipelineMultisampleStateCreateInfo *ms_info =
      pCreateInfo->pRasterizationState->rasterizerDiscardEnable ? NULL :
      pCreateInfo->pMultisampleState;
   if (states & ANV_CMD_DIRTY_DYNAMIC_SAMPLE_LOCATIONS) {
      const VkPipelineSampleLocationsStateCreateInfoEXT *sl_info = ms_info ?
         vk_find_struct_const(ms_info, PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT) : NULL;

      if (sl_info) {
         dynamic->sample_locations.samples =
            sl_info->sampleLocationsInfo.sampleLocationsCount;
         const VkSampleLocationEXT *positions =
            sl_info->sampleLocationsInfo.pSampleLocations;
         for (uint32_t i = 0; i < dynamic->sample_locations.samples; i++) {
            dynamic->sample_locations.locations[i].x = positions[i].x;
            dynamic->sample_locations.locations[i].y = positions[i].y;
         }
      }
   }
   /* Ensure we always have valid values for sample_locations. */
   if (pipeline->base.device->vk.enabled_extensions.EXT_sample_locations &&
       dynamic->sample_locations.samples == 0) {
      dynamic->sample_locations.samples =
         ms_info ? ms_info->rasterizationSamples : 1;
      const struct intel_sample_position *positions =
         intel_get_sample_positions(dynamic->sample_locations.samples);
      for (uint32_t i = 0; i < dynamic->sample_locations.samples; i++) {
         dynamic->sample_locations.locations[i].x = positions[i].x;
         dynamic->sample_locations.locations[i].y = positions[i].y;
      }
   }

   if (states & ANV_CMD_DIRTY_DYNAMIC_COLOR_BLEND_STATE) {
      if (!pCreateInfo->pRasterizationState->rasterizerDiscardEnable &&
          uses_color_att) {
         assert(pCreateInfo->pColorBlendState);
         const VkPipelineColorWriteCreateInfoEXT *color_write_info =
            vk_find_struct_const(pCreateInfo->pColorBlendState->pNext,
                                 PIPELINE_COLOR_WRITE_CREATE_INFO_EXT);

         if (color_write_info) {
            dynamic->color_writes = 0;
            for (uint32_t i = 0; i < color_write_info->attachmentCount; i++) {
               dynamic->color_writes |=
                  color_write_info->pColorWriteEnables[i] ? (1u << i) : 0;
            }
         }
      }
   }

   const VkPipelineFragmentShadingRateStateCreateInfoKHR *fsr_state =
      vk_find_struct_const(pCreateInfo->pNext,
                           PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR);
   if (fsr_state) {
      if (states & ANV_CMD_DIRTY_DYNAMIC_SHADING_RATE)
         dynamic->fragment_shading_rate = fsr_state->fragmentSize;
   }

   pipeline->dynamic_state_mask = states;

   /* Mark states that can either be dynamic or fully baked into the pipeline.
    */
   pipeline->static_state_mask = states &
      (ANV_CMD_DIRTY_DYNAMIC_SAMPLE_LOCATIONS |
       ANV_CMD_DIRTY_DYNAMIC_COLOR_BLEND_STATE |
       ANV_CMD_DIRTY_DYNAMIC_SHADING_RATE |
       ANV_CMD_DIRTY_DYNAMIC_RASTERIZER_DISCARD_ENABLE |
       ANV_CMD_DIRTY_DYNAMIC_LOGIC_OP |
       ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY);
}

static void
anv_pipeline_validate_create_info(const VkGraphicsPipelineCreateInfo *info)
{
#ifdef DEBUG
   struct anv_render_pass *renderpass = NULL;
   struct anv_subpass *subpass = NULL;

   /* Assert that all required members of VkGraphicsPipelineCreateInfo are
    * present.  See the Vulkan 1.0.28 spec, Section 9.2 Graphics Pipelines.
    */
   assert(info->sType == VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);

   renderpass = anv_render_pass_from_handle(info->renderPass);
   assert(renderpass);

   assert(info->subpass < renderpass->subpass_count);
   subpass = &renderpass->subpasses[info->subpass];

   assert(info->stageCount >= 1);
   assert(info->pRasterizationState);
   if (!info->pRasterizationState->rasterizerDiscardEnable) {
      assert(info->pViewportState);
      assert(info->pMultisampleState);

      if (subpass && subpass->depth_stencil_attachment)
         assert(info->pDepthStencilState);

      if (subpass && subpass->color_count > 0) {
         bool all_color_unused = true;
         for (int i = 0; i < subpass->color_count; i++) {
            if (subpass->color_attachments[i].attachment != VK_ATTACHMENT_UNUSED)
               all_color_unused = false;
         }
         /* pColorBlendState is ignored if the pipeline has rasterization
          * disabled or if the subpass of the render pass the pipeline is
          * created against does not use any color attachments.
          */
         assert(info->pColorBlendState || all_color_unused);
      }
   }

   for (uint32_t i = 0; i < info->stageCount; ++i) {
      switch (info->pStages[i].stage) {
      case VK_SHADER_STAGE_VERTEX_BIT:
         assert(info->pVertexInputState);
         assert(info->pInputAssemblyState);
         break;
      case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
      case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
         assert(info->pTessellationState);
         break;
      default:
         break;
      }
   }
#endif
}

/**
 * Calculate the desired L3 partitioning based on the current state of the
 * pipeline.  For now this simply returns the conservative defaults calculated
 * by get_default_l3_weights(), but we could probably do better by gathering
 * more statistics from the pipeline state (e.g. guess of expected URB usage
 * and bound surfaces), or by using feed-back from performance counters.
 */
void
anv_pipeline_setup_l3_config(struct anv_pipeline *pipeline, bool needs_slm)
{
   const struct intel_device_info *devinfo = &pipeline->device->info;

   const struct intel_l3_weights w =
      intel_get_default_l3_weights(devinfo, true, needs_slm);

   pipeline->l3_config = intel_get_l3_config(devinfo, w);
}

static VkLineRasterizationModeEXT
vk_line_rasterization_mode(const VkPipelineRasterizationLineStateCreateInfoEXT *line_info,
                           const VkPipelineMultisampleStateCreateInfo *ms_info)
{
   VkLineRasterizationModeEXT line_mode =
      line_info ? line_info->lineRasterizationMode :
                  VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT;

   if (line_mode == VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT) {
      if (ms_info && ms_info->rasterizationSamples > 1) {
         return VK_LINE_RASTERIZATION_MODE_RECTANGULAR_EXT;
      } else {
         return VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT;
      }
   }

   return line_mode;
}

VkResult
anv_graphics_pipeline_init(struct anv_graphics_pipeline *pipeline,
                           struct anv_device *device,
                           struct anv_pipeline_cache *cache,
                           const VkGraphicsPipelineCreateInfo *pCreateInfo,
                           const VkAllocationCallbacks *alloc)
{
   VkResult result;

   anv_pipeline_validate_create_info(pCreateInfo);

   result = anv_pipeline_init(&pipeline->base, device,
                              ANV_PIPELINE_GRAPHICS, pCreateInfo->flags,
                              alloc);
   if (result != VK_SUCCESS)
      return result;

   anv_batch_set_storage(&pipeline->base.batch, ANV_NULL_ADDRESS,
                         pipeline->batch_data, sizeof(pipeline->batch_data));

   ANV_FROM_HANDLE(anv_render_pass, render_pass, pCreateInfo->renderPass);
   assert(pCreateInfo->subpass < render_pass->subpass_count);
   pipeline->subpass = &render_pass->subpasses[pCreateInfo->subpass];

   assert(pCreateInfo->pRasterizationState);

   if (pCreateInfo->pDynamicState) {
      /* Remove all of the states that are marked as dynamic */
      uint32_t count = pCreateInfo->pDynamicState->dynamicStateCount;
      for (uint32_t s = 0; s < count; s++) {
         pipeline->dynamic_states |= anv_cmd_dirty_bit_for_vk_dynamic_state(
            pCreateInfo->pDynamicState->pDynamicStates[s]);
      }
   }

   pipeline->active_stages = 0;
   for (uint32_t i = 0; i < pCreateInfo->stageCount; i++)
      pipeline->active_stages |= pCreateInfo->pStages[i].stage;

   if (pipeline->active_stages & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
      pipeline->active_stages |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

   copy_non_dynamic_state(pipeline, pCreateInfo);

   pipeline->depth_clamp_enable = pCreateInfo->pRasterizationState->depthClampEnable;

   /* Previously we enabled depth clipping when !depthClampEnable.
    * DepthClipStateCreateInfo now makes depth clipping explicit so if the
    * clipping info is available, use its enable value to determine clipping,
    * otherwise fallback to the previous !depthClampEnable logic.
    */
   const VkPipelineRasterizationDepthClipStateCreateInfoEXT *clip_info =
      vk_find_struct_const(pCreateInfo->pRasterizationState->pNext,
                           PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT);
   pipeline->depth_clip_enable = clip_info ? clip_info->depthClipEnable : !pipeline->depth_clamp_enable;

   pipeline->sample_shading_enable =
      !pCreateInfo->pRasterizationState->rasterizerDiscardEnable &&
      pCreateInfo->pMultisampleState &&
      pCreateInfo->pMultisampleState->sampleShadingEnable;

   result = anv_pipeline_compile_graphics(pipeline, cache, pCreateInfo);
   if (result != VK_SUCCESS) {
      anv_pipeline_finish(&pipeline->base, device, alloc);
      return result;
   }

   anv_pipeline_setup_l3_config(&pipeline->base, false);

   if (anv_pipeline_is_primitive(pipeline)) {
      const VkPipelineVertexInputStateCreateInfo *vi_info =
         pCreateInfo->pVertexInputState;

      const uint64_t inputs_read = get_vs_prog_data(pipeline)->inputs_read;

      for (uint32_t i = 0; i < vi_info->vertexAttributeDescriptionCount; i++) {
         const VkVertexInputAttributeDescription *desc =
            &vi_info->pVertexAttributeDescriptions[i];

         if (inputs_read & (1ull << (VERT_ATTRIB_GENERIC0 + desc->location)))
            pipeline->vb_used |= 1 << desc->binding;
      }

      for (uint32_t i = 0; i < vi_info->vertexBindingDescriptionCount; i++) {
         const VkVertexInputBindingDescription *desc =
            &vi_info->pVertexBindingDescriptions[i];

         pipeline->vb[desc->binding].stride = desc->stride;

         /* Step rate is programmed per vertex element (attribute), not
          * binding. Set up a map of which bindings step per instance, for
          * reference by vertex element setup. */
         switch (desc->inputRate) {
         default:
         case VK_VERTEX_INPUT_RATE_VERTEX:
            pipeline->vb[desc->binding].instanced = false;
            break;
         case VK_VERTEX_INPUT_RATE_INSTANCE:
            pipeline->vb[desc->binding].instanced = true;
            break;
         }

         pipeline->vb[desc->binding].instance_divisor = 1;
      }

      const VkPipelineVertexInputDivisorStateCreateInfoEXT *vi_div_state =
         vk_find_struct_const(vi_info->pNext,
                              PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT);
      if (vi_div_state) {
         for (uint32_t i = 0; i < vi_div_state->vertexBindingDivisorCount; i++) {
            const VkVertexInputBindingDivisorDescriptionEXT *desc =
               &vi_div_state->pVertexBindingDivisors[i];

            pipeline->vb[desc->binding].instance_divisor = desc->divisor;
         }
      }

      /* Our implementation of VK_KHR_multiview uses instancing to draw the
       * different views.  If the client asks for instancing, we need to multiply
       * the instance divisor by the number of views ensure that we repeat the
       * client's per-instance data once for each view.
       */
      if (pipeline->subpass->view_mask && !pipeline->use_primitive_replication) {
         const uint32_t view_count = anv_subpass_view_count(pipeline->subpass);
         for (uint32_t vb = 0; vb < MAX_VBS; vb++) {
            if (pipeline->vb[vb].instanced)
               pipeline->vb[vb].instance_divisor *= view_count;
         }
      }

      const VkPipelineInputAssemblyStateCreateInfo *ia_info =
         pCreateInfo->pInputAssemblyState;
      const VkPipelineTessellationStateCreateInfo *tess_info =
         pCreateInfo->pTessellationState;

      if (anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL))
         pipeline->topology = _3DPRIM_PATCHLIST(tess_info->patchControlPoints);
      else
         pipeline->topology = vk_to_intel_primitive_type[ia_info->topology];
   }

   /* If rasterization is not enabled, ms_info must be ignored. */
   const bool raster_enabled =
      !pCreateInfo->pRasterizationState->rasterizerDiscardEnable ||
      (pipeline->dynamic_states &
       ANV_CMD_DIRTY_DYNAMIC_RASTERIZER_DISCARD_ENABLE);

   const VkPipelineMultisampleStateCreateInfo *ms_info =
      raster_enabled ? pCreateInfo->pMultisampleState : NULL;

   const VkPipelineRasterizationLineStateCreateInfoEXT *line_info =
      vk_find_struct_const(pCreateInfo->pRasterizationState->pNext,
                           PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT);

   /* Store line mode, polygon mode and rasterization samples, these are used
    * for dynamic primitive topology.
    */
   pipeline->line_mode = vk_line_rasterization_mode(line_info, ms_info);
   pipeline->polygon_mode = pCreateInfo->pRasterizationState->polygonMode;
   pipeline->rasterization_samples =
      ms_info ? ms_info->rasterizationSamples : 1;

   return VK_SUCCESS;
}

static VkResult
compile_upload_rt_shader(struct anv_ray_tracing_pipeline *pipeline,
                         struct anv_pipeline_cache *cache,
                         nir_shader *nir,
                         struct anv_pipeline_stage *stage,
                         struct anv_shader_bin **shader_out,
                         void *mem_ctx)
{
   const struct brw_compiler *compiler =
      pipeline->base.device->physical->compiler;
   const struct intel_device_info *devinfo = compiler->devinfo;

   nir_shader **resume_shaders = NULL;
   uint32_t num_resume_shaders = 0;
   if (nir->info.stage != MESA_SHADER_COMPUTE) {
      NIR_PASS_V(nir, nir_lower_shader_calls,
                 nir_address_format_64bit_global,
                 BRW_BTD_STACK_ALIGN,
                 &resume_shaders, &num_resume_shaders, mem_ctx);
      NIR_PASS_V(nir, brw_nir_lower_shader_calls);
      NIR_PASS_V(nir, brw_nir_lower_rt_intrinsics, devinfo);
   }

   for (unsigned i = 0; i < num_resume_shaders; i++) {
      NIR_PASS_V(resume_shaders[i], brw_nir_lower_shader_calls);
      NIR_PASS_V(resume_shaders[i], brw_nir_lower_rt_intrinsics, devinfo);
   }

   stage->code =
      brw_compile_bs(compiler, pipeline->base.device, mem_ctx,
                     &stage->key.bs, &stage->prog_data.bs, nir,
                     num_resume_shaders, resume_shaders, stage->stats, NULL);
   if (stage->code == NULL)
      return vk_error(pipeline, VK_ERROR_OUT_OF_HOST_MEMORY);

   /* Ray-tracing shaders don't have a "real" bind map */
   struct anv_pipeline_bind_map empty_bind_map = {};

   const unsigned code_size = stage->prog_data.base.program_size;
   struct anv_shader_bin *bin =
      anv_device_upload_kernel(pipeline->base.device,
                               cache,
                               stage->stage,
                               &stage->cache_key, sizeof(stage->cache_key),
                               stage->code, code_size,
                               &stage->prog_data.base,
                               sizeof(stage->prog_data.bs),
                               stage->stats, 1,
                               NULL, &empty_bind_map);
   if (bin == NULL)
      return vk_error(pipeline, VK_ERROR_OUT_OF_HOST_MEMORY);

   /* TODO: Figure out executables for resume shaders */
   anv_pipeline_add_executables(&pipeline->base, stage, bin);
   util_dynarray_append(&pipeline->shaders, struct anv_shader_bin *, bin);

   *shader_out = bin;

   return VK_SUCCESS;
}

static bool
is_rt_stack_size_dynamic(const VkRayTracingPipelineCreateInfoKHR *info)
{
   if (info->pDynamicState == NULL)
      return false;

   for (unsigned i = 0; i < info->pDynamicState->dynamicStateCount; i++) {
      if (info->pDynamicState->pDynamicStates[i] ==
          VK_DYNAMIC_STATE_RAY_TRACING_PIPELINE_STACK_SIZE_KHR)
         return true;
   }

   return false;
}

static void
anv_pipeline_compute_ray_tracing_stacks(struct anv_ray_tracing_pipeline *pipeline,
                                        const VkRayTracingPipelineCreateInfoKHR *info,
                                        uint32_t *stack_max)
{
   if (is_rt_stack_size_dynamic(info)) {
      pipeline->stack_size = 0; /* 0 means dynamic */
   } else {
      /* From the Vulkan spec:
       *
       *    "If the stack size is not set explicitly, the stack size for a
       *    pipeline is:
       *
       *       rayGenStackMax +
       *       min(1, maxPipelineRayRecursionDepth) ×
       *       max(closestHitStackMax, missStackMax,
       *           intersectionStackMax + anyHitStackMax) +
       *       max(0, maxPipelineRayRecursionDepth-1) ×
       *       max(closestHitStackMax, missStackMax) +
       *       2 × callableStackMax"
       */
      pipeline->stack_size =
         stack_max[MESA_SHADER_RAYGEN] +
         MIN2(1, info->maxPipelineRayRecursionDepth) *
         MAX4(stack_max[MESA_SHADER_CLOSEST_HIT],
              stack_max[MESA_SHADER_MISS],
              stack_max[MESA_SHADER_INTERSECTION],
              stack_max[MESA_SHADER_ANY_HIT]) +
         MAX2(0, (int)info->maxPipelineRayRecursionDepth - 1) *
         MAX2(stack_max[MESA_SHADER_CLOSEST_HIT],
              stack_max[MESA_SHADER_MISS]) +
         2 * stack_max[MESA_SHADER_CALLABLE];

      /* This is an extremely unlikely case but we need to set it to some
       * non-zero value so that we don't accidentally think it's dynamic.
       * Our minimum stack size is 2KB anyway so we could set to any small
       * value we like.
       */
      if (pipeline->stack_size == 0)
         pipeline->stack_size = 1;
   }
}

static struct anv_pipeline_stage *
anv_pipeline_init_ray_tracing_stages(struct anv_ray_tracing_pipeline *pipeline,
                                     const VkRayTracingPipelineCreateInfoKHR *info,
                                     void *pipeline_ctx)
{
   ANV_FROM_HANDLE(anv_pipeline_layout, layout, info->layout);

   /* Create enough stage entries for all shader modules plus potential
    * combinaisons in the groups.
    */
   struct anv_pipeline_stage *stages =
      rzalloc_array(pipeline_ctx, struct anv_pipeline_stage, info->stageCount);

   for (uint32_t i = 0; i < info->stageCount; i++) {
      const VkPipelineShaderStageCreateInfo *sinfo = &info->pStages[i];
      if (sinfo->module == VK_NULL_HANDLE)
         continue;

      int64_t stage_start = os_time_get_nano();

      stages[i] = (struct anv_pipeline_stage) {
         .stage = vk_to_mesa_shader_stage(sinfo->stage),
         .module = vk_shader_module_from_handle(sinfo->module),
         .entrypoint = sinfo->pName,
         .spec_info = sinfo->pSpecializationInfo,
         .cache_key = {
            .stage = vk_to_mesa_shader_stage(sinfo->stage),
         },
         .feedback = {
            .flags = VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT_EXT,
         },
      };

      populate_bs_prog_key(&pipeline->base.device->info, sinfo->flags,
                           pipeline->base.device->robust_buffer_access,
                           &stages[i].key.bs);

      anv_pipeline_hash_shader(stages[i].module,
                               stages[i].entrypoint,
                               stages[i].stage,
                               stages[i].spec_info,
                               stages[i].shader_sha1);

      if (stages[i].stage != MESA_SHADER_INTERSECTION) {
         anv_pipeline_hash_ray_tracing_shader(pipeline, layout, &stages[i],
                                              stages[i].cache_key.sha1);
      }

      stages[i].feedback.duration += os_time_get_nano() - stage_start;
   }

   for (uint32_t i = 0; i < info->groupCount; i++) {
      const VkRayTracingShaderGroupCreateInfoKHR *ginfo = &info->pGroups[i];

      if (ginfo->type != VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR)
         continue;

      int64_t stage_start = os_time_get_nano();

      uint32_t intersection_idx = ginfo->intersectionShader;
      assert(intersection_idx < info->stageCount);

      uint32_t any_hit_idx = ginfo->anyHitShader;
      if (any_hit_idx != VK_SHADER_UNUSED_KHR) {
         assert(any_hit_idx < info->stageCount);
         anv_pipeline_hash_ray_tracing_combined_shader(pipeline,
                                                       layout,
                                                       &stages[intersection_idx],
                                                       &stages[any_hit_idx],
                                                       stages[intersection_idx].cache_key.sha1);
      } else {
         anv_pipeline_hash_ray_tracing_shader(pipeline, layout,
                                              &stages[intersection_idx],
                                              stages[intersection_idx].cache_key.sha1);
      }

      stages[intersection_idx].feedback.duration += os_time_get_nano() - stage_start;
   }

   return stages;
}

static bool
anv_pipeline_load_cached_shaders(struct anv_ray_tracing_pipeline *pipeline,
                                 struct anv_pipeline_cache *cache,
                                 const VkRayTracingPipelineCreateInfoKHR *info,
                                 struct anv_pipeline_stage *stages,
                                 uint32_t *stack_max)
{
   uint32_t shaders = 0, cache_hits = 0;
   for (uint32_t i = 0; i < info->stageCount; i++) {
      if (stages[i].entrypoint == NULL)
         continue;

      shaders++;

      int64_t stage_start = os_time_get_nano();

      bool cache_hit;
      stages[i].bin = anv_device_search_for_kernel(pipeline->base.device, cache,
                                                   &stages[i].cache_key,
                                                   sizeof(stages[i].cache_key),
                                                   &cache_hit);
      if (cache_hit) {
         cache_hits++;
         stages[i].feedback.flags |=
            VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT_EXT;
      }

      if (stages[i].bin != NULL) {
         anv_pipeline_add_executables(&pipeline->base, &stages[i], stages[i].bin);
         util_dynarray_append(&pipeline->shaders, struct anv_shader_bin *, stages[i].bin);

         uint32_t stack_size =
            brw_bs_prog_data_const(stages[i].bin->prog_data)->max_stack_size;
         stack_max[stages[i].stage] =
            MAX2(stack_max[stages[i].stage], stack_size);
      }

      stages[i].feedback.duration += os_time_get_nano() - stage_start;
   }

   return cache_hits == shaders;
}

static VkResult
anv_pipeline_compile_ray_tracing(struct anv_ray_tracing_pipeline *pipeline,
                                 struct anv_pipeline_cache *cache,
                                 const VkRayTracingPipelineCreateInfoKHR *info)
{
   const struct intel_device_info *devinfo = &pipeline->base.device->info;
   VkResult result;

   VkPipelineCreationFeedbackEXT pipeline_feedback = {
      .flags = VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT_EXT,
   };
   int64_t pipeline_start = os_time_get_nano();

   void *pipeline_ctx = ralloc_context(NULL);

   struct anv_pipeline_stage *stages =
      anv_pipeline_init_ray_tracing_stages(pipeline, info, pipeline_ctx);

   ANV_FROM_HANDLE(anv_pipeline_layout, layout, info->layout);

   const bool skip_cache_lookup =
      (pipeline->base.flags & VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR);

   uint32_t stack_max[MESA_VULKAN_SHADER_STAGES] = {};

   if (!skip_cache_lookup &&
       anv_pipeline_load_cached_shaders(pipeline, cache, info, stages, stack_max)) {
      pipeline_feedback.flags |=
         VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT_EXT;
      goto done;
   }

   if (info->flags & VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_EXT) {
      ralloc_free(pipeline_ctx);
      return VK_PIPELINE_COMPILE_REQUIRED_EXT;
   }

   for (uint32_t i = 0; i < info->stageCount; i++) {
      if (stages[i].entrypoint == NULL)
         continue;

      int64_t stage_start = os_time_get_nano();

      stages[i].nir = anv_pipeline_stage_get_nir(&pipeline->base, cache,
                                                 pipeline_ctx, &stages[i]);
      if (stages[i].nir == NULL) {
         ralloc_free(pipeline_ctx);
         return vk_error(pipeline, VK_ERROR_OUT_OF_HOST_MEMORY);
      }

      anv_pipeline_lower_nir(&pipeline->base, pipeline_ctx, &stages[i], layout);

      stages[i].feedback.duration += os_time_get_nano() - stage_start;
   }

   for (uint32_t i = 0; i < info->stageCount; i++) {
      if (stages[i].entrypoint == NULL)
         continue;

      /* Shader found in cache already. */
      if (stages[i].bin != NULL)
         continue;

      /* We handle intersection shaders as part of the group */
      if (stages[i].stage == MESA_SHADER_INTERSECTION)
         continue;

      int64_t stage_start = os_time_get_nano();

      void *stage_ctx = ralloc_context(pipeline_ctx);

      nir_shader *nir = nir_shader_clone(stage_ctx, stages[i].nir);
      switch (stages[i].stage) {
      case MESA_SHADER_RAYGEN:
         brw_nir_lower_raygen(nir);
         break;

      case MESA_SHADER_ANY_HIT:
         brw_nir_lower_any_hit(nir, devinfo);
         break;

      case MESA_SHADER_CLOSEST_HIT:
         brw_nir_lower_closest_hit(nir);
         break;

      case MESA_SHADER_MISS:
         brw_nir_lower_miss(nir);
         break;

      case MESA_SHADER_INTERSECTION:
         unreachable("These are handled later");

      case MESA_SHADER_CALLABLE:
         brw_nir_lower_callable(nir);
         break;

      default:
         unreachable("Invalid ray-tracing shader stage");
      }

      result = compile_upload_rt_shader(pipeline, cache, nir, &stages[i],
                                        &stages[i].bin, stage_ctx);
      if (result != VK_SUCCESS) {
         ralloc_free(pipeline_ctx);
         return result;
      }

      uint32_t stack_size =
         brw_bs_prog_data_const(stages[i].bin->prog_data)->max_stack_size;
      stack_max[stages[i].stage] = MAX2(stack_max[stages[i].stage], stack_size);

      ralloc_free(stage_ctx);

      stages[i].feedback.duration += os_time_get_nano() - stage_start;
   }

   for (uint32_t i = 0; i < info->groupCount; i++) {
      const VkRayTracingShaderGroupCreateInfoKHR *ginfo = &info->pGroups[i];
      struct anv_rt_shader_group *group = &pipeline->groups[i];
      group->type = ginfo->type;
      switch (ginfo->type) {
      case VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR:
         assert(ginfo->generalShader < info->stageCount);
         group->general = stages[ginfo->generalShader].bin;
         break;

      case VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR:
         if (ginfo->anyHitShader < info->stageCount)
            group->any_hit = stages[ginfo->anyHitShader].bin;

         if (ginfo->closestHitShader < info->stageCount)
            group->closest_hit = stages[ginfo->closestHitShader].bin;
         break;

      case VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR: {
         if (ginfo->closestHitShader < info->stageCount)
            group->closest_hit = stages[ginfo->closestHitShader].bin;

         uint32_t intersection_idx = info->pGroups[i].intersectionShader;
         assert(intersection_idx < info->stageCount);

         /* Only compile this stage if not already found in the cache. */
         if (stages[intersection_idx].bin == NULL) {
            /* The any-hit and intersection shader have to be combined */
            uint32_t any_hit_idx = info->pGroups[i].anyHitShader;
            const nir_shader *any_hit = NULL;
            if (any_hit_idx < info->stageCount)
               any_hit = stages[any_hit_idx].nir;

            void *group_ctx = ralloc_context(pipeline_ctx);
            nir_shader *intersection =
               nir_shader_clone(group_ctx, stages[intersection_idx].nir);

            brw_nir_lower_combined_intersection_any_hit(intersection, any_hit,
                                                        devinfo);

            result = compile_upload_rt_shader(pipeline, cache,
                                              intersection,
                                              &stages[intersection_idx],
                                              &group->intersection,
                                              group_ctx);
            ralloc_free(group_ctx);
            if (result != VK_SUCCESS)
               return result;
         } else {
            group->intersection = stages[intersection_idx].bin;
         }

         uint32_t stack_size =
            brw_bs_prog_data_const(group->intersection->prog_data)->max_stack_size;
         stack_max[MESA_SHADER_INTERSECTION] =
            MAX2(stack_max[MESA_SHADER_INTERSECTION], stack_size);

         break;
      }

      default:
         unreachable("Invalid ray tracing shader group type");
      }
   }

 done:
   ralloc_free(pipeline_ctx);

   anv_pipeline_compute_ray_tracing_stacks(pipeline, info, stack_max);

   pipeline_feedback.duration = os_time_get_nano() - pipeline_start;

   const VkPipelineCreationFeedbackCreateInfoEXT *create_feedback =
      vk_find_struct_const(info->pNext, PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT);
   if (create_feedback) {
      *create_feedback->pPipelineCreationFeedback = pipeline_feedback;

      assert(info->stageCount == create_feedback->pipelineStageCreationFeedbackCount);
      for (uint32_t i = 0; i < info->stageCount; i++) {
         gl_shader_stage s = vk_to_mesa_shader_stage(info->pStages[i].stage);
         create_feedback->pPipelineStageCreationFeedbacks[i] = stages[s].feedback;
      }
   }

   return VK_SUCCESS;
}

VkResult
anv_device_init_rt_shaders(struct anv_device *device)
{
   if (!device->vk.enabled_extensions.KHR_ray_tracing_pipeline)
      return VK_SUCCESS;

   bool cache_hit;

   struct brw_rt_trampoline {
      char name[16];
      struct brw_cs_prog_key key;
   } trampoline_key = {
      .name = "rt-trampoline",
      .key = {
         /* TODO: Other subgroup sizes? */
         .base.subgroup_size_type = BRW_SUBGROUP_SIZE_REQUIRE_8,
      },
   };
   device->rt_trampoline =
      anv_device_search_for_kernel(device, &device->default_pipeline_cache,
                                   &trampoline_key, sizeof(trampoline_key),
                                   &cache_hit);
   if (device->rt_trampoline == NULL) {

      void *tmp_ctx = ralloc_context(NULL);
      nir_shader *trampoline_nir =
         brw_nir_create_raygen_trampoline(device->physical->compiler, tmp_ctx);

      struct anv_pipeline_bind_map bind_map = {
         .surface_count = 0,
         .sampler_count = 0,
      };
      uint32_t dummy_params[4] = { 0, };
      struct brw_cs_prog_data trampoline_prog_data = {
         .base.nr_params = 4,
         .base.param = dummy_params,
         .uses_inline_data = true,
         .uses_btd_stack_ids = true,
      };
      struct brw_compile_cs_params params = {
         .nir = trampoline_nir,
         .key = &trampoline_key.key,
         .prog_data = &trampoline_prog_data,
         .log_data = device,
      };
      const unsigned *tramp_data =
         brw_compile_cs(device->physical->compiler, tmp_ctx, &params);

      device->rt_trampoline =
         anv_device_upload_kernel(device, &device->default_pipeline_cache,
                                  MESA_SHADER_COMPUTE,
                                  &trampoline_key, sizeof(trampoline_key),
                                  tramp_data,
                                  trampoline_prog_data.base.program_size,
                                  &trampoline_prog_data.base,
                                  sizeof(trampoline_prog_data),
                                  NULL, 0, NULL, &bind_map);

      ralloc_free(tmp_ctx);

      if (device->rt_trampoline == NULL)
         return vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
   }

   struct brw_rt_trivial_return {
      char name[16];
      struct brw_bs_prog_key key;
   } return_key = {
      .name = "rt-trivial-ret",
   };
   device->rt_trivial_return =
      anv_device_search_for_kernel(device, &device->default_pipeline_cache,
                                   &return_key, sizeof(return_key),
                                   &cache_hit);
   if (device->rt_trivial_return == NULL) {
      void *tmp_ctx = ralloc_context(NULL);
      nir_shader *trivial_return_nir =
         brw_nir_create_trivial_return_shader(device->physical->compiler, tmp_ctx);

      NIR_PASS_V(trivial_return_nir, brw_nir_lower_rt_intrinsics, &device->info);

      struct anv_pipeline_bind_map bind_map = {
         .surface_count = 0,
         .sampler_count = 0,
      };
      struct brw_bs_prog_data return_prog_data = { 0, };
      const unsigned *return_data =
         brw_compile_bs(device->physical->compiler, device, tmp_ctx,
                        &return_key.key, &return_prog_data, trivial_return_nir,
                        0, 0, NULL, NULL);

      device->rt_trivial_return =
         anv_device_upload_kernel(device, &device->default_pipeline_cache,
                                  MESA_SHADER_CALLABLE,
                                  &return_key, sizeof(return_key),
                                  return_data, return_prog_data.base.program_size,
                                  &return_prog_data.base, sizeof(return_prog_data),
                                  NULL, 0, NULL, &bind_map);

      ralloc_free(tmp_ctx);

      if (device->rt_trivial_return == NULL) {
         anv_shader_bin_unref(device, device->rt_trampoline);
         return vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
      }
   }

   return VK_SUCCESS;
}

void
anv_device_finish_rt_shaders(struct anv_device *device)
{
   if (!device->vk.enabled_extensions.KHR_ray_tracing_pipeline)
      return;

   anv_shader_bin_unref(device, device->rt_trampoline);
}

VkResult
anv_ray_tracing_pipeline_init(struct anv_ray_tracing_pipeline *pipeline,
                              struct anv_device *device,
                              struct anv_pipeline_cache *cache,
                              const VkRayTracingPipelineCreateInfoKHR *pCreateInfo,
                              const VkAllocationCallbacks *alloc)
{
   VkResult result;

   util_dynarray_init(&pipeline->shaders, pipeline->base.mem_ctx);

   result = anv_pipeline_compile_ray_tracing(pipeline, cache, pCreateInfo);
   if (result != VK_SUCCESS)
      goto fail;

   anv_pipeline_setup_l3_config(&pipeline->base, /* needs_slm */ false);

   return VK_SUCCESS;

fail:
   util_dynarray_foreach(&pipeline->shaders,
                         struct anv_shader_bin *, shader) {
      anv_shader_bin_unref(device, *shader);
   }
   return result;
}

#define WRITE_STR(field, ...) ({                               \
   memset(field, 0, sizeof(field));                            \
   UNUSED int i = snprintf(field, sizeof(field), __VA_ARGS__); \
   assert(i > 0 && i < sizeof(field));                         \
})

VkResult anv_GetPipelineExecutablePropertiesKHR(
    VkDevice                                    device,
    const VkPipelineInfoKHR*                    pPipelineInfo,
    uint32_t*                                   pExecutableCount,
    VkPipelineExecutablePropertiesKHR*          pProperties)
{
   ANV_FROM_HANDLE(anv_pipeline, pipeline, pPipelineInfo->pipeline);
   VK_OUTARRAY_MAKE(out, pProperties, pExecutableCount);

   util_dynarray_foreach (&pipeline->executables, struct anv_pipeline_executable, exe) {
      vk_outarray_append(&out, props) {
         gl_shader_stage stage = exe->stage;
         props->stages = mesa_to_vk_shader_stage(stage);

         unsigned simd_width = exe->stats.dispatch_width;
         if (stage == MESA_SHADER_FRAGMENT) {
            WRITE_STR(props->name, "%s%d %s",
                      simd_width ? "SIMD" : "vec",
                      simd_width ? simd_width : 4,
                      _mesa_shader_stage_to_string(stage));
         } else {
            WRITE_STR(props->name, "%s", _mesa_shader_stage_to_string(stage));
         }
         WRITE_STR(props->description, "%s%d %s shader",
                   simd_width ? "SIMD" : "vec",
                   simd_width ? simd_width : 4,
                   _mesa_shader_stage_to_string(stage));

         /* The compiler gives us a dispatch width of 0 for vec4 but Vulkan
          * wants a subgroup size of 1.
          */
         props->subgroupSize = MAX2(simd_width, 1);
      }
   }

   return vk_outarray_status(&out);
}

static const struct anv_pipeline_executable *
anv_pipeline_get_executable(struct anv_pipeline *pipeline, uint32_t index)
{
   assert(index < util_dynarray_num_elements(&pipeline->executables,
                                             struct anv_pipeline_executable));
   return util_dynarray_element(
      &pipeline->executables, struct anv_pipeline_executable, index);
}

VkResult anv_GetPipelineExecutableStatisticsKHR(
    VkDevice                                    device,
    const VkPipelineExecutableInfoKHR*          pExecutableInfo,
    uint32_t*                                   pStatisticCount,
    VkPipelineExecutableStatisticKHR*           pStatistics)
{
   ANV_FROM_HANDLE(anv_pipeline, pipeline, pExecutableInfo->pipeline);
   VK_OUTARRAY_MAKE(out, pStatistics, pStatisticCount);

   const struct anv_pipeline_executable *exe =
      anv_pipeline_get_executable(pipeline, pExecutableInfo->executableIndex);

   const struct brw_stage_prog_data *prog_data;
   switch (pipeline->type) {
   case ANV_PIPELINE_GRAPHICS: {
      prog_data = anv_pipeline_to_graphics(pipeline)->shaders[exe->stage]->prog_data;
      break;
   }
   case ANV_PIPELINE_COMPUTE: {
      prog_data = anv_pipeline_to_compute(pipeline)->cs->prog_data;
      break;
   }
   default:
      unreachable("invalid pipeline type");
   }

   vk_outarray_append(&out, stat) {
      WRITE_STR(stat->name, "Instruction Count");
      WRITE_STR(stat->description,
                "Number of GEN instructions in the final generated "
                "shader executable.");
      stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
      stat->value.u64 = exe->stats.instructions;
   }

   vk_outarray_append(&out, stat) {
      WRITE_STR(stat->name, "SEND Count");
      WRITE_STR(stat->description,
                "Number of instructions in the final generated shader "
                "executable which access external units such as the "
                "constant cache or the sampler.");
      stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
      stat->value.u64 = exe->stats.sends;
   }

   vk_outarray_append(&out, stat) {
      WRITE_STR(stat->name, "Loop Count");
      WRITE_STR(stat->description,
                "Number of loops (not unrolled) in the final generated "
                "shader executable.");
      stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
      stat->value.u64 = exe->stats.loops;
   }

   vk_outarray_append(&out, stat) {
      WRITE_STR(stat->name, "Cycle Count");
      WRITE_STR(stat->description,
                "Estimate of the number of EU cycles required to execute "
                "the final generated executable.  This is an estimate only "
                "and may vary greatly from actual run-time performance.");
      stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
      stat->value.u64 = exe->stats.cycles;
   }

   vk_outarray_append(&out, stat) {
      WRITE_STR(stat->name, "Spill Count");
      WRITE_STR(stat->description,
                "Number of scratch spill operations.  This gives a rough "
                "estimate of the cost incurred due to spilling temporary "
                "values to memory.  If this is non-zero, you may want to "
                "adjust your shader to reduce register pressure.");
      stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
      stat->value.u64 = exe->stats.spills;
   }

   vk_outarray_append(&out, stat) {
      WRITE_STR(stat->name, "Fill Count");
      WRITE_STR(stat->description,
                "Number of scratch fill operations.  This gives a rough "
                "estimate of the cost incurred due to spilling temporary "
                "values to memory.  If this is non-zero, you may want to "
                "adjust your shader to reduce register pressure.");
      stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
      stat->value.u64 = exe->stats.fills;
   }

   vk_outarray_append(&out, stat) {
      WRITE_STR(stat->name, "Scratch Memory Size");
      WRITE_STR(stat->description,
                "Number of bytes of scratch memory required by the "
                "generated shader executable.  If this is non-zero, you "
                "may want to adjust your shader to reduce register "
                "pressure.");
      stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
      stat->value.u64 = prog_data->total_scratch;
   }

   if (gl_shader_stage_uses_workgroup(exe->stage)) {
      vk_outarray_append(&out, stat) {
         WRITE_STR(stat->name, "Workgroup Memory Size");
         WRITE_STR(stat->description,
                   "Number of bytes of workgroup shared memory used by this "
                   "shader including any padding.");
         stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
         stat->value.u64 = prog_data->total_shared;
      }
   }

   return vk_outarray_status(&out);
}

static bool
write_ir_text(VkPipelineExecutableInternalRepresentationKHR* ir,
              const char *data)
{
   ir->isText = VK_TRUE;

   size_t data_len = strlen(data) + 1;

   if (ir->pData == NULL) {
      ir->dataSize = data_len;
      return true;
   }

   strncpy(ir->pData, data, ir->dataSize);
   if (ir->dataSize < data_len)
      return false;

   ir->dataSize = data_len;
   return true;
}

VkResult anv_GetPipelineExecutableInternalRepresentationsKHR(
    VkDevice                                    device,
    const VkPipelineExecutableInfoKHR*          pExecutableInfo,
    uint32_t*                                   pInternalRepresentationCount,
    VkPipelineExecutableInternalRepresentationKHR* pInternalRepresentations)
{
   ANV_FROM_HANDLE(anv_pipeline, pipeline, pExecutableInfo->pipeline);
   VK_OUTARRAY_MAKE(out, pInternalRepresentations,
                    pInternalRepresentationCount);
   bool incomplete_text = false;

   const struct anv_pipeline_executable *exe =
      anv_pipeline_get_executable(pipeline, pExecutableInfo->executableIndex);

   if (exe->nir) {
      vk_outarray_append(&out, ir) {
         WRITE_STR(ir->name, "Final NIR");
         WRITE_STR(ir->description,
                   "Final NIR before going into the back-end compiler");

         if (!write_ir_text(ir, exe->nir))
            incomplete_text = true;
      }
   }

   if (exe->disasm) {
      vk_outarray_append(&out, ir) {
         WRITE_STR(ir->name, "GEN Assembly");
         WRITE_STR(ir->description,
                   "Final GEN assembly for the generated shader binary");

         if (!write_ir_text(ir, exe->disasm))
            incomplete_text = true;
      }
   }

   return incomplete_text ? VK_INCOMPLETE : vk_outarray_status(&out);
}

VkResult
anv_GetRayTracingShaderGroupHandlesKHR(
    VkDevice                                    _device,
    VkPipeline                                  _pipeline,
    uint32_t                                    firstGroup,
    uint32_t                                    groupCount,
    size_t                                      dataSize,
    void*                                       pData)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_pipeline, pipeline, _pipeline);

   if (pipeline->type != ANV_PIPELINE_RAY_TRACING)
      return vk_error(device, VK_ERROR_FEATURE_NOT_PRESENT);

   struct anv_ray_tracing_pipeline *rt_pipeline =
      anv_pipeline_to_ray_tracing(pipeline);

   for (uint32_t i = 0; i < groupCount; i++) {
      struct anv_rt_shader_group *group = &rt_pipeline->groups[firstGroup + i];
      memcpy(pData, group->handle, sizeof(group->handle));
      pData += sizeof(group->handle);
   }

   return VK_SUCCESS;
}

VkResult
anv_GetRayTracingCaptureReplayShaderGroupHandlesKHR(
    VkDevice                                    _device,
    VkPipeline                                  pipeline,
    uint32_t                                    firstGroup,
    uint32_t                                    groupCount,
    size_t                                      dataSize,
    void*                                       pData)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   unreachable("Unimplemented");
   return vk_error(device, VK_ERROR_FEATURE_NOT_PRESENT);
}

VkDeviceSize
anv_GetRayTracingShaderGroupStackSizeKHR(
    VkDevice                                    device,
    VkPipeline                                  _pipeline,
    uint32_t                                    group,
    VkShaderGroupShaderKHR                      groupShader)
{
   ANV_FROM_HANDLE(anv_pipeline, pipeline, _pipeline);
   assert(pipeline->type == ANV_PIPELINE_RAY_TRACING);

   struct anv_ray_tracing_pipeline *rt_pipeline =
      anv_pipeline_to_ray_tracing(pipeline);

   assert(group < rt_pipeline->group_count);

   struct anv_shader_bin *bin;
   switch (groupShader) {
   case VK_SHADER_GROUP_SHADER_GENERAL_KHR:
      bin = rt_pipeline->groups[group].general;
      break;

   case VK_SHADER_GROUP_SHADER_CLOSEST_HIT_KHR:
      bin = rt_pipeline->groups[group].closest_hit;
      break;

   case VK_SHADER_GROUP_SHADER_ANY_HIT_KHR:
      bin = rt_pipeline->groups[group].any_hit;
      break;

   case VK_SHADER_GROUP_SHADER_INTERSECTION_KHR:
      bin = rt_pipeline->groups[group].intersection;
      break;

   default:
      unreachable("Invalid VkShaderGroupShader enum");
   }

   if (bin == NULL)
      return 0;

   return brw_bs_prog_data_const(bin->prog_data)->max_stack_size;
}
