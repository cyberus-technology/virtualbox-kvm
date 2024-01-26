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

#include "util/blob.h"
#include "util/hash_table.h"
#include "util/debug.h"
#include "util/disk_cache.h"
#include "util/mesa-sha1.h"
#include "nir/nir_serialize.h"
#include "anv_private.h"
#include "nir/nir_xfb_info.h"
#include "vulkan/util/vk_util.h"

struct anv_shader_bin *
anv_shader_bin_create(struct anv_device *device,
                      gl_shader_stage stage,
                      const void *key_data, uint32_t key_size,
                      const void *kernel_data, uint32_t kernel_size,
                      const struct brw_stage_prog_data *prog_data_in,
                      uint32_t prog_data_size,
                      const struct brw_compile_stats *stats, uint32_t num_stats,
                      const nir_xfb_info *xfb_info_in,
                      const struct anv_pipeline_bind_map *bind_map)
{
   VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct anv_shader_bin, shader, 1);
   VK_MULTIALLOC_DECL_SIZE(&ma, struct anv_shader_bin_key, key,
                                sizeof(*key) + key_size);
   VK_MULTIALLOC_DECL_SIZE(&ma, struct brw_stage_prog_data, prog_data,
                                prog_data_size);
   VK_MULTIALLOC_DECL(&ma, struct brw_shader_reloc, prog_data_relocs,
                           prog_data_in->num_relocs);
   VK_MULTIALLOC_DECL(&ma, uint32_t, prog_data_param, prog_data_in->nr_params);

   VK_MULTIALLOC_DECL_SIZE(&ma, nir_xfb_info, xfb_info,
                                xfb_info_in == NULL ? 0 :
                                nir_xfb_info_size(xfb_info_in->output_count));

   VK_MULTIALLOC_DECL(&ma, struct anv_pipeline_binding, surface_to_descriptor,
                           bind_map->surface_count);
   VK_MULTIALLOC_DECL(&ma, struct anv_pipeline_binding, sampler_to_descriptor,
                           bind_map->sampler_count);

   if (!vk_multialloc_alloc(&ma, &device->vk.alloc,
                            VK_SYSTEM_ALLOCATION_SCOPE_DEVICE))
      return NULL;

   shader->ref_cnt = 1;

   shader->stage = stage;

   key->size = key_size;
   memcpy(key->data, key_data, key_size);
   shader->key = key;

   shader->kernel =
      anv_state_pool_alloc(&device->instruction_state_pool, kernel_size, 64);
   memcpy(shader->kernel.map, kernel_data, kernel_size);
   shader->kernel_size = kernel_size;

   uint64_t shader_data_addr = INSTRUCTION_STATE_POOL_MIN_ADDRESS +
                               shader->kernel.offset +
                               prog_data_in->const_data_offset;

   int rv_count = 0;
   struct brw_shader_reloc_value reloc_values[5];
   reloc_values[rv_count++] = (struct brw_shader_reloc_value) {
      .id = BRW_SHADER_RELOC_CONST_DATA_ADDR_LOW,
      .value = shader_data_addr,
   };
   reloc_values[rv_count++] = (struct brw_shader_reloc_value) {
      .id = BRW_SHADER_RELOC_CONST_DATA_ADDR_HIGH,
      .value = shader_data_addr >> 32,
   };
   reloc_values[rv_count++] = (struct brw_shader_reloc_value) {
      .id = BRW_SHADER_RELOC_SHADER_START_OFFSET,
      .value = shader->kernel.offset,
   };
   if (brw_shader_stage_is_bindless(stage)) {
      const struct brw_bs_prog_data *bs_prog_data =
         brw_bs_prog_data_const(prog_data_in);
      uint64_t resume_sbt_addr = INSTRUCTION_STATE_POOL_MIN_ADDRESS +
                                 shader->kernel.offset +
                                 bs_prog_data->resume_sbt_offset;
      reloc_values[rv_count++] = (struct brw_shader_reloc_value) {
         .id = BRW_SHADER_RELOC_RESUME_SBT_ADDR_LOW,
         .value = resume_sbt_addr,
      };
      reloc_values[rv_count++] = (struct brw_shader_reloc_value) {
         .id = BRW_SHADER_RELOC_RESUME_SBT_ADDR_HIGH,
         .value = resume_sbt_addr >> 32,
      };
   }

   brw_write_shader_relocs(&device->info, shader->kernel.map, prog_data_in,
                           reloc_values, rv_count);

   memcpy(prog_data, prog_data_in, prog_data_size);
   typed_memcpy(prog_data_relocs, prog_data_in->relocs,
                prog_data_in->num_relocs);
   prog_data->relocs = prog_data_relocs;
   memset(prog_data_param, 0,
          prog_data->nr_params * sizeof(*prog_data_param));
   prog_data->param = prog_data_param;
   shader->prog_data = prog_data;
   shader->prog_data_size = prog_data_size;

   assert(num_stats <= ARRAY_SIZE(shader->stats));
   typed_memcpy(shader->stats, stats, num_stats);
   shader->num_stats = num_stats;

   if (xfb_info_in) {
      *xfb_info = *xfb_info_in;
      typed_memcpy(xfb_info->outputs, xfb_info_in->outputs,
                   xfb_info_in->output_count);
      shader->xfb_info = xfb_info;
   } else {
      shader->xfb_info = NULL;
   }

   shader->bind_map = *bind_map;
   typed_memcpy(surface_to_descriptor, bind_map->surface_to_descriptor,
                bind_map->surface_count);
   shader->bind_map.surface_to_descriptor = surface_to_descriptor;
   typed_memcpy(sampler_to_descriptor, bind_map->sampler_to_descriptor,
                bind_map->sampler_count);
   shader->bind_map.sampler_to_descriptor = sampler_to_descriptor;

   return shader;
}

void
anv_shader_bin_destroy(struct anv_device *device,
                       struct anv_shader_bin *shader)
{
   assert(shader->ref_cnt == 0);
   anv_state_pool_free(&device->instruction_state_pool, shader->kernel);
   vk_free(&device->vk.alloc, shader);
}

static bool
anv_shader_bin_write_to_blob(const struct anv_shader_bin *shader,
                             struct blob *blob)
{
   blob_write_uint32(blob, shader->stage);

   blob_write_uint32(blob, shader->key->size);
   blob_write_bytes(blob, shader->key->data, shader->key->size);

   blob_write_uint32(blob, shader->kernel_size);
   blob_write_bytes(blob, shader->kernel.map, shader->kernel_size);

   blob_write_uint32(blob, shader->prog_data_size);
   blob_write_bytes(blob, shader->prog_data, shader->prog_data_size);
   blob_write_bytes(blob, shader->prog_data->relocs,
                    shader->prog_data->num_relocs *
                    sizeof(shader->prog_data->relocs[0]));

   blob_write_uint32(blob, shader->num_stats);
   blob_write_bytes(blob, shader->stats,
                    shader->num_stats * sizeof(shader->stats[0]));

   if (shader->xfb_info) {
      uint32_t xfb_info_size =
         nir_xfb_info_size(shader->xfb_info->output_count);
      blob_write_uint32(blob, xfb_info_size);
      blob_write_bytes(blob, shader->xfb_info, xfb_info_size);
   } else {
      blob_write_uint32(blob, 0);
   }

   blob_write_bytes(blob, shader->bind_map.surface_sha1,
                    sizeof(shader->bind_map.surface_sha1));
   blob_write_bytes(blob, shader->bind_map.sampler_sha1,
                    sizeof(shader->bind_map.sampler_sha1));
   blob_write_bytes(blob, shader->bind_map.push_sha1,
                    sizeof(shader->bind_map.push_sha1));
   blob_write_uint32(blob, shader->bind_map.surface_count);
   blob_write_uint32(blob, shader->bind_map.sampler_count);
   blob_write_bytes(blob, shader->bind_map.surface_to_descriptor,
                    shader->bind_map.surface_count *
                    sizeof(*shader->bind_map.surface_to_descriptor));
   blob_write_bytes(blob, shader->bind_map.sampler_to_descriptor,
                    shader->bind_map.sampler_count *
                    sizeof(*shader->bind_map.sampler_to_descriptor));
   blob_write_bytes(blob, shader->bind_map.push_ranges,
                    sizeof(shader->bind_map.push_ranges));

   return !blob->out_of_memory;
}

static struct anv_shader_bin *
anv_shader_bin_create_from_blob(struct anv_device *device,
                                struct blob_reader *blob)
{
   gl_shader_stage stage = blob_read_uint32(blob);

   uint32_t key_size = blob_read_uint32(blob);
   const void *key_data = blob_read_bytes(blob, key_size);

   uint32_t kernel_size = blob_read_uint32(blob);
   const void *kernel_data = blob_read_bytes(blob, kernel_size);

   uint32_t prog_data_size = blob_read_uint32(blob);
   const void *prog_data_bytes = blob_read_bytes(blob, prog_data_size);
   if (blob->overrun)
      return NULL;

   union brw_any_prog_data prog_data;
   memcpy(&prog_data, prog_data_bytes,
          MIN2(sizeof(prog_data), prog_data_size));
   prog_data.base.relocs =
      blob_read_bytes(blob, prog_data.base.num_relocs *
                            sizeof(prog_data.base.relocs[0]));

   uint32_t num_stats = blob_read_uint32(blob);
   const struct brw_compile_stats *stats =
      blob_read_bytes(blob, num_stats * sizeof(stats[0]));

   const nir_xfb_info *xfb_info = NULL;
   uint32_t xfb_size = blob_read_uint32(blob);
   if (xfb_size)
      xfb_info = blob_read_bytes(blob, xfb_size);

   struct anv_pipeline_bind_map bind_map;
   blob_copy_bytes(blob, bind_map.surface_sha1, sizeof(bind_map.surface_sha1));
   blob_copy_bytes(blob, bind_map.sampler_sha1, sizeof(bind_map.sampler_sha1));
   blob_copy_bytes(blob, bind_map.push_sha1, sizeof(bind_map.push_sha1));
   bind_map.surface_count = blob_read_uint32(blob);
   bind_map.sampler_count = blob_read_uint32(blob);
   bind_map.surface_to_descriptor = (void *)
      blob_read_bytes(blob, bind_map.surface_count *
                            sizeof(*bind_map.surface_to_descriptor));
   bind_map.sampler_to_descriptor = (void *)
      blob_read_bytes(blob, bind_map.sampler_count *
                            sizeof(*bind_map.sampler_to_descriptor));
   blob_copy_bytes(blob, bind_map.push_ranges, sizeof(bind_map.push_ranges));

   if (blob->overrun)
      return NULL;

   return anv_shader_bin_create(device, stage,
                                key_data, key_size,
                                kernel_data, kernel_size,
                                &prog_data.base, prog_data_size,
                                stats, num_stats, xfb_info, &bind_map);
}

/* Remaining work:
 *
 * - Compact binding table layout so it's tight and not dependent on
 *   descriptor set layout.
 *
 * - Review prog_data struct for size and cacheability: struct
 *   brw_stage_prog_data has binding_table which uses a lot of uint32_t for 8
 *   bit quantities etc; use bit fields for all bools, eg dual_src_blend.
 */

static uint32_t
shader_bin_key_hash_func(const void *void_key)
{
   const struct anv_shader_bin_key *key = void_key;
   return _mesa_hash_data(key->data, key->size);
}

static bool
shader_bin_key_compare_func(const void *void_a, const void *void_b)
{
   const struct anv_shader_bin_key *a = void_a, *b = void_b;
   if (a->size != b->size)
      return false;

   return memcmp(a->data, b->data, a->size) == 0;
}

static uint32_t
sha1_hash_func(const void *sha1)
{
   return _mesa_hash_data(sha1, 20);
}

static bool
sha1_compare_func(const void *sha1_a, const void *sha1_b)
{
   return memcmp(sha1_a, sha1_b, 20) == 0;
}

void
anv_pipeline_cache_init(struct anv_pipeline_cache *cache,
                        struct anv_device *device,
                        bool cache_enabled,
                        bool external_sync)
{
   vk_object_base_init(&device->vk, &cache->base,
                       VK_OBJECT_TYPE_PIPELINE_CACHE);
   cache->device = device;
   cache->external_sync = external_sync;
   pthread_mutex_init(&cache->mutex, NULL);

   if (cache_enabled) {
      cache->cache = _mesa_hash_table_create(NULL, shader_bin_key_hash_func,
                                             shader_bin_key_compare_func);
      cache->nir_cache = _mesa_hash_table_create(NULL, sha1_hash_func,
                                                 sha1_compare_func);
   } else {
      cache->cache = NULL;
      cache->nir_cache = NULL;
   }
}

void
anv_pipeline_cache_finish(struct anv_pipeline_cache *cache)
{
   pthread_mutex_destroy(&cache->mutex);

   if (cache->cache) {
      /* This is a bit unfortunate.  In order to keep things from randomly
       * going away, the shader cache has to hold a reference to all shader
       * binaries it contains.  We unref them when we destroy the cache.
       */
      hash_table_foreach(cache->cache, entry)
         anv_shader_bin_unref(cache->device, entry->data);

      _mesa_hash_table_destroy(cache->cache, NULL);
   }

   if (cache->nir_cache) {
      hash_table_foreach(cache->nir_cache, entry)
         ralloc_free(entry->data);

      _mesa_hash_table_destroy(cache->nir_cache, NULL);
   }

   vk_object_base_finish(&cache->base);
}

static struct anv_shader_bin *
anv_pipeline_cache_search_locked(struct anv_pipeline_cache *cache,
                                 const void *key_data, uint32_t key_size)
{
   uint32_t vla[1 + DIV_ROUND_UP(key_size, sizeof(uint32_t))];
   struct anv_shader_bin_key *key = (void *)vla;
   key->size = key_size;
   memcpy(key->data, key_data, key_size);

   struct hash_entry *entry = _mesa_hash_table_search(cache->cache, key);
   if (entry)
      return entry->data;
   else
      return NULL;
}

static inline void
anv_cache_lock(struct anv_pipeline_cache *cache)
{
   if (!cache->external_sync)
      pthread_mutex_lock(&cache->mutex);
}

static inline void
anv_cache_unlock(struct anv_pipeline_cache *cache)
{
   if (!cache->external_sync)
      pthread_mutex_unlock(&cache->mutex);
}

struct anv_shader_bin *
anv_pipeline_cache_search(struct anv_pipeline_cache *cache,
                          const void *key_data, uint32_t key_size)
{
   if (!cache->cache)
      return NULL;

   anv_cache_lock(cache);

   struct anv_shader_bin *shader =
      anv_pipeline_cache_search_locked(cache, key_data, key_size);

   anv_cache_unlock(cache);

   /* We increment refcount before handing it to the caller */
   if (shader)
      anv_shader_bin_ref(shader);

   return shader;
}

static void
anv_pipeline_cache_add_shader_bin(struct anv_pipeline_cache *cache,
                                  struct anv_shader_bin *bin)
{
   if (!cache->cache)
      return;

   anv_cache_lock(cache);

   struct hash_entry *entry = _mesa_hash_table_search(cache->cache, bin->key);
   if (entry == NULL) {
      /* Take a reference for the cache */
      anv_shader_bin_ref(bin);
      _mesa_hash_table_insert(cache->cache, bin->key, bin);
   }

   anv_cache_unlock(cache);
}

static struct anv_shader_bin *
anv_pipeline_cache_add_shader_locked(struct anv_pipeline_cache *cache,
                                     gl_shader_stage stage,
                                     const void *key_data, uint32_t key_size,
                                     const void *kernel_data,
                                     uint32_t kernel_size,
                                     const struct brw_stage_prog_data *prog_data,
                                     uint32_t prog_data_size,
                                     const struct brw_compile_stats *stats,
                                     uint32_t num_stats,
                                     const nir_xfb_info *xfb_info,
                                     const struct anv_pipeline_bind_map *bind_map)
{
   struct anv_shader_bin *shader =
      anv_pipeline_cache_search_locked(cache, key_data, key_size);
   if (shader)
      return shader;

   struct anv_shader_bin *bin =
      anv_shader_bin_create(cache->device, stage,
                            key_data, key_size,
                            kernel_data, kernel_size,
                            prog_data, prog_data_size,
                            stats, num_stats, xfb_info, bind_map);
   if (!bin)
      return NULL;

   _mesa_hash_table_insert(cache->cache, bin->key, bin);

   return bin;
}

struct anv_shader_bin *
anv_pipeline_cache_upload_kernel(struct anv_pipeline_cache *cache,
                                 gl_shader_stage stage,
                                 const void *key_data, uint32_t key_size,
                                 const void *kernel_data, uint32_t kernel_size,
                                 const struct brw_stage_prog_data *prog_data,
                                 uint32_t prog_data_size,
                                 const struct brw_compile_stats *stats,
                                 uint32_t num_stats,
                                 const nir_xfb_info *xfb_info,
                                 const struct anv_pipeline_bind_map *bind_map)
{
   if (cache->cache) {
      anv_cache_lock(cache);

      struct anv_shader_bin *bin =
         anv_pipeline_cache_add_shader_locked(cache, stage, key_data, key_size,
                                              kernel_data, kernel_size,
                                              prog_data, prog_data_size,
                                              stats, num_stats,
                                              xfb_info, bind_map);

      anv_cache_unlock(cache);

      /* We increment refcount before handing it to the caller */
      if (bin)
         anv_shader_bin_ref(bin);

      return bin;
   } else {
      /* In this case, we're not caching it so the caller owns it entirely */
      return anv_shader_bin_create(cache->device, stage,
                                   key_data, key_size,
                                   kernel_data, kernel_size,
                                   prog_data, prog_data_size,
                                   stats, num_stats,
                                   xfb_info, bind_map);
   }
}

static void
anv_pipeline_cache_load(struct anv_pipeline_cache *cache,
                        const void *data, size_t size)
{
   struct anv_device *device = cache->device;
   struct anv_physical_device *pdevice = device->physical;

   if (cache->cache == NULL)
      return;

   struct blob_reader blob;
   blob_reader_init(&blob, data, size);

   struct vk_pipeline_cache_header header;
   blob_copy_bytes(&blob, &header, sizeof(header));
   uint32_t count = blob_read_uint32(&blob);
   if (blob.overrun)
      return;

   if (header.header_size < sizeof(header))
      return;
   if (header.header_version != VK_PIPELINE_CACHE_HEADER_VERSION_ONE)
      return;
   if (header.vendor_id != 0x8086)
      return;
   if (header.device_id != device->info.chipset_id)
      return;
   if (memcmp(header.uuid, pdevice->pipeline_cache_uuid, VK_UUID_SIZE) != 0)
      return;

   for (uint32_t i = 0; i < count; i++) {
      struct anv_shader_bin *bin =
         anv_shader_bin_create_from_blob(device, &blob);
      if (!bin)
         break;
      _mesa_hash_table_insert(cache->cache, bin->key, bin);
   }
}

VkResult anv_CreatePipelineCache(
    VkDevice                                    _device,
    const VkPipelineCacheCreateInfo*            pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipelineCache*                            pPipelineCache)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   struct anv_pipeline_cache *cache;

   assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO);

   cache = vk_alloc2(&device->vk.alloc, pAllocator,
                       sizeof(*cache), 8,
                       VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (cache == NULL)
      return vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);

   anv_pipeline_cache_init(cache, device,
                           device->physical->instance->pipeline_cache_enabled,
                           pCreateInfo->flags & VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT_EXT);

   if (pCreateInfo->initialDataSize > 0)
      anv_pipeline_cache_load(cache,
                              pCreateInfo->pInitialData,
                              pCreateInfo->initialDataSize);

   *pPipelineCache = anv_pipeline_cache_to_handle(cache);

   return VK_SUCCESS;
}

void anv_DestroyPipelineCache(
    VkDevice                                    _device,
    VkPipelineCache                             _cache,
    const VkAllocationCallbacks*                pAllocator)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_pipeline_cache, cache, _cache);

   if (!cache)
      return;

   anv_pipeline_cache_finish(cache);

   vk_free2(&device->vk.alloc, pAllocator, cache);
}

VkResult anv_GetPipelineCacheData(
    VkDevice                                    _device,
    VkPipelineCache                             _cache,
    size_t*                                     pDataSize,
    void*                                       pData)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_pipeline_cache, cache, _cache);

   struct blob blob;
   if (pData) {
      blob_init_fixed(&blob, pData, *pDataSize);
   } else {
      blob_init_fixed(&blob, NULL, SIZE_MAX);
   }

   struct vk_pipeline_cache_header header = {
      .header_size = sizeof(struct vk_pipeline_cache_header),
      .header_version = VK_PIPELINE_CACHE_HEADER_VERSION_ONE,
      .vendor_id = 0x8086,
      .device_id = device->info.chipset_id,
   };
   memcpy(header.uuid, device->physical->pipeline_cache_uuid, VK_UUID_SIZE);
   blob_write_bytes(&blob, &header, sizeof(header));

   uint32_t count = 0;
   intptr_t count_offset = blob_reserve_uint32(&blob);
   if (count_offset < 0) {
      *pDataSize = 0;
      blob_finish(&blob);
      return VK_INCOMPLETE;
   }

   VkResult result = VK_SUCCESS;
   if (cache->cache) {
      hash_table_foreach(cache->cache, entry) {
         struct anv_shader_bin *shader = entry->data;

         size_t save_size = blob.size;
         if (!anv_shader_bin_write_to_blob(shader, &blob)) {
            /* If it fails reset to the previous size and bail */
            blob.size = save_size;
            result = VK_INCOMPLETE;
            break;
         }

         count++;
      }
   }

   blob_overwrite_uint32(&blob, count_offset, count);

   *pDataSize = blob.size;

   blob_finish(&blob);

   return result;
}

VkResult anv_MergePipelineCaches(
    VkDevice                                    _device,
    VkPipelineCache                             destCache,
    uint32_t                                    srcCacheCount,
    const VkPipelineCache*                      pSrcCaches)
{
   ANV_FROM_HANDLE(anv_pipeline_cache, dst, destCache);

   if (!dst->cache)
      return VK_SUCCESS;

   for (uint32_t i = 0; i < srcCacheCount; i++) {
      ANV_FROM_HANDLE(anv_pipeline_cache, src, pSrcCaches[i]);
      if (!src->cache)
         continue;

      hash_table_foreach(src->cache, entry) {
         struct anv_shader_bin *bin = entry->data;
         assert(bin);

         if (_mesa_hash_table_search(dst->cache, bin->key))
            continue;

         anv_shader_bin_ref(bin);
         _mesa_hash_table_insert(dst->cache, bin->key, bin);
      }
   }

   return VK_SUCCESS;
}

struct anv_shader_bin *
anv_device_search_for_kernel(struct anv_device *device,
                             struct anv_pipeline_cache *cache,
                             const void *key_data, uint32_t key_size,
                             bool *user_cache_hit)
{
   struct anv_shader_bin *bin;

   *user_cache_hit = false;

   if (cache) {
      bin = anv_pipeline_cache_search(cache, key_data, key_size);
      if (bin) {
         *user_cache_hit = cache != &device->default_pipeline_cache;
         return bin;
      }
   }

#ifdef ENABLE_SHADER_CACHE
   struct disk_cache *disk_cache = device->physical->disk_cache;
   if (disk_cache && device->physical->instance->pipeline_cache_enabled) {
      cache_key cache_key;
      disk_cache_compute_key(disk_cache, key_data, key_size, cache_key);

      size_t buffer_size;
      uint8_t *buffer = disk_cache_get(disk_cache, cache_key, &buffer_size);
      if (buffer) {
         struct blob_reader blob;
         blob_reader_init(&blob, buffer, buffer_size);
         bin = anv_shader_bin_create_from_blob(device, &blob);
         free(buffer);

         if (bin) {
            if (cache)
               anv_pipeline_cache_add_shader_bin(cache, bin);
            return bin;
         }
      }
   }
#endif

   return NULL;
}

struct anv_shader_bin *
anv_device_upload_kernel(struct anv_device *device,
                         struct anv_pipeline_cache *cache,
                         gl_shader_stage stage,
                         const void *key_data, uint32_t key_size,
                         const void *kernel_data, uint32_t kernel_size,
                         const struct brw_stage_prog_data *prog_data,
                         uint32_t prog_data_size,
                         const struct brw_compile_stats *stats,
                         uint32_t num_stats,
                         const nir_xfb_info *xfb_info,
                         const struct anv_pipeline_bind_map *bind_map)
{
   struct anv_shader_bin *bin;
   if (cache) {
      bin = anv_pipeline_cache_upload_kernel(cache, stage, key_data, key_size,
                                             kernel_data, kernel_size,
                                             prog_data, prog_data_size,
                                             stats, num_stats,
                                             xfb_info, bind_map);
   } else {
      bin = anv_shader_bin_create(device, stage, key_data, key_size,
                                  kernel_data, kernel_size,
                                  prog_data, prog_data_size,
                                  stats, num_stats,
                                  xfb_info, bind_map);
   }

   if (bin == NULL)
      return NULL;

#ifdef ENABLE_SHADER_CACHE
   struct disk_cache *disk_cache = device->physical->disk_cache;
   if (disk_cache) {
      struct blob binary;
      blob_init(&binary);
      if (anv_shader_bin_write_to_blob(bin, &binary)) {
         cache_key cache_key;
         disk_cache_compute_key(disk_cache, key_data, key_size, cache_key);

         disk_cache_put(disk_cache, cache_key, binary.data, binary.size, NULL);
      }

      blob_finish(&binary);
   }
#endif

   return bin;
}

struct serialized_nir {
   unsigned char sha1_key[20];
   size_t size;
   char data[0];
};

struct nir_shader *
anv_device_search_for_nir(struct anv_device *device,
                          struct anv_pipeline_cache *cache,
                          const nir_shader_compiler_options *nir_options,
                          unsigned char sha1_key[20],
                          void *mem_ctx)
{
   if (cache && cache->nir_cache) {
      const struct serialized_nir *snir = NULL;

      anv_cache_lock(cache);
      struct hash_entry *entry =
         _mesa_hash_table_search(cache->nir_cache, sha1_key);
      if (entry)
         snir = entry->data;
      anv_cache_unlock(cache);

      if (snir) {
         struct blob_reader blob;
         blob_reader_init(&blob, snir->data, snir->size);

         nir_shader *nir = nir_deserialize(mem_ctx, nir_options, &blob);
         if (blob.overrun) {
            ralloc_free(nir);
         } else {
            return nir;
         }
      }
   }

   return NULL;
}

void
anv_device_upload_nir(struct anv_device *device,
                      struct anv_pipeline_cache *cache,
                      const struct nir_shader *nir,
                      unsigned char sha1_key[20])
{
   if (cache && cache->nir_cache) {
      anv_cache_lock(cache);
      struct hash_entry *entry =
         _mesa_hash_table_search(cache->nir_cache, sha1_key);
      anv_cache_unlock(cache);
      if (entry)
         return;

      struct blob blob;
      blob_init(&blob);

      nir_serialize(&blob, nir, false);
      if (blob.out_of_memory) {
         blob_finish(&blob);
         return;
      }

      anv_cache_lock(cache);
      /* Because ralloc isn't thread-safe, we have to do all this inside the
       * lock.  We could unlock for the big memcpy but it's probably not worth
       * the hassle.
       */
      entry = _mesa_hash_table_search(cache->nir_cache, sha1_key);
      if (entry) {
         blob_finish(&blob);
         anv_cache_unlock(cache);
         return;
      }

      struct serialized_nir *snir =
         ralloc_size(cache->nir_cache, sizeof(*snir) + blob.size);
      memcpy(snir->sha1_key, sha1_key, 20);
      snir->size = blob.size;
      memcpy(snir->data, blob.data, blob.size);

      blob_finish(&blob);

      _mesa_hash_table_insert(cache->nir_cache, snir->sha1_key, snir);

      anv_cache_unlock(cache);
   }
}
