/*
 * Copyright © 2018 Intel Corporation
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

#include "brw_nir.h"
#include "compiler/nir/nir_builder.h"
#include "util/u_math.h"
#include "util/bitscan.h"

static nir_ssa_def *
dup_mem_intrinsic(nir_builder *b, nir_intrinsic_instr *intrin,
                  nir_ssa_def *store_src, int offset,
                  unsigned num_components, unsigned bit_size,
                  unsigned align)
{
   const nir_intrinsic_info *info = &nir_intrinsic_infos[intrin->intrinsic];

   nir_intrinsic_instr *dup =
      nir_intrinsic_instr_create(b->shader, intrin->intrinsic);

   nir_src *intrin_offset_src = nir_get_io_offset_src(intrin);
   for (unsigned i = 0; i < info->num_srcs; i++) {
      assert(intrin->src[i].is_ssa);
      if (i == 0 && store_src) {
         assert(!info->has_dest);
         assert(&intrin->src[i] != intrin_offset_src);
         dup->src[i] = nir_src_for_ssa(store_src);
      } else if (&intrin->src[i] == intrin_offset_src) {
         dup->src[i] = nir_src_for_ssa(nir_iadd_imm(b, intrin->src[i].ssa,
                                                       offset));
      } else {
         dup->src[i] = nir_src_for_ssa(intrin->src[i].ssa);
      }
   }

   dup->num_components = num_components;
   if (intrin->intrinsic == nir_intrinsic_load_scratch ||
       intrin->intrinsic == nir_intrinsic_store_scratch)
      assert(num_components == 1);

   for (unsigned i = 0; i < info->num_indices; i++)
      dup->const_index[i] = intrin->const_index[i];

   if (nir_intrinsic_has_access(intrin))
      nir_intrinsic_set_access(dup, nir_intrinsic_access(intrin));

   nir_intrinsic_set_align(dup, align, 0);

   if (info->has_dest) {
      assert(intrin->dest.is_ssa);
      nir_ssa_dest_init(&dup->instr, &dup->dest,
                        num_components, bit_size, NULL);
   } else {
      nir_intrinsic_set_write_mask(dup, (1 << num_components) - 1);
   }

   nir_builder_instr_insert(b, &dup->instr);

   return info->has_dest ? &dup->dest.ssa : NULL;
}

static bool
lower_mem_load_bit_size(nir_builder *b, nir_intrinsic_instr *intrin,
                        const struct intel_device_info *devinfo)
{
   const bool needs_scalar =
      intrin->intrinsic == nir_intrinsic_load_scratch;

   assert(intrin->dest.is_ssa);
   const unsigned bit_size = intrin->dest.ssa.bit_size;
   const unsigned num_components = intrin->dest.ssa.num_components;
   const unsigned bytes_read = num_components * (bit_size / 8);
   const unsigned align = nir_intrinsic_align(intrin);

   if (bit_size == 32 && align >= 32 && intrin->num_components <= 4 &&
       (!needs_scalar || intrin->num_components == 1))
      return false;

   nir_ssa_def *result;
   nir_src *offset_src = nir_get_io_offset_src(intrin);
   if (bit_size < 32 && !needs_scalar && nir_src_is_const(*offset_src)) {
      /* The offset is constant so we can use a 32-bit load and just shift it
       * around as needed.
       */
      const int load_offset = nir_src_as_uint(*offset_src) % 4;
      assert(load_offset % (bit_size / 8) == 0);
      const unsigned load_comps32 = DIV_ROUND_UP(bytes_read + load_offset, 4);
      /* A 16-bit vec4 is a 32-bit vec2.  We add an extra component in case
       * we offset into a component with load_offset.
       */
      assert(load_comps32 <= 3);

      nir_ssa_def *load = dup_mem_intrinsic(b, intrin, NULL, -load_offset,
                                            load_comps32, 32, 4);
      result = nir_extract_bits(b, &load, 1, load_offset * 8,
                                num_components, bit_size);
   } else {
      /* Otherwise, we have to break it into smaller loads.  We could end up
       * with as many as 32 loads if we're loading a u64vec16 from scratch.
       */
      nir_ssa_def *loads[32];
      unsigned num_loads = 0;
      int load_offset = 0;
      while (load_offset < bytes_read) {
         const unsigned bytes_left = bytes_read - load_offset;
         unsigned load_bit_size, load_comps;
         if (align < 4) {
            load_comps = 1;
            /* Choose a byte, word, or dword */
            load_bit_size = util_next_power_of_two(MIN2(bytes_left, 4)) * 8;
         } else {
            assert(load_offset % 4 == 0);
            load_bit_size = 32;
            load_comps = needs_scalar ? 1 :
                         DIV_ROUND_UP(MIN2(bytes_left, 16), 4);
         }

         loads[num_loads++] = dup_mem_intrinsic(b, intrin, NULL, load_offset,
                                                load_comps, load_bit_size,
                                                align);

         load_offset += load_comps * (load_bit_size / 8);
      }
      assert(num_loads <= ARRAY_SIZE(loads));
      result = nir_extract_bits(b, loads, num_loads, 0,
                                num_components, bit_size);
   }

   nir_ssa_def_rewrite_uses(&intrin->dest.ssa,
                            result);
   nir_instr_remove(&intrin->instr);

   return true;
}

static bool
lower_mem_store_bit_size(nir_builder *b, nir_intrinsic_instr *intrin,
                         const struct intel_device_info *devinfo)
{
   const bool needs_scalar =
      intrin->intrinsic == nir_intrinsic_store_scratch;

   assert(intrin->src[0].is_ssa);
   nir_ssa_def *value = intrin->src[0].ssa;

   assert(intrin->num_components == value->num_components);
   const unsigned bit_size = value->bit_size;
   const unsigned num_components = intrin->num_components;
   const unsigned bytes_written = num_components * (bit_size / 8);
   const unsigned align_mul = nir_intrinsic_align_mul(intrin);
   const unsigned align_offset = nir_intrinsic_align_offset(intrin);
   const unsigned align = nir_intrinsic_align(intrin);

   nir_component_mask_t writemask = nir_intrinsic_write_mask(intrin);
   assert(writemask < (1 << num_components));

   if ((value->bit_size <= 32 && num_components == 1) ||
       (value->bit_size == 32 && num_components <= 4 && align >= 32 &&
        writemask == (1 << num_components) - 1 &&
        !needs_scalar))
      return false;

   nir_src *offset_src = nir_get_io_offset_src(intrin);
   const bool offset_is_const = nir_src_is_const(*offset_src);
   const unsigned const_offset =
      offset_is_const ? nir_src_as_uint(*offset_src) : 0;

   const unsigned byte_size = bit_size / 8;
   assert(byte_size <= sizeof(uint64_t));

   BITSET_DECLARE(mask, NIR_MAX_VEC_COMPONENTS * sizeof(uint64_t));
   BITSET_ZERO(mask);

   for (unsigned i = 0; i < num_components; i++) {
      if (writemask & (1u << i))
         BITSET_SET_RANGE_INSIDE_WORD(mask, i * byte_size, ((i + 1) * byte_size) - 1);
   }

   while (BITSET_FFS(mask) != 0) {
      const int start = BITSET_FFS(mask) - 1;

      int end;
      for (end = start + 1; end < bytes_written; end++) {
         if (!(BITSET_TEST(mask, end)))
            break;
      }
      /* The size of the current contiguous chunk in bytes */
      const unsigned chunk_bytes = end - start;

      const bool is_dword_aligned =
         (align_mul >= 4 && (align_offset + start) % 4 == 0) ||
         (offset_is_const && (start + const_offset) % 4 == 0);

      unsigned store_comps, store_bit_size, store_align;
      if (chunk_bytes >= 4 && is_dword_aligned) {
         store_align = MAX2(align, 4);
         store_bit_size = 32;
         store_comps = needs_scalar ? 1 : MIN2(chunk_bytes, 16) / 4;
      } else {
         store_align = align;
         store_comps = 1;
         store_bit_size = MIN2(chunk_bytes, 4) * 8;
         /* The bit size must be a power of two */
         if (store_bit_size == 24)
            store_bit_size = 16;
      }
      const unsigned store_bytes = store_comps * (store_bit_size / 8);

      nir_ssa_def *packed = nir_extract_bits(b, &value, 1, start * 8,
                                             store_comps, store_bit_size);

      dup_mem_intrinsic(b, intrin, packed, start,
                        store_comps, store_bit_size, store_align);

      BITSET_CLEAR_RANGE(mask, start, (start + store_bytes - 1));
   }

   nir_instr_remove(&intrin->instr);

   return true;
}

static bool
lower_mem_access_bit_sizes_instr(nir_builder *b,
                                nir_instr *instr,
                                void *cb_data)
{
   const struct intel_device_info *devinfo = cb_data;

   if (instr->type != nir_instr_type_intrinsic)
      return false;

   b->cursor = nir_after_instr(instr);

   nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_shared:
   case nir_intrinsic_load_scratch:
      return lower_mem_load_bit_size(b, intrin, devinfo);

   case nir_intrinsic_store_global:
   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_store_shared:
   case nir_intrinsic_store_scratch:
      return lower_mem_store_bit_size(b, intrin, devinfo);

   default:
      return false;
   }
}

/**
 * This pass loads arbitrary SSBO and shared memory load/store operations to
 * intrinsics which are natively handleable by GEN hardware.  In particular,
 * we have two general types of memory load/store messages:
 *
 *  - Untyped surface read/write:  These can load/store between one and four
 *    dword components to/from a dword-aligned offset.
 *
 *  - Byte scattered read/write:  These can load/store a single byte, word, or
 *    dword scalar to/from an unaligned byte offset.
 *
 * Neither type of message can do a write-masked store.  This pass converts
 * all nir load/store intrinsics into a series of either 8 or 32-bit
 * load/store intrinsics with a number of components that we can directly
 * handle in hardware and with a trivial write-mask.
 *
 * For scratch access, additional consideration has to be made due to the way
 * that we swizzle the memory addresses to achieve decent cache locality.  In
 * particular, even though untyped surface read/write messages exist and work,
 * we can't use them to load multiple components in a single SEND.  For more
 * detail on the scratch swizzle, see fs_visitor::swizzle_nir_scratch_addr.
 */
bool
brw_nir_lower_mem_access_bit_sizes(nir_shader *shader,
                                   const struct intel_device_info *devinfo)
{
   return nir_shader_instructions_pass(shader, lower_mem_access_bit_sizes_instr,
                                       nir_metadata_block_index |
                                       nir_metadata_dominance,
                                       (void *)devinfo);
}
