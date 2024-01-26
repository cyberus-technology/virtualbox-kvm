/*
 * Copyright © 2020 Intel Corporation
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

#ifndef BRW_NIR_RT_BUILDER_H
#define BRW_NIR_RT_BUILDER_H

#include "brw_rt.h"
#include "nir_builder.h"

/* We have our own load/store scratch helpers because they emit a global
 * memory read or write based on the scratch_base_ptr system value rather
 * than a load/store_scratch intrinsic.
 */
static inline nir_ssa_def *
brw_nir_rt_load_scratch(nir_builder *b, uint32_t offset, unsigned align,
                        unsigned num_components, unsigned bit_size)
{
   nir_ssa_def *addr =
      nir_iadd_imm(b, nir_load_scratch_base_ptr(b, 1, 64, 1), offset);
   return nir_load_global(b, addr, MIN2(align, BRW_BTD_STACK_ALIGN),
                          num_components, bit_size);
}

static inline void
brw_nir_rt_store_scratch(nir_builder *b, uint32_t offset, unsigned align,
                         nir_ssa_def *value, nir_component_mask_t write_mask)
{
   nir_ssa_def *addr =
      nir_iadd_imm(b, nir_load_scratch_base_ptr(b, 1, 64, 1), offset);
   nir_store_global(b, addr, MIN2(align, BRW_BTD_STACK_ALIGN),
                    value, write_mask);
}

static inline void
brw_nir_btd_spawn(nir_builder *b, nir_ssa_def *record_addr)
{
   nir_btd_spawn_intel(b, nir_load_btd_global_arg_addr_intel(b), record_addr);
}

static inline void
brw_nir_btd_retire(nir_builder *b)
{
   nir_btd_retire_intel(b);
}

/** This is a pseudo-op which does a bindless return
 *
 * It loads the return address from the stack and calls btd_spawn to spawn the
 * resume shader.
 */
static inline void
brw_nir_btd_return(struct nir_builder *b)
{
   assert(b->shader->scratch_size == BRW_BTD_STACK_CALLEE_DATA_SIZE);
   nir_ssa_def *resume_addr =
      brw_nir_rt_load_scratch(b, BRW_BTD_STACK_RESUME_BSR_ADDR_OFFSET,
                              8 /* align */, 1, 64);
   brw_nir_btd_spawn(b, resume_addr);
}

static inline void
assert_def_size(nir_ssa_def *def, unsigned num_components, unsigned bit_size)
{
   assert(def->num_components == num_components);
   assert(def->bit_size == bit_size);
}

static inline nir_ssa_def *
brw_nir_num_rt_stacks(nir_builder *b,
                      const struct intel_device_info *devinfo)
{
   return nir_imul_imm(b, nir_load_ray_num_dss_rt_stacks_intel(b),
                          intel_device_info_num_dual_subslices(devinfo));
}

static inline nir_ssa_def *
brw_nir_rt_stack_id(nir_builder *b)
{
   return nir_iadd(b, nir_umul_32x16(b, nir_load_ray_num_dss_rt_stacks_intel(b),
                                     nir_load_btd_dss_id_intel(b)),
                      nir_load_btd_stack_id_intel(b));
}

static inline nir_ssa_def *
brw_nir_rt_sw_hotzone_addr(nir_builder *b,
                           const struct intel_device_info *devinfo)
{
   nir_ssa_def *offset32 =
      nir_imul_imm(b, brw_nir_rt_stack_id(b), BRW_RT_SIZEOF_HOTZONE);

   offset32 = nir_iadd(b, offset32, nir_ineg(b,
      nir_imul_imm(b, brw_nir_num_rt_stacks(b, devinfo),
                      BRW_RT_SIZEOF_HOTZONE)));

   return nir_iadd(b, nir_load_ray_base_mem_addr_intel(b),
                      nir_i2i64(b, offset32));
}

static inline nir_ssa_def *
brw_nir_rt_ray_addr(nir_builder *b)
{
   /* From the BSpec "Address Computation for Memory Based Data Structures:
    * Ray and TraversalStack (Async Ray Tracing)":
    *
    *    stackBase = RTDispatchGlobals.rtMemBasePtr
    *              + (DSSID * RTDispatchGlobals.numDSSRTStacks + stackID)
    *              * RTDispatchGlobals.stackSizePerRay // 64B aligned
    *
    * We assume that we can calculate a 32-bit offset first and then add it
    * to the 64-bit base address at the end.
    */
   nir_ssa_def *offset32 =
      nir_imul(b, brw_nir_rt_stack_id(b),
                  nir_load_ray_hw_stack_size_intel(b));
   return nir_iadd(b, nir_load_ray_base_mem_addr_intel(b),
                      nir_u2u64(b, offset32));
}

static inline nir_ssa_def *
brw_nir_rt_mem_hit_addr(nir_builder *b, bool committed)
{
   return nir_iadd_imm(b, brw_nir_rt_ray_addr(b),
                          committed ? 0 : BRW_RT_SIZEOF_HIT_INFO);
}

static inline nir_ssa_def *
brw_nir_rt_hit_attrib_data_addr(nir_builder *b)
{
   return nir_iadd_imm(b, brw_nir_rt_ray_addr(b),
                          BRW_RT_OFFSETOF_HIT_ATTRIB_DATA);
}

static inline nir_ssa_def *
brw_nir_rt_mem_ray_addr(nir_builder *b,
                        enum brw_rt_bvh_level bvh_level)
{
   /* From the BSpec "Address Computation for Memory Based Data Structures:
    * Ray and TraversalStack (Async Ray Tracing)":
    *
    *    rayBase = stackBase + sizeof(HitInfo) * 2 // 64B aligned
    *    rayPtr  = rayBase + bvhLevel * sizeof(Ray); // 64B aligned
    *
    * In Vulkan, we always have exactly two levels of BVH: World and Object.
    */
   uint32_t offset = BRW_RT_SIZEOF_HIT_INFO * 2 +
                     bvh_level * BRW_RT_SIZEOF_RAY;
   return nir_iadd_imm(b, brw_nir_rt_ray_addr(b), offset);
}

static inline nir_ssa_def *
brw_nir_rt_sw_stack_addr(nir_builder *b,
                         const struct intel_device_info *devinfo)
{
   nir_ssa_def *addr = nir_load_ray_base_mem_addr_intel(b);

   nir_ssa_def *offset32 = nir_imul(b, brw_nir_num_rt_stacks(b, devinfo),
                                       nir_load_ray_hw_stack_size_intel(b));
   addr = nir_iadd(b, addr, nir_u2u64(b, offset32));

   return nir_iadd(b, addr,
      nir_imul(b, nir_u2u64(b, brw_nir_rt_stack_id(b)),
                  nir_u2u64(b, nir_load_ray_sw_stack_size_intel(b))));
}

static inline nir_ssa_def *
nir_unpack_64_4x16_split_z(nir_builder *b, nir_ssa_def *val)
{
   return nir_unpack_32_2x16_split_x(b, nir_unpack_64_2x32_split_y(b, val));
}

struct brw_nir_rt_globals_defs {
   nir_ssa_def *base_mem_addr;
   nir_ssa_def *call_stack_handler_addr;
   nir_ssa_def *hw_stack_size;
   nir_ssa_def *num_dss_rt_stacks;
   nir_ssa_def *hit_sbt_addr;
   nir_ssa_def *hit_sbt_stride;
   nir_ssa_def *miss_sbt_addr;
   nir_ssa_def *miss_sbt_stride;
   nir_ssa_def *sw_stack_size;
   nir_ssa_def *launch_size;
   nir_ssa_def *call_sbt_addr;
   nir_ssa_def *call_sbt_stride;
   nir_ssa_def *resume_sbt_addr;
};

static inline void
brw_nir_rt_load_globals(nir_builder *b,
                        struct brw_nir_rt_globals_defs *defs)
{
   nir_ssa_def *addr = nir_load_btd_global_arg_addr_intel(b);

   nir_ssa_def *data;
   data = nir_load_global_const_block_intel(b, 16, addr, nir_imm_true(b));
   defs->base_mem_addr = nir_pack_64_2x32(b, nir_channels(b, data, 0x3));

   defs->call_stack_handler_addr =
      nir_pack_64_2x32(b, nir_channels(b, data, 0x3 << 2));

   defs->hw_stack_size = nir_channel(b, data, 4);
   defs->num_dss_rt_stacks = nir_iand_imm(b, nir_channel(b, data, 5), 0xffff);
   defs->hit_sbt_addr =
      nir_pack_64_2x32_split(b, nir_channel(b, data, 8),
                                nir_extract_i16(b, nir_channel(b, data, 9),
                                                   nir_imm_int(b, 0)));
   defs->hit_sbt_stride =
      nir_unpack_32_2x16_split_y(b, nir_channel(b, data, 9));
   defs->miss_sbt_addr =
      nir_pack_64_2x32_split(b, nir_channel(b, data, 10),
                                nir_extract_i16(b, nir_channel(b, data, 11),
                                                   nir_imm_int(b, 0)));
   defs->miss_sbt_stride =
      nir_unpack_32_2x16_split_y(b, nir_channel(b, data, 11));
   defs->sw_stack_size = nir_channel(b, data, 12);
   defs->launch_size = nir_channels(b, data, 0x7u << 13);

   data = nir_load_global_const_block_intel(b, 8, nir_iadd_imm(b, addr, 64),
                                                  nir_imm_true(b));
   defs->call_sbt_addr =
      nir_pack_64_2x32_split(b, nir_channel(b, data, 0),
                                nir_extract_i16(b, nir_channel(b, data, 1),
                                                   nir_imm_int(b, 0)));
   defs->call_sbt_stride =
      nir_unpack_32_2x16_split_y(b, nir_channel(b, data, 1));

   defs->resume_sbt_addr =
      nir_pack_64_2x32(b, nir_channels(b, data, 0x3 << 2));
}

static inline nir_ssa_def *
brw_nir_rt_unpack_leaf_ptr(nir_builder *b, nir_ssa_def *vec2)
{
   /* Hit record leaf pointers are 42-bit and assumed to be in 64B chunks.
    * This leaves 22 bits at the top for other stuff.
    */
   nir_ssa_def *ptr64 = nir_imul_imm(b, nir_pack_64_2x32(b, vec2), 64);

   /* The top 16 bits (remember, we shifted by 6 already) contain garbage
    * that we need to get rid of.
    */
   nir_ssa_def *ptr_lo = nir_unpack_64_2x32_split_x(b, ptr64);
   nir_ssa_def *ptr_hi = nir_unpack_64_2x32_split_y(b, ptr64);
   ptr_hi = nir_extract_i16(b, ptr_hi, nir_imm_int(b, 0));
   return nir_pack_64_2x32_split(b, ptr_lo, ptr_hi);
}

struct brw_nir_rt_mem_hit_defs {
   nir_ssa_def *t;
   nir_ssa_def *tri_bary; /**< Only valid for triangle geometry */
   nir_ssa_def *aabb_hit_kind; /**< Only valid for AABB geometry */
   nir_ssa_def *leaf_type;
   nir_ssa_def *prim_leaf_index;
   nir_ssa_def *front_face;
   nir_ssa_def *prim_leaf_ptr;
   nir_ssa_def *inst_leaf_ptr;
};

static inline void
brw_nir_rt_load_mem_hit(nir_builder *b,
                        struct brw_nir_rt_mem_hit_defs *defs,
                        bool committed)
{
   nir_ssa_def *hit_addr = brw_nir_rt_mem_hit_addr(b, committed);

   nir_ssa_def *data = nir_load_global(b, hit_addr, 16, 4, 32);
   defs->t = nir_channel(b, data, 0);
   defs->aabb_hit_kind = nir_channel(b, data, 1);
   defs->tri_bary = nir_channels(b, data, 0x6);
   nir_ssa_def *bitfield = nir_channel(b, data, 3);
   defs->leaf_type =
      nir_ubitfield_extract(b, bitfield, nir_imm_int(b, 17), nir_imm_int(b, 3));
   defs->prim_leaf_index =
      nir_ubitfield_extract(b, bitfield, nir_imm_int(b, 20), nir_imm_int(b, 4));
   defs->front_face = nir_i2b(b, nir_iand_imm(b, bitfield, 1 << 27));

   data = nir_load_global(b, nir_iadd_imm(b, hit_addr, 16), 16, 4, 32);
   defs->prim_leaf_ptr =
      brw_nir_rt_unpack_leaf_ptr(b, nir_channels(b, data, 0x3 << 0));
   defs->inst_leaf_ptr =
      brw_nir_rt_unpack_leaf_ptr(b, nir_channels(b, data, 0x3 << 2));
}

static inline void
brw_nir_memcpy_global(nir_builder *b,
                      nir_ssa_def *dst_addr, uint32_t dst_align,
                      nir_ssa_def *src_addr, uint32_t src_align,
                      uint32_t size)
{
   /* We're going to copy in 16B chunks */
   assert(size % 16 == 0);
   dst_align = MIN2(dst_align, 16);
   src_align = MIN2(src_align, 16);

   for (unsigned offset = 0; offset < size; offset += 16) {
      nir_ssa_def *data =
         nir_load_global(b, nir_iadd_imm(b, src_addr, offset), src_align,
                         4, 32);
      nir_store_global(b, nir_iadd_imm(b, dst_addr, offset), dst_align,
                       data, 0xf /* write_mask */);
   }
}

static inline void
brw_nir_rt_commit_hit(nir_builder *b)
{
   brw_nir_memcpy_global(b, brw_nir_rt_mem_hit_addr(b, true), 16,
                            brw_nir_rt_mem_hit_addr(b, false), 16,
                            BRW_RT_SIZEOF_HIT_INFO);
}

struct brw_nir_rt_mem_ray_defs {
   nir_ssa_def *orig;
   nir_ssa_def *dir;
   nir_ssa_def *t_near;
   nir_ssa_def *t_far;
   nir_ssa_def *root_node_ptr;
   nir_ssa_def *ray_flags;
   nir_ssa_def *hit_group_sr_base_ptr;
   nir_ssa_def *hit_group_sr_stride;
   nir_ssa_def *miss_sr_ptr;
   nir_ssa_def *shader_index_multiplier;
   nir_ssa_def *inst_leaf_ptr;
   nir_ssa_def *ray_mask;
};

static inline void
brw_nir_rt_store_mem_ray(nir_builder *b,
                         const struct brw_nir_rt_mem_ray_defs *defs,
                         enum brw_rt_bvh_level bvh_level)
{
   nir_ssa_def *ray_addr = brw_nir_rt_mem_ray_addr(b, bvh_level);

   assert_def_size(defs->orig, 3, 32);
   assert_def_size(defs->dir, 3, 32);
   nir_store_global(b, nir_iadd_imm(b, ray_addr, 0), 16,
      nir_vec4(b, nir_channel(b, defs->orig, 0),
                  nir_channel(b, defs->orig, 1),
                  nir_channel(b, defs->orig, 2),
                  nir_channel(b, defs->dir, 0)),
      ~0 /* write mask */);

   assert_def_size(defs->t_near, 1, 32);
   assert_def_size(defs->t_far, 1, 32);
   nir_store_global(b, nir_iadd_imm(b, ray_addr, 16), 16,
      nir_vec4(b, nir_channel(b, defs->dir, 1),
                  nir_channel(b, defs->dir, 2),
                  defs->t_near,
                  defs->t_far),
      ~0 /* write mask */);

   assert_def_size(defs->root_node_ptr, 1, 64);
   assert_def_size(defs->ray_flags, 1, 16);
   assert_def_size(defs->hit_group_sr_base_ptr, 1, 64);
   assert_def_size(defs->hit_group_sr_stride, 1, 16);
   nir_store_global(b, nir_iadd_imm(b, ray_addr, 32), 16,
      nir_vec4(b, nir_unpack_64_2x32_split_x(b, defs->root_node_ptr),
                  nir_pack_32_2x16_split(b,
                     nir_unpack_64_4x16_split_z(b, defs->root_node_ptr),
                     defs->ray_flags),
                  nir_unpack_64_2x32_split_x(b, defs->hit_group_sr_base_ptr),
                  nir_pack_32_2x16_split(b,
                     nir_unpack_64_4x16_split_z(b, defs->hit_group_sr_base_ptr),
                     defs->hit_group_sr_stride)),
      ~0 /* write mask */);

   /* leaf_ptr is optional */
   nir_ssa_def *inst_leaf_ptr;
   if (defs->inst_leaf_ptr) {
      inst_leaf_ptr = defs->inst_leaf_ptr;
   } else {
      inst_leaf_ptr = nir_imm_int64(b, 0);
   }

   assert_def_size(defs->miss_sr_ptr, 1, 64);
   assert_def_size(defs->shader_index_multiplier, 1, 32);
   assert_def_size(inst_leaf_ptr, 1, 64);
   assert_def_size(defs->ray_mask, 1, 32);
   nir_store_global(b, nir_iadd_imm(b, ray_addr, 48), 16,
      nir_vec4(b, nir_unpack_64_2x32_split_x(b, defs->miss_sr_ptr),
                  nir_pack_32_2x16_split(b,
                     nir_unpack_64_4x16_split_z(b, defs->miss_sr_ptr),
                     nir_unpack_32_2x16_split_x(b,
                        nir_ishl(b, defs->shader_index_multiplier,
                                    nir_imm_int(b, 8)))),
                  nir_unpack_64_2x32_split_x(b, inst_leaf_ptr),
                  nir_pack_32_2x16_split(b,
                     nir_unpack_64_4x16_split_z(b, inst_leaf_ptr),
                     nir_unpack_32_2x16_split_x(b, defs->ray_mask))),
      ~0 /* write mask */);
}

static inline void
brw_nir_rt_load_mem_ray(nir_builder *b,
                        struct brw_nir_rt_mem_ray_defs *defs,
                        enum brw_rt_bvh_level bvh_level)
{
   nir_ssa_def *ray_addr = brw_nir_rt_mem_ray_addr(b, bvh_level);

   nir_ssa_def *data[4] = {
      nir_load_global(b, nir_iadd_imm(b, ray_addr,  0), 16, 4, 32),
      nir_load_global(b, nir_iadd_imm(b, ray_addr, 16), 16, 4, 32),
      nir_load_global(b, nir_iadd_imm(b, ray_addr, 32), 16, 4, 32),
      nir_load_global(b, nir_iadd_imm(b, ray_addr, 48), 16, 4, 32),
   };

   defs->orig = nir_channels(b, data[0], 0x7);
   defs->dir = nir_vec3(b, nir_channel(b, data[0], 3),
                           nir_channel(b, data[1], 0),
                           nir_channel(b, data[1], 1));
   defs->t_near = nir_channel(b, data[1], 2);
   defs->t_far = nir_channel(b, data[1], 3);
   defs->root_node_ptr =
      nir_pack_64_2x32_split(b, nir_channel(b, data[2], 0),
                                nir_extract_i16(b, nir_channel(b, data[2], 1),
                                                   nir_imm_int(b, 0)));
   defs->ray_flags =
      nir_unpack_32_2x16_split_y(b, nir_channel(b, data[2], 1));
   defs->hit_group_sr_base_ptr =
      nir_pack_64_2x32_split(b, nir_channel(b, data[2], 2),
                                nir_extract_i16(b, nir_channel(b, data[2], 3),
                                                   nir_imm_int(b, 0)));
   defs->hit_group_sr_stride =
      nir_unpack_32_2x16_split_y(b, nir_channel(b, data[2], 3));
   defs->miss_sr_ptr =
      nir_pack_64_2x32_split(b, nir_channel(b, data[3], 0),
                                nir_extract_i16(b, nir_channel(b, data[3], 1),
                                                   nir_imm_int(b, 0)));
   defs->shader_index_multiplier =
      nir_ushr(b, nir_unpack_32_2x16_split_y(b, nir_channel(b, data[3], 1)),
                  nir_imm_int(b, 8));
   defs->inst_leaf_ptr =
      nir_pack_64_2x32_split(b, nir_channel(b, data[3], 2),
                                nir_extract_i16(b, nir_channel(b, data[3], 3),
                                                   nir_imm_int(b, 0)));
   defs->ray_mask =
      nir_unpack_32_2x16_split_y(b, nir_channel(b, data[3], 3));
}

struct brw_nir_rt_bvh_instance_leaf_defs {
   nir_ssa_def *world_to_object[4];
   nir_ssa_def *instance_id;
   nir_ssa_def *instance_index;
   nir_ssa_def *object_to_world[4];
};

static inline void
brw_nir_rt_load_bvh_instance_leaf(nir_builder *b,
                                  struct brw_nir_rt_bvh_instance_leaf_defs *defs,
                                  nir_ssa_def *leaf_addr)
{
   /* We don't care about the first 16B of the leaf for now.  One day, we may
    * add code to decode it but none of that data is directly required for
    * implementing any ray-tracing built-ins.
    */

   defs->world_to_object[0] =
      nir_load_global(b, nir_iadd_imm(b, leaf_addr, 16), 4, 3, 32);
   defs->world_to_object[1] =
      nir_load_global(b, nir_iadd_imm(b, leaf_addr, 28), 4, 3, 32);
   defs->world_to_object[2] =
      nir_load_global(b, nir_iadd_imm(b, leaf_addr, 40), 4, 3, 32);
   /* The last column of the matrices is swapped between the two probably
    * because it makes it easier/faster for hardware somehow.
    */
   defs->object_to_world[3] =
      nir_load_global(b, nir_iadd_imm(b, leaf_addr, 52), 4, 3, 32);

   nir_ssa_def *data =
      nir_load_global(b, nir_iadd_imm(b, leaf_addr, 64), 4, 4, 32);
   defs->instance_id = nir_channel(b, data, 2);
   defs->instance_index = nir_channel(b, data, 3);

   defs->object_to_world[0] =
      nir_load_global(b, nir_iadd_imm(b, leaf_addr, 80), 4, 3, 32);
   defs->object_to_world[1] =
      nir_load_global(b, nir_iadd_imm(b, leaf_addr, 92), 4, 3, 32);
   defs->object_to_world[2] =
      nir_load_global(b, nir_iadd_imm(b, leaf_addr, 104), 4, 3, 32);
   defs->world_to_object[3] =
      nir_load_global(b, nir_iadd_imm(b, leaf_addr, 116), 4, 3, 32);
}

#endif /* BRW_NIR_RT_BUILDER_H */
